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
/// \file   CheckRunnerFactory.cxx
/// \author Piotr Konopka
///
#include <vector>
#include <unordered_set>

#include <Framework/DataProcessorSpec.h>
#include <Framework/DeviceSpec.h>
#include <Framework/DataProcessorSpec.h>
#include <Framework/DataSpecUtils.h>
#include <Framework/CompletionPolicyHelpers.h>
#include <Framework/O2ControlLabels.h>

#include "QualityControl/CheckRunner.h"
#include "QualityControl/CheckRunnerFactory.h"
#include "QualityControl/CommonSpec.h"

namespace o2::quality_control::checker
{

using namespace o2::framework;

DataProcessorSpec CheckRunnerFactory::create(CheckRunnerConfig checkRunnerConfig, const std::vector<CheckConfig>& checkConfigs, std::vector<std::string> storeVector)
{
  auto options = checkRunnerConfig.options;
  CheckRunner qcCheckRunner{ std::move(checkRunnerConfig), checkConfigs };
  qcCheckRunner.setTaskStoreSet({ storeVector.begin(), storeVector.end() });

  DataProcessorSpec newCheckRunner{ qcCheckRunner.getDeviceName(),
                                    qcCheckRunner.getInputs(),
                                    Outputs{ qcCheckRunner.getOutputs() },
                                    AlgorithmSpec{},
                                    options };
  newCheckRunner.labels.emplace_back(o2::framework::ecs::qcReconfigurable);
  newCheckRunner.labels.emplace_back(CheckRunner::getCheckRunnerLabel());
  newCheckRunner.algorithm = adaptFromTask<CheckRunner>(std::move(qcCheckRunner));
  return newCheckRunner;
}

DataProcessorSpec CheckRunnerFactory::createSinkDevice(const CheckRunnerConfig& checkRunnerConfig, const o2::framework::InputSpec& input)
{
  CheckRunner qcCheckRunner{ checkRunnerConfig, input };
  qcCheckRunner.setTaskStoreSet({ DataSpecUtils::label(input) });

  DataProcessorSpec newCheckRunner{ qcCheckRunner.getDeviceName(),
                                    qcCheckRunner.getInputs(),
                                    Outputs{ qcCheckRunner.getOutputs() },
                                    adaptFromTask<CheckRunner>(std::move(qcCheckRunner)),
                                    checkRunnerConfig.options,
                                    {},
                                    { o2::framework::ecs::qcReconfigurable } };

  return newCheckRunner;
}

void CheckRunnerFactory::customizeInfrastructure(std::vector<framework::CompletionPolicy>& policies)
{
  auto matcher = [label = CheckRunner::getCheckRunnerLabel()](framework::DeviceSpec const& device) {
    return std::find(device.labels.begin(), device.labels.end(), label) != device.labels.end();
  };
  policies.emplace_back(CompletionPolicyHelpers::consumeWhenAny("checkerCompletionPolicy", matcher));
}

CheckRunnerConfig CheckRunnerFactory::extractConfig(const CommonSpec& commonSpec)
{
  Options options{
    { "runNumber", framework::VariantType::String, { "Run number" } },
    { "qcConfiguration", VariantType::Dict, emptyDict(), { "Some dictionary configuration" } }
  };

  core::Activity fallbackActivity{
    commonSpec.activityNumber,
    commonSpec.activityType,
    commonSpec.activityPeriodName,
    commonSpec.activityPassName,
    commonSpec.activityProvenance,
    { commonSpec.activityStart, commonSpec.activityEnd }
  };
  return {
    commonSpec.database,
    commonSpec.consulUrl,
    commonSpec.monitoringUrl,
    commonSpec.bookkeepingUrl,
    commonSpec.infologgerDiscardParameters,
    fallbackActivity,
    options
  };
}

} // namespace o2::quality_control::checker
