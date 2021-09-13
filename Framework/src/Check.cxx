// Copyright 2019-2020 CERN and copyright holders of ALICE O2.
// See https://alice-o2.web.cern.ch/copyright for details of the copyright holders.
// All rights not expressly granted are reserved.
//
// This software is distributed under the terms of the GNU General Public
// License v3 (GPL Version 3), copied verbatim in the file "COPYING".
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.

#include "QualityControl/Check.h"

#include <memory>
#include <algorithm>
// boost
#include <boost/range/adaptor/map.hpp>
#include <boost/range/algorithm/copy.hpp>
#include <utility>
// ROOT
#include <TClass.h>
// O2
#include <Common/Exceptions.h>
#include <Framework/DataDescriptorQueryBuilder.h>
// QC
#include "QualityControl/TaskRunner.h"
#include "QualityControl/InputUtils.h"
#include "QualityControl/RootClassFactory.h"

using namespace AliceO2::Common;
using namespace AliceO2::InfoLogger;
using namespace o2::configuration;

using namespace o2::quality_control::checker;
using namespace std;

namespace o2::quality_control::checker
{

/// Static functions
o2::header::DataDescription Check::createCheckerDataDescription(const std::string& name)
{
  if (name.empty()) {
    BOOST_THROW_EXCEPTION(FatalException() << errinfo_details("Empty taskName for checker's data description"));
  }
  o2::header::DataDescription description;
  description.runtimeInit(std::string(name.substr(0, o2::header::DataDescription::size - 4) + "-chk").c_str());
  return description;
}

/// Members
Check::Check(CheckConfig config)
  : mLogger(QcInfoLogger::GetInstance()),
    mCheckConfig(std::move(config))
{
}

void Check::init()
{
  try {
    mCheckInterface = root_class_factory::create<CheckInterface>(mCheckConfig.moduleName, mCheckConfig.className);
    mCheckInterface->setCustomParameters(mCheckConfig.customParameters);
    mCheckInterface->configure(mCheckConfig.name);
  } catch (...) {
    std::string diagnostic = boost::current_exception_diagnostic_information();
    ILOG(Fatal, Ops) << "Unexpected exception, diagnostic information follows:\n"
                     << diagnostic << ENDM;
    throw;
  }

  // Print setting
  mLogger << mCheckConfig.name << ": Module " << mCheckConfig.moduleName << AliceO2::InfoLogger::InfoLogger::endm;
  mLogger << mCheckConfig.name << ": Class " << mCheckConfig.className << AliceO2::InfoLogger::InfoLogger::endm;
  mLogger << mCheckConfig.name << ": Detector " << mCheckConfig.detectorName << AliceO2::InfoLogger::InfoLogger::endm;
  mLogger << mCheckConfig.name << ": Policy " << UpdatePolicyTypeUtils::ToString(mCheckConfig.policyType) << AliceO2::InfoLogger::InfoLogger::endm;
  mLogger << mCheckConfig.name << ": MonitorObjects : " << AliceO2::InfoLogger::InfoLogger::endm;
  for (const auto& moname : mCheckConfig.objectNames) {
    mLogger << mCheckConfig.name << "   - " << moname << AliceO2::InfoLogger::InfoLogger::endm;
  }
}

QualityObjectsType Check::check(std::map<std::string, std::shared_ptr<MonitorObject>>& moMap)
{
  if (mCheckInterface == nullptr) {
    BOOST_THROW_EXCEPTION(FatalException() << errinfo_details("Attempting to check, but no CheckInterface is loaded"));
  }

  std::map<std::string, std::shared_ptr<MonitorObject>> shadowMap;
  // Take only the MOs which are needed to be checked
  if (mCheckConfig.allObjects) {
    /*
     * User didn't specify the MOs.
     * All MOs are passed, no shadowing needed.
     */
    shadowMap = moMap;
  } else {
    /*
     * Shadow MOs.
     * Don't pass MOs that weren't specified by user.
     * The user might safely rely on getting only required MOs inside the map.
     *
     * Implementation: Copy to different map only required MOs.
     */
    for (auto& key : mCheckConfig.objectNames) {
      // don't create empty shared_ptr
      if (moMap.count(key)) {
        shadowMap.insert({ key, moMap[key] });
      }
    }
  }

  // Prepare a vector of MO maps to be checked, each one will receive a separate Quality.
  std::vector<std::map<std::string, std::shared_ptr<MonitorObject>>> moMapsToCheck;
  if (mCheckConfig.policyType == UpdatePolicyType::OnEachSeparately) {
    // In this case we want to check all MOs separately and we get separate QOs for them.
    for (auto mo : shadowMap) {
      moMapsToCheck.push_back({ std::move(mo) });
    }
  } else {
    moMapsToCheck.emplace_back(shadowMap);
  }

  QualityObjectsType qualityObjects;
  for (auto& moMapToCheck : moMapsToCheck) {
    std::vector<std::string> monitorObjectsNames;
    boost::copy(moMapToCheck | boost::adaptors::map_keys, std::back_inserter(monitorObjectsNames));

    auto quality = mCheckInterface->check(&moMapToCheck);
    mLogger << "Check '" << mCheckConfig.name << "', quality '" << quality << "'" << ENDM;
    // todo: take metadata from somewhere
    qualityObjects.emplace_back(std::make_shared<QualityObject>(
      quality,
      mCheckConfig.name,
      mCheckConfig.detectorName,
      UpdatePolicyTypeUtils::ToString(mCheckConfig.policyType),
      stringifyInput(mCheckConfig.inputSpecs),
      monitorObjectsNames));
    beautify(moMapToCheck, quality);
  }

  return qualityObjects;
}

void Check::beautify(std::map<std::string, std::shared_ptr<MonitorObject>>& moMap, Quality quality)
{
  if (!mCheckConfig.allowBeautify) {
    return;
  }

  for (auto const& item : moMap) {
    mCheckInterface->beautify(item.second /*mo*/, quality);
  }
}
UpdatePolicyType Check::getUpdatePolicyType() const
{
  return mCheckConfig.policyType;
}

std::vector<std::string> Check::getObjectsNames() const
{
  return mCheckConfig.objectNames;
}

bool Check::getAllObjectsOption() const
{
  return mCheckConfig.allObjects;
}

CheckConfig Check::extractConfig(const CommonSpec&, const CheckSpec& checkSpec)
{
  framework::Inputs inputs;
  std::vector<std::string> objectNames;
  UpdatePolicyType updatePolicy = checkSpec.updatePolicy;
  bool checkAllObjects = false;
  for (const auto& dataSource : checkSpec.dataSources) {
    if (!dataSource.isOneOf(DataSourceType::Task, DataSourceType::ExternalTask, DataSourceType::PostProcessingTask)) {
      throw std::runtime_error(
        "Unsupported dataSource '" + dataSource.name + "' for a Check '" + checkSpec.checkName + "'");
    }
    inputs.insert(inputs.end(), dataSource.inputs.begin(), dataSource.inputs.end());

    // Subscribe on predefined MOs.
    // If "MOs" are not set, the check function will be triggered whenever a new MO appears.
    if (dataSource.subInputs.empty()) {
      // fixme: this is a dirty fix. Policies should be refactored, so this check won't be needed.
      if (checkSpec.updatePolicy != UpdatePolicyType::OnEachSeparately) {
        updatePolicy = UpdatePolicyType::OnGlobalAny;
      }
      checkAllObjects = true;
    } else {
      // todo consider moving this to spec reader
      for (const auto& moName : dataSource.subInputs) {
        auto name = dataSource.name + "/" + moName;
        // todo: consider making objectNames an std::set
        if (std::find(objectNames.begin(), objectNames.end(), name) == objectNames.end()) {
          objectNames.push_back(name);
        }
      }
    }
  }

  bool allowBeautify = checkSpec.dataSources.size() <= 1;
  if (!allowBeautify) {
    // See QC-299 for details
    ILOG(Warning, Devel) << "Beautification disabled because more than one source is used in this Check (" << checkSpec.checkName << ")" << ENDM;
  }

  framework::OutputSpec qoSpec{ "QC", createCheckerDataDescription(checkSpec.checkName), 0 };

  return {
    checkSpec.checkName,
    checkSpec.moduleName,
    checkSpec.className,
    checkSpec.detectorName,
    checkSpec.customParameters,
    updatePolicy,
    std::move(objectNames),
    checkAllObjects,
    allowBeautify,
    std::move(inputs),
    std::move(qoSpec)
  };
}

} // namespace o2::quality_control::checker