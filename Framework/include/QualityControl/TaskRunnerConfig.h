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
#include <memory>

#include <Framework/DataProcessorSpec.h>
#include "QualityControl/Activity.h"
#include "QualityControl/DiscardFileParameters.h"
#include "QualityControl/CustomParameters.h"

namespace o2::base
{
class GRPGeomRequest;
}

namespace o2::globaltracking
{
struct DataRequest;
}

namespace o2::quality_control::core
{

/// \brief  Container for the configuration of a Task
struct TaskRunnerConfig {
  std::string deviceName;
  std::string taskName;
  std::string moduleName;
  std::string className;
  std::vector<std::pair<size_t, size_t>> cycleDurations = {};
  int maxNumberCycles;
  std::string consulUrl{};
  std::string conditionUrl{};
  std::string monitoringUrl{};
  std::string bookkeepingUrl{};
  framework::Inputs inputSpecs{};
  framework::OutputSpec moSpec{ "XXX", "INVALID" };
  framework::Options options{};
  CustomParameters customParameters;
  std::string detectorName = "MISC"; // intended to be the 3 letters code
  int parallelTaskID = 0;            // ID to differentiate parallel local Tasks from one another. 0 means this is the only one.
  std::string saveToFile{};
  int resetAfterCycles = 0;
  core::DiscardFileParameters infologgerDiscardParameters;
  Activity fallbackActivity;
  std::shared_ptr<o2::base::GRPGeomRequest> grpGeomRequest;
  std::shared_ptr<o2::globaltracking::DataRequest> globalTrackingDataRequest;
};

} // namespace o2::quality_control::core

#endif // QC_CORE_TASKCONFIG_H
