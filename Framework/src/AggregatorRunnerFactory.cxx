// Copyright CERN and copyright holders of ALICE O2. This software is
// distributed under the terms of the GNU General Public License v3 (GPL
// Version 3), copied verbatim in the file "COPYING".
//
// See http://alice-o2.web.cern.ch/license for full licensing information.
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

DataProcessorSpec AggregatorRunnerFactory::create(const vector<OutputSpec>& checkerRunnerOutputs, const std::string& configurationSource)
{
  AggregatorRunner aggregator{ configurationSource, checkerRunnerOutputs };

  // build DataProcessorSpec
  DataProcessorSpec aggregatorSpec{
    aggregator.getDeviceName(),
    aggregator.getInputs(),
    Outputs{ aggregator.getOutput() },
    adaptFromTask<AggregatorRunner>(std::move(aggregator)),
    Options{}
  };

  return aggregatorSpec;
}

// Specify a custom policy to trigger whenever something arrive regardless of the timeslice.
void AggregatorRunnerFactory::customizeInfrastructure(std::vector<framework::CompletionPolicy>& policies)
{
  auto matcher = [](framework::DeviceSpec const& device) {
    return device.name.find(AggregatorRunner::createAggregatorRunnerIdString()) != std::string::npos;
  };
  auto callback = CompletionPolicyHelpers::consumeWhenAny().callback;

  policies.emplace_back("aggregatorRunnerCompletionPolicy", matcher, callback);
}

} // namespace o2::quality_control::checker
