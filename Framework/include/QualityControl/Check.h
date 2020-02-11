// Copyright CERN and copyright holders of ALICE O2. This software is
// distributed under the terms of the GNU General Public License v3 (GPL
// Version 3), copied verbatim in the file "COPYING".
//
// See http://alice-o2.web.cern.ch/license for full licensing information.
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.

#ifndef QC_CHECKER_CHECK_H
#define QC_CHECKER_CHECK_H

// std
#include <functional>
#include <map>
#include <vector>
#include <memory>
// O2
#include <Framework/DataProcessorSpec.h>
// QC
#include "QualityControl/Quality.h"
#include "QualityControl/QualityObject.h"
#include "QualityControl/MonitorObject.h"
#include "QualityControl/CheckInterface.h"
#include "QualityControl/QcInfoLogger.h"

namespace o2::quality_control::checker
{

/// \brief The class in charge of providing single check for a given map of MonitorObjects.
///
/// A Check is in charge of loading/instantiating the single check from module, configure them and manage the check process:
/// tell whenever the check policy is fulfilled, shadow not required MonitorObjects, invoke beautify process if needed.
///
/// \author Rafal Pacholek
class Check
{
 public:
  /// Constructor
  /**
   * \brief Check constructor
   *
   * Create Check that will load single check from a module and run if invoked.
   *
   * @param checkName Check name from the configuration
   * @param configurationSource Path to configuration
   */
  Check(std::string checkName, std::string configurationSource);

  /**
   * \brief Initialize the check state
   * Expected to run in the init phase of the FairDevice
   */
  void init();

  std::shared_ptr<o2::quality_control::core::QualityObject> check(std::map<std::string, std::shared_ptr<o2::quality_control::core::MonitorObject>>& moMap);

  // Policy
  /**
   * \brief Change the revision.
   *
   * Update the revision number with the latest global revision.
   * Expected to be changed after invoke of `check(moMap)` or revision number overflow.
   */
  void updateRevision(unsigned int revision);

  /**
   * \brief Return true if the Monitor Objects were changed accordingly to the policy
   */
  bool isReady(std::map<std::string, unsigned int>& revisionMap);

  const std::string getName() { return mName; };
  std::shared_ptr<o2::quality_control::core::QualityObject> getQualityObject() { return mQualityObject; };
  o2::framework::OutputSpec getOutputSpec() const { return mOutputSpec; };
  o2::framework::Inputs getInputs() const { return mInputs; };
  std::vector<std::string> getMonitorObjectNames() { return mMonitorObjectNames; }
  bool isBeautified() { return mBeautify; }

  //TODO: Unique Input string
  static o2::header::DataDescription createCheckerDataDescription(const std::string taskName);

  // For testing purpose
  void setCheckInterface(CheckInterface* checkInterface) { mCheckInterface = checkInterface; };

 private:
  void initConfig();
  void initPolicy(std::string policyType);

  /**
   * \brief Load a library.
   * Load a library if it is not already in the cache.
   */
  void loadLibrary();

  void beautify(std::map<std::string, std::shared_ptr<MonitorObject>>& moMap);

  const std::string mName;

  std::string mConfigurationSource;
  o2::quality_control::core::QcInfoLogger& mLogger;

  // Latest Quality
  std::shared_ptr<o2::quality_control::core::QualityObject> mQualityObject;

  // DPL information
  o2::framework::Inputs mInputs;
  o2::framework::OutputSpec mOutputSpec;

  // Monitor Object information
  std::vector<std::string> mMonitorObjectNames;
  bool mAllMOs = false;
  bool mBeautify = true;

  // Check module
  std::string mModuleName;
  std::string mClassName;
  CheckInterface* mCheckInterface = nullptr;
  std::unordered_map<std::string, std::string> mCustomParameters;

  // Policy
  std::string mPolicyType;
  std::function<bool(std::map<std::string, unsigned int>&)> mPolicy;
  unsigned int mMORevision = 0;
  bool mPolicyHelper = false; // Depending on policy, the purpose might change
};

} // namespace o2::quality_control::checker

#endif
