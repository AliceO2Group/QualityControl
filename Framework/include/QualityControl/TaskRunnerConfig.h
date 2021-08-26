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

///
/// \file   TaskRunnerConfig.h
/// \author Barthelemy von Haller
///

#ifndef QC_CORE_TASKCONFIG_H
#define QC_CORE_TASKCONFIG_H

#include <string>
#include <unordered_map>
#include <vector>

#include <Framework/DataProcessorSpec.h>

namespace o2::quality_control::core
{

/// \brief  Container for the configuration of a Task
struct TaskRunnerConfig {
  std::string deviceName;
  std::string taskName;
  std::string moduleName;
  std::string className;
  int cycleDurationSeconds;
  int maxNumberCycles;
  std::string consulUrl{};
  std::string conditionUrl{};
  std::string monitoringUrl{};
  framework::Inputs inputSpecs{};
  framework::OutputSpec moSpec{ "XXX", "INVALID" };
  framework::Options options{};
  std::unordered_map<std::string, std::string> customParameters = {};
  std::string detectorName = "MISC"; // intended to be the 3 letters code
  int parallelTaskID = 0;            // ID to differentiate parallel local Tasks from one another. 0 means this is the only one.
  std::string saveToFile{};
  int resetAfterCycles = 0;
  bool infologgerFilterDiscardDebug = false;
  int infologgerDiscardLevel = 21;
  int activityType = 0;
  std::string activityPeriodName = "";
  std::string activityPassName = "";
  std::string activityProvenance = "qc";
  int defaultRunNumber = 0;
  std::string configurationSource{};
};

} // namespace o2::quality_control::core

#endif // QC_CORE_TASKCONFIG_H
