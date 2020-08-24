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

using namespace AliceO2::Common;
using namespace AliceO2::InfoLogger;
using namespace o2::configuration;

using namespace o2::quality_control::checker;
using namespace o2::quality_control::core;
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
  mPolicy = [](std::map<std::string, unsigned int>) {
    // Prevent from using of uninitiated policy
    BOOST_THROW_EXCEPTION(FatalException() << errinfo_details("Policy not initiated: try to run Check::init() first"));
    return false;
  };

  try {
    initConfig(checkName);
  } catch (...) {
    std::string diagnostic = boost::current_exception_diagnostic_information();
    LOG(ERROR) << "Unexpected exception, diagnostic information follows:\n"
               << diagnostic;
    throw;
  }
}

void Check::initConfig(std::string checkName)
{
  mCheckConfig.checkName = checkName;

  std::unique_ptr<ConfigurationInterface> config = ConfigurationFactory::getConfiguration(mConfigurationSource);
  std::vector<std::string> inputs;
  const auto& checkConfig = config->getRecursive("qc.checks." + mCheckConfig.checkName);

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
    if (dataSource.get<std::string>("type") == "Task" || dataSource.get<std::string>("type") == "ExternalTask") {
      auto taskName = dataSource.get<std::string>("name");
      mNumberOfTaskSources++;

      if (dataSource.get<std::string>("type") == "Task") {
        mInputs.push_back({ taskName, TaskRunner::createTaskDataOrigin(), TaskRunner::createTaskDataDescription(taskName) });
      } else if (dataSource.get<std::string>("type") == "ExternalTask") {
        auto query = config->getString("qc.externalTasks." + taskName + ".query").get();
        framework::Inputs input = o2::framework::DataDescriptorQueryBuilder::parse(query.c_str());
        mInputs.insert(mInputs.end(), std::make_move_iterator(input.begin()),
                       std::make_move_iterator(input.end()));
      }

      // Subscribe on predefined MOs.
      // If "MOs" are not set or "MOs" is set to "all", the check function will be triggered whenever a new MO appears.
      if (dataSource.count("MOs") == 0 || dataSource.get<std::string>("MOs") == "all") {
        // fixme: this is a dirty fix. Policies should be refactored, so this check won't be needed.
        if (mCheckConfig.policyType != "OnEachSeparately") {
          mCheckConfig.policyType = "_OnGlobalAny";
        }
        mCheckConfig.allMOs = true;
      } else {
        for (const auto& moName : dataSource.get_child("MOs")) {
          auto name = std::string(taskName + "/" + moName.second.get_value<std::string>());
          if (std::find(mCheckConfig.moNames.begin(), mCheckConfig.moNames.end(), name) == mCheckConfig.moNames.end()) {
            mCheckConfig.moNames.push_back(name);
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

void Check::initPolicy(std::string policyType)
{
  if (policyType == "OnAll") {
    /** 
     * Run check if all MOs are updated 
     */

    mPolicy = [&](std::map<std::string, unsigned int>& revisionMap) {
      for (const auto& moname : mCheckConfig.moNames) {
        if (revisionMap[moname] <= mMORevision) {
          // Expect: revisionMap[notExistingKey] == 0
          return false;
        }
      }
      return true;
    };
  } else if (policyType == "OnAnyNonZero") {
    /**
     * Return true if any declared MOs were updated
     * Guarantee that all declared MOs are available
     */
    mPolicy = [&](std::map<std::string, unsigned int>& revisionMap) {
      if (!mPolicyHelper) {
        // Check if all monitor objects are available
        for (const auto& moname : mCheckConfig.moNames) {
          if (!revisionMap.count(moname)) {
            return false;
          }
        }
        // From now on all MOs are available
        mPolicyHelper = true;
      }

      for (const auto& moname : mCheckConfig.moNames) {
        if (revisionMap[moname] > mMORevision) {
          return true;
        }
      }
      return false;
    };

  } else if (policyType == "OnEachSeparately") {
    /**
     * Return true if any declared MOs were updated
     * This is the same behaviour as OnAny, but we should pass
     * only one MO to a check at once.
     */
    mPolicy = [&](std::map<std::string, unsigned int>& revisionMap) {
      if (mCheckConfig.allMOs) {
        return true;
      }

      for (const auto& moname : mCheckConfig.moNames) {
        if (revisionMap.count(moname) && revisionMap[moname] > mMORevision) {
          return true;
        }
      }
      return false;
    };

  } else if (policyType == "_OnGlobalAny") {
    /**
     * Return true if any MOs were updated.
     * Inner policy - used for `"MOs": "all"`
     * Might return true even if MO is not used in Check
     */

    mPolicy = [](std::map<std::string, unsigned int>& revisionMap) {
      // Expecting check of this policy only if any change
      (void)revisionMap; // Suppress Unused warning
      return true;
    };

  } else /* if (policyType == "OnAny") */ {
    /**
     * Default behaviour
     *
     * Run check if any declared MOs are updated
     * Does not guarantee to contain all declared MOs 
     */
    mPolicy = [&](std::map<std::string, unsigned int>& revisionMap) {
      for (const auto& moname : mCheckConfig.moNames) {
        if (revisionMap.count(moname) && revisionMap[moname] > mMORevision) {
          return true;
        }
      }
      return false;
    };
  }
}

void Check::init()
{
  try {
    mCheckInterface = root_class_factory::create<CheckInterface>(mCheckConfig.moduleName, mCheckConfig.className);
    mCheckInterface->setCustomParameters(mCheckConfig.customParameters);
    mCheckInterface->configure(mCheckConfig.checkName);
  } catch (...) {
    std::string diagnostic = boost::current_exception_diagnostic_information();
    LOG(ERROR) << "Unexpected exception, diagnostic information follows:\n"
               << diagnostic;
    throw;
  }

  /** 
   * The policy needs to be here. If running in constructor, the lambda gets wrong reference
   * and runs into SegmentationFault.
   */
  initPolicy(mCheckConfig.policyType);

  // Determine whether we can beautify
  // See QC-299 for details
  if (mNumberOfTaskSources > 1) {
    mBeautify = false;
    ILOG(Warning) << "Beautification disabled because more than one source is used in this Check (" << mCheckConfig.checkName << ")" << ENDM;
  }

  // Print setting
  mLogger << mCheckConfig.checkName << ": Module " << mCheckConfig.moduleName << AliceO2::InfoLogger::InfoLogger::endm;
  mLogger << mCheckConfig.checkName << ": Class " << mCheckConfig.className << AliceO2::InfoLogger::InfoLogger::endm;
  mLogger << mCheckConfig.checkName << ": Detector " << mCheckConfig.detectorName << AliceO2::InfoLogger::InfoLogger::endm;
  mLogger << mCheckConfig.checkName << ": Policy " << mCheckConfig.policyType << AliceO2::InfoLogger::InfoLogger::endm;
  mLogger << mCheckConfig.checkName << ": MonitorObjects : " << AliceO2::InfoLogger::InfoLogger::endm;
  for (const auto& moname : mCheckConfig.moNames) {
    mLogger << mCheckConfig.checkName << "   - " << moname << AliceO2::InfoLogger::InfoLogger::endm;
  }
}

bool Check::isReady(std::map<std::string, unsigned int>& revisionMap)
{
  return mPolicy(revisionMap);
}

void Check::updateRevision(unsigned int revision)
{
  mMORevision = revision;
}

QualityObjectsType Check::check(std::map<std::string, std::shared_ptr<MonitorObject>>& moMap)
{
  if (mCheckInterface == nullptr) {
    BOOST_THROW_EXCEPTION(FatalException() << errinfo_details("Attempting to check, but no CheckInterface is loaded"));
  }

  std::map<std::string, std::shared_ptr<MonitorObject>> shadowMap;
  // Take only the MOs which are needed to be checked
  if (mCheckConfig.allMOs) {
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
    for (auto& key : mCheckConfig.moNames) {
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
    mLogger << "Check '" << mCheckConfig.checkName << "', quality '" << quality << "'" << ENDM;
    // todo: take metadata from somewhere
    qualityObjects.emplace_back(std::make_shared<QualityObject>(
      quality,
      mCheckConfig.checkName,
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