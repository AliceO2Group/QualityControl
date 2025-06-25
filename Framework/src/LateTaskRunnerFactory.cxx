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
/// \file   LateTaskRunnerFactory.cxx
/// \author Piotr Konopka
///

#include "QualityControl/LateTaskRunnerFactory.h"

#include "QualityControl/LateTaskRunner.h"
#include "QualityControl/LateTaskSpec.h"
#include "QualityControl/CommonSpec.h"
#include "QualityControl/InfrastructureSpecReader.h"

#include <Framework/CompletionPolicyHelpers.h>
#include <Framework/DeviceSpec.h>


using namespace o2::framework;

namespace o2::quality_control::core
{

o2::framework::DataProcessorSpec LateTaskRunnerFactory::create(const LateTaskRunnerConfig& config)
{
  LateTaskRunner qcTask{ config };

  DataProcessorSpec newTask{
    config.deviceName,
    config.inputSpecs,
    { config.moSpec },
    adaptFromTask<LateTaskRunner>(std::move(qcTask)),
    config.options
  };
  newTask.labels.emplace_back(LateTaskRunner::getLabel());
  if (!config.critical) {
    framework::DataProcessorLabel expendableLabel = { "expendable" };
    newTask.labels.emplace_back(expendableLabel);
  }

  return newTask;
}

LateTaskRunnerConfig LateTaskRunnerFactory::extractConfig(const CommonSpec& commonSpec, const LateTaskSpec& lateTaskSpec)
{
  // todo: this could be generalized
  std::string deviceName{ LateTaskRunner::createIdString() + "-" + InfrastructureSpecReader::validateDetectorName(lateTaskSpec.detectorName) + "-" + lateTaskSpec.taskName };

  // todo: this could generalized
  std::vector<InputSpec> inputs;
  for (const auto& ds : lateTaskSpec.dataSources) {
    if (!ds.isOneOf(DataSourceType::Task, DataSourceType::TaskMovingWindow, DataSourceType::Check, DataSourceType::Aggregator)) {
      throw std::runtime_error("This data source of the task '" + lateTaskSpec.taskName + "' is not supported.");
    }
    inputs.insert(inputs.end(), ds.inputs.begin(), ds.inputs.end());
  }

  OutputSpec monitorObjectsSpec{ { "mo" },
                               LateTaskRunner::createDataOrigin(lateTaskSpec.detectorName),
                               LateTaskRunner::createDataDescription(lateTaskSpec.taskName),
                               0,
                               Lifetime::Sporadic };

  return LateTaskRunnerConfig{
    {
      .moduleName = lateTaskSpec.moduleName,
      .className = lateTaskSpec.className,
      .detectorName = lateTaskSpec.detectorName,
      .consulUrl = commonSpec.consulUrl,
      .customParameters = lateTaskSpec.customParameters,
      .ccdbUrl = commonSpec.conditionDBUrl,
      .repository = commonSpec.database,
    },
    lateTaskSpec.taskName,
    deviceName,
    inputs,
    monitorObjectsSpec,
    {},
    lateTaskSpec.critical
  };
}

void LateTaskRunnerFactory::customizeInfrastructure(std::vector<framework::CompletionPolicy>& policies)
{
  auto matcher = [label = LateTaskRunner::getLabel()](auto const& device) {
    return std::find(device.labels.begin(), device.labels.end(), label) != device.labels.end();
  };
  policies.emplace_back(CompletionPolicyHelpers::consumeWhenAny("lateTasksCompletionPolicy", matcher));
}

}

