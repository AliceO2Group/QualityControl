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

DataProcessorSpec AggregatorRunnerFactory::create(const vector<OutputSpec>& checkerRunnerOutputs, std::string configurationSource)
{
  AggregatorRunner aggregator{ std::move(configurationSource), checkerRunnerOutputs };

  // build DataProcessorSpec
  DataProcessorSpec aggregatorSpec{
    aggregator.getDeviceName(),
    aggregator.getInputs(),
    Outputs{ aggregator.getOutput() },
    AlgorithmSpec{},
    Options{}
  };
  cout << "input : " << DataSpecUtils::describe(aggregator.getInputs().at(0)) << endl;
  cout << "checkerRunnerOutputs : " << DataSpecUtils::describe(checkerRunnerOutputs.at(0)) << endl;
  cout << "input validate : " << DataSpecUtils::validate(aggregator.getInputs().at(0)) << endl;
  cout << "checkerRunnerOutputs validate  : " << DataSpecUtils::validate(checkerRunnerOutputs.at(0)) << endl;
  cout << "input spec.binding.empty() : " << aggregator.getInputs().at(0).binding.empty() << endl;
  cout << "input binding : " << aggregator.getInputs().at(0).binding << endl;
  cout << "A" << endl;
  aggregatorSpec.algorithm = adaptFromTask<AggregatorRunner>(std::move(aggregator));
  cout << "B" << endl;

  return aggregatorSpec;
}

// TODO what is that ??
void AggregatorRunnerFactory::customizeInfrastructure(std::vector<framework::CompletionPolicy>& policies)
{
  auto matcher = [](framework::DeviceSpec const& device) {
    return device.name.find(AggregatorRunner::createAggregatorRunnerIdString()) != std::string::npos;
  };
  auto callback = CompletionPolicyHelpers::consumeWhenAny().callback;

  policies.emplace_back("aggregatorRunnerCompletionPolicy", matcher, callback);
}

} // namespace o2::quality_control::checker
