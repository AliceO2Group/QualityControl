// Copyright CERN and copyright holders of ALICE O2. This software is
// distributed under the terms of the GNU General Public License v3 (GPL
// Version 3), copied verbatim in the file "COPYING".
//
// See http://alice-o2.web.cern.ch/license for full licensing information.
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
// ROOT
#include <TClass.h>
// O2
#include <Common/Exceptions.h>
#include <Configuration/ConfigurationFactory.h>
#include <Framework/DataDescriptorQueryBuilder.h>
// QC
#include "QualityControl/TaskRunner.h"
#include "QualityControl/InputUtils.h"
#include "QualityControl/RootClassFactory.h"
#include "QualityControl/PostProcessingDevice.h"
// Fairlogger
#include <fairlogger/Logger.h>

using namespace AliceO2::Common;
using namespace AliceO2::InfoLogger;
using namespace o2::configuration;

using namespace o2::quality_control::checker;
using namespace o2::quality_control::core;
using namespace o2::quality_control::postprocessing;
using namespace std;

/// Static functions
o2::header::DataDescription Check::createCheckerDataDescription(const std::string name)
{
  if (name.empty()) {
    BOOST_THROW_EXCEPTION(FatalException() << errinfo_details("Empty taskName for checker's data description"));
  }
  o2::header::DataDescription description;
  description.runtimeInit(std::string(name.substr(0, o2::header::DataDescription::size - 4) + "-chk").c_str());
  return description;
}

/// Members

Check::Check(std::string checkName, std::string configurationSource)
  : mConfigurationSource(configurationSource),
    mLogger(QcInfoLogger::GetInstance()),
    mNumberOfTaskSources(0),
    mInputs{},
    mOutputSpec{ "QC", Check::createCheckerDataDescription(checkName), 0 },
    mBeautify(true)
{
  try {
    initConfig(checkName);
  } catch (...) {
    std::string diagnostic = boost::current_exception_diagnostic_information();
    ILOG(Fatal, Ops) << "Unexpected exception, diagnostic information follows:\n"
                     << diagnostic << ENDM;
    throw;
  }
}

void Check::initConfig(std::string checkName)
{
  mCheckConfig.name = checkName;

  std::unique_ptr<ConfigurationInterface> config = ConfigurationFactory::getConfiguration(mConfigurationSource);
  const auto& checkConfig = config->getRecursive("qc.checks." + mCheckConfig.name);

  // Params
  if (checkConfig.count("checkParameters")) {
    mCheckConfig.customParameters = config->getRecursiveMap("qc.checks." + checkName + ".checkParameters");
  }

  // Policy
  if (checkConfig.count("policy")) {
    mCheckConfig.policyType = checkConfig.get<std::string>("policy");
  }

  // Inputs
  mNumberOfTaskSources = 0;
  for (const auto& [_key, dataSource] : checkConfig.get_child("dataSource")) {
    (void)_key;
    if (auto sourceType = dataSource.get<std::string>("type");
        sourceType == "Task" || sourceType == "ExternalTask" || sourceType == "PostProcessing") {
      auto taskName = dataSource.get<std::string>("name");
      mNumberOfTaskSources++;

      if (dataSource.get<std::string>("type") == "Task") {
        mInputs.push_back({ taskName, TaskRunner::createTaskDataOrigin(), TaskRunner::createTaskDataDescription(taskName) });
      } else if (dataSource.get<std::string>("type") == "PostProcessing") {
        mInputs.push_back({ taskName, PostProcessingDevice::createPostProcessingDataOrigin(), PostProcessingDevice::createPostProcessingDataDescription(taskName) });
      } else if (dataSource.get<std::string>("type") == "ExternalTask") {
        auto query = config->getString("qc.externalTasks." + taskName + ".query").get();
        framework::Inputs input = o2::framework::DataDescriptorQueryBuilder::parse(query.c_str());
        mInputs.insert(mInputs.end(), std::make_move_iterator(input.begin()),
                       std::make_move_iterator(input.end()));
      }

      // Subscribe on predefined MOs.
      // If "MOs" are not set, the check function will be triggered whenever a new MO appears.
      if (dataSource.count("MOs") == 0) {
        // fixme: this is a dirty fix. Policies should be refactored, so this check won't be needed.
        if (mCheckConfig.policyType != "OnEachSeparately") {
          mCheckConfig.policyType = "_OnGlobalAny";
        }
        mCheckConfig.allObjects = true;
      } else {
        for (const auto& moName : dataSource.get_child("MOs")) {
          auto name = std::string(taskName + "/" + moName.second.get_value<std::string>());
          if (std::find(mCheckConfig.objectNames.begin(), mCheckConfig.objectNames.end(), name) == mCheckConfig.objectNames.end()) {
            mCheckConfig.objectNames.push_back(name);
          }
        }
      }
    }

    // Support for sources other than Tasks can be implemented here.
  }
  mInputsStringified = stringifyInput(mInputs);

  // Prepare module loading
  mCheckConfig.moduleName = checkConfig.get<std::string>("moduleName");
  mCheckConfig.className = checkConfig.get<std::string>("className");

  // Detector name, if none use "DET"
  mCheckConfig.detectorName = checkConfig.get<std::string>("detectorName", "DET");
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

  // Determine whether we can beautify
  // See QC-299 for details
  if (mNumberOfTaskSources > 1) {
    mBeautify = false;
    ILOG(Warning, Devel) << "Beautification disabled because more than one source is used in this Check (" << mCheckConfig.name << ")" << ENDM;
  }

  // Print setting
  mLogger << mCheckConfig.name << ": Module " << mCheckConfig.moduleName << AliceO2::InfoLogger::InfoLogger::endm;
  mLogger << mCheckConfig.name << ": Class " << mCheckConfig.className << AliceO2::InfoLogger::InfoLogger::endm;
  mLogger << mCheckConfig.name << ": Detector " << mCheckConfig.detectorName << AliceO2::InfoLogger::InfoLogger::endm;
  mLogger << mCheckConfig.name << ": Policy " << mCheckConfig.policyType << AliceO2::InfoLogger::InfoLogger::endm;
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
  if (mCheckConfig.policyType == "OnEachSeparately") {
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
      mCheckConfig.policyType,
      mInputsStringified,
      monitorObjectsNames));
    beautify(moMapToCheck, quality);
  }

  return qualityObjects;
}

void Check::beautify(std::map<std::string, std::shared_ptr<MonitorObject>>& moMap, Quality quality)
{
  if (!mBeautify) {
    return;
  }

  for (auto const& item : moMap) {
    mCheckInterface->beautify(item.second /*mo*/, quality);
  }
}

std::string Check::getPolicyName() const
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
