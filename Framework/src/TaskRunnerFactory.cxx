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
#include "QualityControl/TimekeeperFactory.h"
#include "QualityControl/QcInfoLogger.h"

#include <Framework/DeviceSpec.h>
#include <Framework/CompletionPolicy.h>
#include <Headers/DataHeader.h>
#include <Framework/O2ControlLabels.h>
#include <Framework/DataProcessorLabel.h>
#include <Framework/DefaultsHelpers.h>
#include <DetectorsBase/GRPGeomHelper.h>
#include <DataFormatsGlobalTracking/RecoContainer.h>
#include <ReconstructionDataFormats/GlobalTrackID.h>

#include <Framework/TimerParamSpec.h>

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
  newTask.labels.emplace_back(TaskRunner::getTaskRunnerLabel());
  if (!taskConfig.critical) {
    framework::DataProcessorLabel expendableLabel = { "expendable" };
    newTask.labels.emplace_back(expendableLabel);
  }

  return newTask;
}

std::vector<std::pair<size_t, size_t>> TaskRunnerFactory::getSanitizedCycleDurations(const CommonSpec& globalConfig, const TaskSpec& taskSpec)
{
  // Two ways of configuring, incompatible.
  // 1. simple, old, way: cycleDurationSeconds is the duration in seconds for all cycles
  // 2. complex, new, way: cycleDurations: a list of tuples specifying different durations to be applied for a certain time

  // First make sure that we have one and only one cycle duration definition
  if (taskSpec.cycleDurationSeconds > 0 && taskSpec.multipleCycleDurations.size() > 0) {
    throw std::runtime_error("Both cycleDurationSeconds and cycleDurations have been defined for task '" + taskSpec.taskName + "'. Pick one. Sheepishly bailing out.");
  }
  if (taskSpec.cycleDurationSeconds <= 0 && taskSpec.multipleCycleDurations.size() == 0) {
    throw std::runtime_error("Neither cycleDurationSeconds nor cycleDurations have been defined for task '" + taskSpec.taskName + "'. Pick one. Sheepishly bailing out.");
  }

  // Convert old style into new style if needed
  std::vector<std::pair<size_t, size_t>> multipleCycleDurations = taskSpec.multipleCycleDurations; // this is the new style
  if (taskSpec.cycleDurationSeconds > 0) {                                                         // if it was actually the old style, then we convert it to the new style
    multipleCycleDurations = { { taskSpec.cycleDurationSeconds, 1 } };
  }

  // Check that the durations are not below 10 seconds except when using a dummy database
  auto dummyDatabaseUsed = globalConfig.database.count("implementation") > 0 && globalConfig.database.at("implementation") == "Dummy";
  if (!dummyDatabaseUsed) {
    for (auto& [cycleDuration, validity] : multipleCycleDurations) {
      if (cycleDuration < 10) {
        ILOG(Error, Support) << "Cycle duration is too short (" << cycleDuration << "), replaced by a duration of 10 seconds." << ENDM;
        cycleDuration = 10;
      }
    }
  }
  return multipleCycleDurations;
}

