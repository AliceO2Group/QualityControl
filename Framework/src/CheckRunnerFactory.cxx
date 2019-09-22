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
/// \file   CheckRunnerFactory.cxx
/// \author Piotr Konopka
///

#include <Framework/DataProcessorSpec.h>
#include <Framework/DeviceSpec.h>
#include <Framework/DataProcessorSpec.h>

#include "QualityControl/CheckRunner.h"
#include "QualityControl/CheckRunnerFactory.h"

namespace o2::quality_control::checker
{

using namespace o2::framework;
using namespace o2::quality_control::checker;

DataProcessorSpec CheckRunnerFactory::create(Check check, std::string configurationSource)
{
  CheckRunner qcCheckRunner{ check, configurationSource };

  DataProcessorSpec newCheckRunner{ qcCheckRunner.getDeviceName(),
                                    qcCheckRunner.getInputs(),
                                    Outputs{ qcCheckRunner.getOutputs() },
                                    adaptFromTask<CheckRunner>(std::move(qcCheckRunner)),
                                    Options{},
                                    std::vector<std::string>{},
                                    std::vector<DataProcessorLabel>{} };

  return newCheckRunner;
}

DataProcessorSpec CheckRunnerFactory::create(std::vector<Check> checks, std::string configurationSource)
{
  CheckRunner qcCheckRunner{ checks, configurationSource };

  DataProcessorSpec newCheckRunner{ qcCheckRunner.getDeviceName(),
                                    qcCheckRunner.getInputs(),
                                    Outputs{ qcCheckRunner.getOutputs() },
                                    adaptFromTask<CheckRunner>(std::move(qcCheckRunner)),
                                    Options{},
                                    std::vector<std::string>{},
                                    std::vector<DataProcessorLabel>{} };

  return newCheckRunner;
}

void CheckRunnerFactory::customizeInfrastructure(std::vector<framework::CompletionPolicy>& policies)
{
  auto matcher = [](framework::DeviceSpec const& device) {
    return device.name.find(CheckRunner::createCheckRunnerIdString()) != std::string::npos;
  };
  auto callback = [](gsl::span<PartRef const> const& inputs) {
    // TODO: Check if need to check nullptr (done in checker::run)
    for (auto& input : inputs) {
      if (!(input.header == nullptr || input.payload == nullptr)) {
        return framework::CompletionPolicy::CompletionOp::Consume;
      }
    }
    return framework::CompletionPolicy::CompletionOp::Wait;
  };

  framework::CompletionPolicy checkerCompletionPolicy{ "checkerCompletionPolicy", matcher, callback };
  policies.push_back(checkerCompletionPolicy);
}

} // namespace o2::quality_control::checker
