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

  // todo validate data source
  if (!taskSpec.dataSource.isOneOf(DataSourceType::DataSamplingPolicy, DataSourceType::Direct)) {
    throw std::runtime_error("This data source of the task '" + taskSpec.taskName + "' is not supported.");
  }
  auto cycleDurationSeconds = taskSpec.cycleDurationSeconds;
  if (cycleDurationSeconds < 10) {
    ILOG(Error, Support) << "Cycle duration is too short (" << cycleDurationSeconds << "), replaced by a duration of 10 seconds." << ENDM;
    cycleDurationSeconds = 10;
  }
  auto inputs = taskSpec.dataSource.inputs;
  inputs.emplace_back("timer-cycle",
                      TaskRunner::createTaskDataOrigin(taskSpec.detectorName),
                      TaskRunner::createTimerDataDescription(taskSpec.taskName),
                      0,
                      Lifetime::Timer);

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
    globalConfig.infologgerFilterDiscardDebug,
    globalConfig.infologgerDiscardLevel,
    globalConfig.infologgerDiscardFile,
    fallbackActivity
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

bool TaskRunnerFactory::computeResetAfterCycles(const TaskSpec& taskSpec)
{
  return taskSpec.mergingMode == "delta" ? 1 : (int)taskSpec.resetAfterCycles;
}

} // namespace o2::quality_control::core
