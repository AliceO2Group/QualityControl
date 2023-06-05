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

#ifndef QC_CHECKER_CHECK_H
#define QC_CHECKER_CHECK_H

// std
#include <map>
#include <vector>
#include <memory>
// O2
#include <Framework/DataProcessorSpec.h>
// QC
#include "QualityControl/QualityObject.h"
#include "QualityControl/CheckConfig.h"

namespace o2::quality_control::core
{
class MonitorObject;
class Quality;
struct CommonSpec;
class Activity;
} // namespace o2::quality_control::core

namespace o2::quality_control::checker
{
class CheckInterface;
struct CheckSpec;

/// \brief The class in charge of providing single check for a given map of MonitorObjects.
///
/// A Check is in charge of loading/instantiating the single check from module, configure them and manage the check process:
/// shadow not required MonitorObjects, invoke  beautify process if needed.
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
  Check(CheckConfig config);

  /**
   * \brief Initialize the check state
   * Expected to run in the init phase of the FairDevice
   */
  void init();

  core::QualityObjectsType check(std::map<std::string, std::shared_ptr<o2::quality_control::core::MonitorObject>>& moMap);

  const std::string& getName() const { return mCheckConfig.name; };
  o2::framework::OutputSpec getOutputSpec() const { return mCheckConfig.qoSpec; };
  o2::framework::Inputs getInputs() const { return mCheckConfig.inputSpecs; };
  const std::string& getDetector() const { return mCheckConfig.detectorName; };
  const CheckConfig& getConfig() const { return mCheckConfig; };
  void setActivity(std::shared_ptr<core::Activity> activity);

  //TODO: Unique Input string
  static o2::header::DataDescription createCheckDataDescription(const std::string& checkName);

  UpdatePolicyType getUpdatePolicyType() const;
  std::vector<std::string> getObjectsNames() const;
  bool getAllObjectsOption() const;

  // todo: probably make CheckFactory
  static CheckConfig extractConfig(const core::CommonSpec&, const CheckSpec&);
  static framework::OutputSpec createOutputSpec(const std::string& checkName);

 private:
  void beautify(std::map<std::string, std::shared_ptr<core::MonitorObject>>& moMap, const core::Quality& quality);

  CheckConfig mCheckConfig;
  CheckInterface* mCheckInterface = nullptr;
};

} // namespace o2::quality_control::checker

#endif
