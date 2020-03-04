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
// Boost
#include <boost/filesystem/path.hpp>
#include <boost/algorithm/string/trim.hpp>
// ROOT
#include <TClass.h>
#include <TMessage.h>
#include <TSystem.h>
// O2
#include <Common/Exceptions.h>
#include <Configuration/ConfigurationFactory.h>
// QC
#include "QualityControl/TaskRunner.h"
#include "QualityControl/InputUtils.h"

using namespace AliceO2::Common;
using namespace AliceO2::InfoLogger;
using namespace o2::configuration;

using namespace o2::quality_control::checker;
using namespace o2::quality_control::core;

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
  : mName(checkName),
    mConfigurationSource(configurationSource),
    mLogger(QcInfoLogger::GetInstance()),
    mLatestQuality(std::make_shared<QualityObject>(checkName)),
    mInputs{},
    mOutputSpec{ "QC", Check::createCheckerDataDescription(checkName), 0 },
    mPolicyType("OnAny")
{
  mPolicy = [](std::map<std::string, unsigned int>) {
    // Prevent from using of uninitiated policy
    BOOST_THROW_EXCEPTION(FatalException() << errinfo_details("Policy not initiated: try to run Check::init() first"));
    return false;
  };

  initConfig();
}

void Check::initConfig()
{
  try {
    std::unique_ptr<ConfigurationInterface> config = ConfigurationFactory::getConfiguration(mConfigurationSource);
    std::vector<std::string> inputs;
    const auto& conf = config->getRecursive("qc.checks." + mName);
    // Params
    if (conf.count("checkParameters")) {
      mCustomParameters = config->getRecursiveMap("qc.checks." + mName + ".checkParameters");
    }

    // Policy
    if (conf.count("policy")) {
      mPolicyType = conf.get<std::string>("policy");
    }

    // Inputs
    for (const auto& [_key, dataSource] : conf.get_child("dataSource")) {
      (void)_key;
      if (dataSource.get<std::string>("type") == "Task") {
        auto taskName = dataSource.get<std::string>("name");

        mInputs.push_back({ taskName, TaskRunner::createTaskDataOrigin(), TaskRunner::createTaskDataDescription(taskName) });

        /*
           * Subscribe on predefined MOs.
           * If "MOs" not set or "MOs" set to "all", the check function will be triggered whenever a new MO appears.
           */
        if (dataSource.count("MOs") == 0 || dataSource.get<std::string>("MOs") == "all") {
          mPolicyType = "_OnGlobalAny";
          mAllMOs = true;
        } else {
          for (const auto& moName : dataSource.get_child("MOs")) {
            auto name = std::string(taskName + "/" + moName.second.get_value<std::string>());
            if (std::find(mMonitorObjectNames.begin(), mMonitorObjectNames.end(), name) == mMonitorObjectNames.end()) {
              mMonitorObjectNames.push_back(name);
            }
          }
        }
      }

      // Here can be implemented other sources for the Check then Task if needed
    }

    mLatestQuality->setInputs(stringifyInput(mInputs));

    // Prepare module loading
    mModuleName = conf.get<std::string>("moduleName");
    mClassName = conf.get<std::string>("className");

    // Detector name, if none use "DET"
    mDetectorName = conf.get<std::string>("detectorName", "DET");
    mLatestQuality->setDetectorName(mDetectorName);

    // Print setting
    mLogger << mName << ": Module " << mModuleName << AliceO2::InfoLogger::InfoLogger::endm;
    mLogger << mName << ": Class " << mClassName << AliceO2::InfoLogger::InfoLogger::endm;
    mLogger << mName << ": Detector " << mDetectorName << AliceO2::InfoLogger::InfoLogger::endm;
    mLogger << mName << ": Policy " << mPolicyType << AliceO2::InfoLogger::InfoLogger::endm;
    mLogger << mName << ": MonitorObjects" << AliceO2::InfoLogger::InfoLogger::endm;
    for (const auto& moname : mMonitorObjectNames) {
      mLogger << mName << " - " << moname << AliceO2::InfoLogger::InfoLogger::endm;
    }

  } catch (...) {
    std::string diagnostic = boost::current_exception_diagnostic_information();
    LOG(ERROR) << "Unexpected exception, diagnostic information follows:\n"
               << diagnostic;
    throw;
  }
}

