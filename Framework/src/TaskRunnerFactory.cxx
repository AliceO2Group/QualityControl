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
/// \file   TaskRunnerFactory.cxx
/// \author Piotr Konopka
///

#include "QualityControl/CommonSpec.h"
#include "QualityControl/TaskRunnerFactory.h"
#include "QualityControl/TaskRunner.h"
#include "QualityControl/TaskRunnerConfig.h"
#include "QualityControl/TaskSpec.h"
#include "QualityControl/InfrastructureSpecReader.h"
#include "QualityControl/QcInfoLogger.h"

#include <Framework/DeviceSpec.h>
#include <Framework/CompletionPolicy.h>
#include <Headers/DataHeader.h>
#include <Framework/O2ControlLabels.h>
#include <Framework/DataProcessorLabel.h>
#include <DetectorsBase/GRPGeomHelper.h>

namespace o2::quality_control::core
{

using namespace o2::framework;

o2::framework::DataProcessorSpec TaskRunnerFactory::create(const TaskRunnerConfig& taskConfig)
{
  TaskRunner qcTask{ taskConfig };

  DataProcessorSpec newTask{
    taskConfig.deviceName,
    taskConfig.inputSpecs,
    { taskConfig.moSpec },
    adaptFromTask<TaskRunner>(std::move(qcTask)),
    taskConfig.options
  };
  newTask.labels.emplace_back(o2::framework::ecs::qcReconfigurable);
  newTask.labels.emplace_back(TaskRunner::getTaskRunnerLabel());

  return newTask;
}

TaskRunnerConfig TaskRunnerFactory::extractConfig(const CommonSpec& globalConfig, const TaskSpec& taskSpec, std::optional<int> id, std::optional<int> resetAfterCycles)
{
  std::string deviceName{ TaskRunner::createTaskRunnerIdString() + "-" + InfrastructureSpecReader::validateDetectorName(taskSpec.detectorName) + "-" + taskSpec.taskName };

  int parallelTaskID = id.value_or(0);

  if (!taskSpec.dataSource.isOneOf(DataSourceType::DataSamplingPolicy, DataSourceType::Direct)) {
    throw std::runtime_error("This data source of the task '" + taskSpec.taskName + "' is not supported.");
  }
  auto cycleDurationSeconds = taskSpec.cycleDurationSeconds;
  auto dummyDatabaseUsed = globalConfig.database.count("implementation") > 0 && globalConfig.database.at("implementation") == "Dummy";
  if (!dummyDatabaseUsed && cycleDurationSeconds < 10) {
    ILOG(Error, Support) << "Cycle duration is too short (" << cycleDurationSeconds << "), replaced by a duration of 10 seconds." << ENDM;
    cycleDurationSeconds = 10;
  }
  auto inputs = taskSpec.dataSource.inputs;
  inputs.emplace_back("timer-cycle",
                      TaskRunner::createTaskDataOrigin(taskSpec.detectorName),
                      TaskRunner::createTimerDataDescription(taskSpec.taskName),
                      0,
                      Lifetime::Timer);

  static std::unordered_map<std::string, o2::base::GRPGeomRequest::GeomRequest> const geomRequestFromString = {
    { "None", o2::base::GRPGeomRequest::GeomRequest::None },
    { "Aligned", o2::base::GRPGeomRequest::GeomRequest::Aligned },
    { "Ideal", o2::base::GRPGeomRequest::GeomRequest::Ideal },
    { "Alignments", o2::base::GRPGeomRequest::GeomRequest::Alignments }
  };
  const auto& grp = taskSpec.grpGeomRequestSpec;
  auto grpGeomRequest = grp.anyRequestEnabled()                                                               //
                          ? nullptr                                                                           //
                          : std::make_shared<o2::base::GRPGeomRequest>(                                       //
                              grp.askTime, grp.askGRPECS, grp.askGRPLHCIF, grp.askGRPMagField, grp.askMatLUT, //
                              geomRequestFromString.at(grp.geomRequest), inputs, grp.askOnceAllButField, grp.needPropagatorD);

  OutputSpec monitorObjectsSpec{ { "mo" },
                                 TaskRunner::createTaskDataOrigin(taskSpec.detectorName),
                                 TaskRunner::createTaskDataDescription(taskSpec.taskName),
                                 static_cast<header::DataHeader::SubSpecificationType>(parallelTaskID),
                                 Lifetime::Sporadic };

  Options options{
    { "period-timer-cycle", framework::VariantType::Int, static_cast<int>(taskSpec.cycleDurationSeconds * 1000000), { "timer period" } },
    { "runNumber", framework::VariantType::String, { "Run number" } },
    { "qcConfiguration", VariantType::Dict, emptyDict(), { "Some dictionary configuration" } }
  };

  Activity fallbackActivity{
    globalConfig.activityNumber,
    globalConfig.activityType,
    globalConfig.activityPeriodName,
    globalConfig.activityPassName,
    globalConfig.activityProvenance,
    { globalConfig.activityStart, globalConfig.activityEnd }
  };

  return {
    deviceName,
    taskSpec.taskName,
    taskSpec.moduleName,
    taskSpec.className,
    cycleDurationSeconds,
    taskSpec.maxNumberCycles,
    globalConfig.consulUrl,
    globalConfig.conditionDBUrl,
    globalConfig.monitoringUrl,
    inputs,
    monitorObjectsSpec,
    options,
    taskSpec.customParameters,
    InfrastructureSpecReader::validateDetectorName(taskSpec.detectorName),
    parallelTaskID,
    taskSpec.saveObjectsToFile,
    resetAfterCycles.value_or(taskSpec.resetAfterCycles),
    globalConfig.infologgerDiscardParameters,
    fallbackActivity,
    grpGeomRequest
  };
}

void TaskRunnerFactory::customizeInfrastructure(std::vector<framework::CompletionPolicy>& policies)
{
  auto matcher = [label = TaskRunner::getTaskRunnerLabel()](framework::DeviceSpec const& device) {
    return std::find(device.labels.begin(), device.labels.end(), label) != device.labels.end();
  };
  auto callback = TaskRunner::completionPolicyCallback;

  framework::CompletionPolicy taskRunnerCompletionPolicy{ "taskRunnerCompletionPolicy", matcher, callback };
  policies.push_back(taskRunnerCompletionPolicy);
}

int TaskRunnerFactory::computeResetAfterCycles(const TaskSpec& taskSpec, bool runningWithMergers)
{
  return (runningWithMergers && taskSpec.mergingMode == "delta") ? 1 : (int)taskSpec.resetAfterCycles;
}

} // namespace o2::quality_control::core