TaskRunnerConfig TaskRunnerFactory::extractConfig(const CommonSpec& globalConfig, const TaskSpec& taskSpec, std::optional<int> id, std::optional<int> resetAfterCycles)
{
  std::string deviceName{ TaskRunner::createTaskRunnerIdString() + "-" + InfrastructureSpecReader::validateDetectorName(taskSpec.detectorName) + "-" + taskSpec.taskName };

  int parallelTaskID = id.value_or(0);

  std::vector<InputSpec> inputs;
  for (const auto& ds : taskSpec.dataSources) {
    if (!ds.isOneOf(DataSourceType::DataSamplingPolicy, DataSourceType::Direct)) {
      throw std::runtime_error("This data source of the task '" + taskSpec.taskName + "' is not supported.");
    }
    inputs.insert(inputs.end(), ds.inputs.begin(), ds.inputs.end());
  }

  // cycle duration
  auto multipleCycleDurations = getSanitizedCycleDurations(globalConfig, taskSpec);
  inputs.emplace_back(createTimerInputSpec(globalConfig, multipleCycleDurations, taskSpec.detectorName, taskSpec.taskName));

  static std::unordered_map<std::string, o2::base::GRPGeomRequest::GeomRequest> const geomRequestFromString = {
    { "None", o2::base::GRPGeomRequest::GeomRequest::None },
    { "Aligned", o2::base::GRPGeomRequest::GeomRequest::Aligned },
    { "Ideal", o2::base::GRPGeomRequest::GeomRequest::Ideal },
    { "Alignments", o2::base::GRPGeomRequest::GeomRequest::Alignments }
  };
  auto grp = taskSpec.grpGeomRequestSpec;
  grp.askGRPECS |= TimekeeperFactory::needsGRPECS(DefaultsHelpers::deploymentMode());
  auto grpGeomRequest = grp.anyRequestEnabled()                                                               //
                          ? std::make_shared<o2::base::GRPGeomRequest>(                                       //
                              grp.askTime, grp.askGRPECS, grp.askGRPLHCIF, grp.askGRPMagField, grp.askMatLUT, //
                              geomRequestFromString.at(grp.geomRequest), inputs, grp.askOnceAllButField,      //
                              grp.needPropagatorD)                                                            //
                          : nullptr;

  const auto& dr = taskSpec.globalTrackingDataRequest;
  auto globalTrackingDataRequest = dr.requestTracks.empty() && dr.requestClusters.empty() ? nullptr : std::make_shared<o2::globaltracking::DataRequest>();
  if (globalTrackingDataRequest) {
    auto canProcessTracksMask = o2::dataformats::GlobalTrackID::getSourcesMask(dr.canProcessTracks);
    auto requestTracksMask = o2::dataformats::GlobalTrackID::getSourcesMask(dr.requestTracks);
    auto requestedTracksMask = canProcessTracksMask & requestTracksMask;
    globalTrackingDataRequest->requestTracks(requestedTracksMask, dr.mc);

    auto canProcessClustersMask = o2::dataformats::GlobalTrackID::getSourcesMask(dr.canProcessClusters);
    auto requestClustersMask = o2::dataformats::GlobalTrackID::getSourcesMask(dr.requestClusters);
    auto requestedClustersMask = canProcessClustersMask & requestClustersMask;
    globalTrackingDataRequest->requestTracks(requestedClustersMask, dr.mc);
    inputs.insert(inputs.begin(), globalTrackingDataRequest->inputs.begin(), globalTrackingDataRequest->inputs.end());
  }

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
    { globalConfig.activityStart, globalConfig.activityEnd },
    globalConfig.activityBeamType,
    globalConfig.activityPartitionName,
    globalConfig.activityFillNumber,
    globalConfig.activityOriginalNumber
  };

  o2::globaltracking::RecoContainer rd;

  return {
    taskSpec.taskName,
    taskSpec.moduleName,
    taskSpec.className,
    InfrastructureSpecReader::validateDetectorName(taskSpec.detectorName),
    globalConfig.consulUrl,
    taskSpec.customParameters,
    globalConfig.conditionDBUrl,
    globalConfig.database,
    taskSpec.dataSources,
    deviceName,
    multipleCycleDurations,
    taskSpec.maxNumberCycles,
    taskSpec.critical,
    globalConfig.monitoringUrl,
    globalConfig.bookkeepingUrl,
    inputs,
    monitorObjectsSpec,
    options,
    parallelTaskID,
    taskSpec.saveObjectsToFile,
    resetAfterCycles.value_or(taskSpec.resetAfterCycles),
    globalConfig.infologgerDiscardParameters,
    fallbackActivity,
    grpGeomRequest,
    globalTrackingDataRequest,
    taskSpec.movingWindows,
    taskSpec.disableLastCycle,
  };
}

InputSpec TaskRunnerFactory::createTimerInputSpec(const CommonSpec& globalConfig, std::vector<std::pair<size_t, size_t>>& cycleDurations,
                                                  const std::string& detectorName, const std::string& taskName)
{
  // Create the TimerSpec for cycleDurations
  std::vector<TimerSpec> timers;
  for (auto& [cycleDuration, period] : cycleDurations) {
    timers.push_back({ cycleDuration * 1000000000 /*µs*/, period });
  }

  return { "timer-cycle",
           TaskRunner::createTaskDataOrigin(detectorName),
           TaskRunner::createTimerDataDescription(taskName),
           0,
           Lifetime::Timer,
           timerSpecs(timers) };
}

void TaskRunnerFactory::customizeInfrastructure(std::vector<framework::CompletionPolicy>& policies)
{
  auto matcher = [label = TaskRunner::getTaskRunnerLabel()](auto const& device) {
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
