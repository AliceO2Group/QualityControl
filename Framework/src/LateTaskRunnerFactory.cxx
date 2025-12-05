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

#include "QualityControl/ActorHelpers.h"
#include "QualityControl/LateTaskRunner.h"
#include "QualityControl/LateTaskSpec.h"
#include "QualityControl/CommonSpec.h"
#include "QualityControl/InfrastructureSpecReader.h"
#include "QualityControl/DataProcessorAdapter.h"

#include <Framework/CompletionPolicyHelpers.h>
#include <Framework/DeviceSpec.h>



using namespace o2::framework;

namespace o2::quality_control::core {

o2::framework::DataProcessorSpec LateTaskRunnerFactory::create(const ServicesConfig& ServicesConfig, const LateTaskConfig& taskConfig)
{
  auto dataProcessorName = actor_helpers::dataProcessorName<LateTaskRunner>(taskConfig.name, taskConfig.detectorName);
  auto inputs = actor_helpers::collectUserInputs<LateTaskRunner>(taskConfig);
  auto outputs = actor_helpers::collectUserOutputs<LateTaskRunner, DataSourceType::LateTask>(taskConfig);

  LateTaskRunner task(ServicesConfig, taskConfig);
  return DataProcessorAdapter::adapt<LateTaskRunner>(std::move(task), std::move(dataProcessorName), std::move(inputs), std::move(outputs), Options{});
}

LateTaskConfig LateTaskRunnerFactory::extractConfig(const CommonSpec& commonSpec, const LateTaskSpec& lateTaskSpec)
{

  return LateTaskConfig{
    {
      .name = lateTaskSpec.taskName,
      .moduleName = lateTaskSpec.moduleName,
      .className = lateTaskSpec.className,
      .detectorName = lateTaskSpec.detectorName,
      .consulUrl = commonSpec.consulUrl,
      .customParameters = lateTaskSpec.customParameters,
      .ccdbUrl = commonSpec.conditionDBUrl,
      .repository = commonSpec.database,
      .dataSources = lateTaskSpec.dataSources
    },
    lateTaskSpec.critical
  };
}

void LateTaskRunnerFactory::customizeInfrastructure(std::vector<framework::CompletionPolicy>& policies)
{
  auto matcher = [label = actor_helpers::dataProcessorLabel<LateTaskRunner>()](auto const& device) {
    return std::find(device.labels.begin(), device.labels.end(), label) != device.labels.end();
  };
  policies.emplace_back(CompletionPolicyHelpers::consumeWhenAny("lateTasksCompletionPolicy", matcher));
}

}