void Check::initPolicy(std::string policyType)
{
  if (policyType == "OnAll") {
    /** 
     * Run check if all MOs are updated 
     */

    mPolicy = [&](std::map<std::string, unsigned int>& revisionMap) {
      for (const auto& moname : mMonitorObjectNames) {
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
        for (const auto& moname : mMonitorObjectNames) {
          if (!revisionMap.count(moname)) {
            return false;
          }
        }
        // From now on all MOs are available
        mPolicyHelper = true;
      }

      for (const auto& moname : mMonitorObjectNames) {
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
      for (const auto& moname : mMonitorObjectNames) {
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
  loadLibrary();

  /** 
   * The policy needs to be here. If running in constructor, the lambda gets wrong reference
   * and runs into SegmentationFault.
   */
  initPolicy(mPolicyType);
}

void Check::loadLibrary()
{
  // Load module
  if (boost::algorithm::trim_copy(mModuleName).empty()) {
    mLogger << "no library name specified" << AliceO2::InfoLogger::InfoLogger::endm;
    return;
  }

  std::string library = "lib" + mModuleName;
  // if vector does not contain -> first time we see it
  mLogger << "Loading library " << library << AliceO2::InfoLogger::InfoLogger::endm;
  int libLoaded = gSystem->Load(library.c_str(), "", true);
  if (libLoaded == 1) {
    mLogger << "Already loaded before" << AliceO2::InfoLogger::InfoLogger::endm;
  } else if (libLoaded < 0 || libLoaded > 1) {
    BOOST_THROW_EXCEPTION(FatalException() << errinfo_details("Failed to load Detector Publisher Library"));
  }

  // Load class

  // Get the class
  TClass* cl;
  std::string tempString("Failed to instantiate Quality Control Module");

  mLogger << "Loading class " << mClassName << AliceO2::InfoLogger::InfoLogger::endm;
  cl = TClass::GetClass(mClassName.c_str());
  if (!cl) {
    tempString += R"( because no dictionary for class named ")";
    tempString += mClassName;
    tempString += R"(" could be retrieved)";
    LOG(ERROR) << tempString;
    BOOST_THROW_EXCEPTION(FatalException() << errinfo_details(tempString));
  }

  //Get instance
  mLogger << "Instantiating class " << mClassName << " (" << cl << ")" << AliceO2::InfoLogger::InfoLogger::endm;
  mCheckInterface = static_cast<CheckInterface*>(cl->New());
  if (!mCheckInterface) {
    tempString += R"( because the class named ")";
    tempString += mClassName;
    tempString += R"( because the class named ")";
    BOOST_THROW_EXCEPTION(FatalException() << errinfo_details(tempString));
  }
  mCheckInterface->setCustomParameters(mCustomParameters);
  mCheckInterface->configure(mName);
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
    if (mAllMOs) {
      /* 
       * User didn't specify the MOs.
       * All MOs are passed, no shadowing needed.
       */
      mLatestQuality->updateQuality(mCheckInterface->check(&moMap));
    } else {
      /* 
       * Shadow MOs.
       * Don't pass MOs that weren't specified by user.
       * The user might safely relay on getting only required MOs inside the map.
       *
       * Implementation: Copy to different map only required MOs.
       */
      std::map<std::string, std::shared_ptr<MonitorObject>> shadowMap;
      for (auto& key : mMonitorObjectNames) {
        if (moMap.count(key)) {
          // don't create empty shared_ptr
          shadowMap.insert({ key, moMap[key] });
        }
      }

      // Trigger loaded check and update quality of the Check.
      mLatestQuality->updateQuality(mCheckInterface->check(&shadowMap));
    }
  }
  mLogger << mName << " Quality: " << mLatestQuality->getQuality() << AliceO2::InfoLogger::InfoLogger::endm;
  // Trigger beautification
  beautify(moMap);

  return mLatestQuality;
}

void Check::beautify(std::map<std::string, std::shared_ptr<MonitorObject>>& moMap)
{
  if (!mBeautify) {
    // Already checked - do not check again
    return;
  }
  if (!(moMap.size() == 1 && mMonitorObjectNames.size() == 1)) {
    // Do not beautify and check in future iterations
    mBeautify = false;
    return;
  }
  // Take first and only item from moMap
  auto& mo = moMap.begin()->second;

  // Beautify
  mLogger << mName << " Beautify" << AliceO2::InfoLogger::InfoLogger::endm;
  mCheckInterface->beautify(mo, mLatestQuality->getQuality());
}
