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
/// \file   AggregatorRunnerFactory.cxx
/// \author Barthelemy von Haller
///

#include <utility>
#include <vector>

#include <Framework/DataProcessorSpec.h>
#include <Framework/DeviceSpec.h>
#include <Framework/DataSpecUtils.h>
#include <Framework/CompletionPolicyHelpers.h>

#include "QualityControl/AggregatorRunner.h"
#include "QualityControl/AggregatorRunnerFactory.h"

using namespace std;
using namespace o2::framework;

namespace o2::quality_control::checker
{

DataProcessorSpec AggregatorRunnerFactory::create(AggregatorRunnerConfig arc, std::vector<AggregatorConfig> acs)
{
  AggregatorRunner aggregator{ std::move(arc), std::move(acs) };

  DataProcessorSpec newAggregatorRunner{
    aggregator.getDeviceName(),
    aggregator.getInputs(),
    Outputs{},
    AlgorithmSpec{},
    Options{}
  };
  newAggregatorRunner.labels.emplace_back(AggregatorRunner::getLabel());
  newAggregatorRunner.algorithm = adaptFromTask<AggregatorRunner>(std::move(aggregator));
  return newAggregatorRunner;
}

// Specify a custom policy to trigger whenever something arrive regardless of the timeslice.
void AggregatorRunnerFactory::customizeInfrastructure(std::vector<framework::CompletionPolicy>& policies)
{
  auto matcher = [label = AggregatorRunner::getLabel()](framework::DeviceSpec const& device) {
    return std::find(device.labels.begin(), device.labels.end(), label) != device.labels.end();
  };
  auto callback = CompletionPolicyHelpers::consumeWhenAny().callback;

  policies.emplace_back("aggregatorRunnerCompletionPolicy", matcher, callback);
}

AggregatorRunnerConfig AggregatorRunnerFactory::extractConfig(const core::CommonSpec& commonSpec)
{
  return {
    commonSpec.database,
    commonSpec.consulUrl,
    commonSpec.monitoringUrl,
    commonSpec.infologgerFilterDiscardDebug,
    commonSpec.infologgerDiscardLevel,
    commonSpec.activityNumber,
    commonSpec.activityPeriodName,
    commonSpec.activityPassName,
    commonSpec.activityProvenance
  };
}

} // namespace o2::quality_control::checker
