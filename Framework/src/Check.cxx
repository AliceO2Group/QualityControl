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
// ROOT
#include <TClass.h>
// O2
#include <Common/Exceptions.h>
#include <Configuration/ConfigurationFactory.h>
// QC
#include "QualityControl/TaskRunner.h"
#include "QualityControl/InputUtils.h"
#include "RootClassFactory.h"

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
    mLatestQuality(std::make_shared<QualityObject>(checkName)),
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
    LOG(ERROR) << "Unexpected exception, diagnostic information follows:\n" << diagnostic;
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
  size_t numberOfTaskSources = 0;
  for (const auto& [_key, dataSource] : checkConfig.get_child("dataSource")) {
    (void)_key;
    if (dataSource.get<std::string>("type") == "Task") {
      auto taskName = dataSource.get<std::string>("name");
      numberOfTaskSources++;
      mInputs.push_back({ taskName, TaskRunner::createTaskDataOrigin(), TaskRunner::createTaskDataDescription(taskName) });

      /*
         * Subscribe on predefined MOs.
         * If "MOs" not set or "MOs" set to "all", the check function will be triggered whenever a new MO appears.
         */
      if (dataSource.count("MOs") == 0 || dataSource.get<std::string>("MOs") == "all") {
        mCheckConfig.policyType = "_OnGlobalAny";
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

    // Here can be implemented other sources for the Check then Task if needed
  }
  mLatestQuality->setInputs(stringifyInput(mInputs));

  // Prepare module loading
  mCheckConfig.moduleName = checkConfig.get<std::string>("moduleName");
  mCheckConfig.className = checkConfig.get<std::string>("className");

  // Detector name, if none use "DET"
  mCheckConfig.detectorName = checkConfig.get<std::string>("detectorName", "DET");
  mLatestQuality->setDetectorName(mCheckConfig.detectorName);

  // Determine whether we can beautify
  // See QC-299 for details
  if (numberOfTaskSources > 1) {
    mBeautify = false;
    ILOG(Warning) << "Beautification disabled because more than one source is used in this Check (" << mCheckConfig.checkName << ")" << ENDM;
  }

  // Print setting
  mLogger << checkName << ": Module " << mCheckConfig.moduleName << AliceO2::InfoLogger::InfoLogger::endm;
  mLogger << checkName << ": Class " << mCheckConfig.className << AliceO2::InfoLogger::InfoLogger::endm;
  mLogger << checkName << ": Detector " << mCheckConfig.detectorName << AliceO2::InfoLogger::InfoLogger::endm;
  mLogger << checkName << ": Policy " << mCheckConfig.policyType << AliceO2::InfoLogger::InfoLogger::endm;
  mLogger << checkName << ": MonitorObjects : " << AliceO2::InfoLogger::InfoLogger::endm;
  for (const auto& moname : mCheckConfig.moNames) {
    mLogger << checkName << "   - " << moname << AliceO2::InfoLogger::InfoLogger::endm;
  }
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
     * Guaranee that all declared MOs are available 
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

  } else if (policyType == "_OnGlobalAny") {
    /**
     * Return true if any MOs were updated.
     * Inner policy - used for `"MOs": "all"`
     * Might return true even if MO is not used in Check
     */

    mPolicy = [](std::map<std::string, unsigned int>& revisionMap) {
      // Expecting check of this policy only if any change
      (void)revisionMap; // Supprses Unused warning
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
}

bool Check::isReady(std::map<std::string, unsigned int>& revisionMap)
{
  return mPolicy(revisionMap);
}

void Check::updateRevision(unsigned int revision)
{
  mMORevision = revision;
}

std::shared_ptr<QualityObject> Check::check(std::map<std::string, std::shared_ptr<MonitorObject>>& moMap)
{
  // Check if the module with the function is loaded
  if (mCheckInterface != nullptr) {
    std::shared_ptr<Quality> quality;
    if (mCheckConfig.allMOs) {
      /* 
       * User didn't specify the MOs.
       * All MOs are passed, no shadowing needed.
       */
      mLatestQuality->updateQuality(mCheckInterface->check(&moMap));
      // Trigger beautification
      beautify(moMap);
    } else {
      /* 
       * Shadow MOs.
       * Don't pass MOs that weren't specified by user.
       * The user might safely relay on getting only required MOs inside the map.
       *
       * Implementation: Copy to different map only required MOs.
       */
      std::map<std::string, std::shared_ptr<MonitorObject>> shadowMap;
      for (auto& key : mCheckConfig.moNames) {
        if (moMap.count(key)) {
          // don't create empty shared_ptr
          shadowMap.insert({ key, moMap[key] });
        }
      }

      // Trigger loaded check and update quality of the Check.
      mLatestQuality->updateQuality(mCheckInterface->check(&shadowMap));
      // Trigger beautification
      beautify(shadowMap);
    }
  }
  mLogger << mCheckConfig.checkName << " Quality: " << mLatestQuality->getQuality() << AliceO2::InfoLogger::InfoLogger::endm;

  return mLatestQuality;
}

void Check::beautify(std::map<std::string, std::shared_ptr<MonitorObject>>& moMap)
{
  if (!mBeautify) {
    return;
  }

  for (auto const& [name, mo] : moMap) {
    mCheckInterface->beautify(mo, mLatestQuality->getQuality());
  }
}
