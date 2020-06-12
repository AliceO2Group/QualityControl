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
#include <vector>
#include <unordered_set>

#include <Framework/DataProcessorSpec.h>
#include <Framework/DeviceSpec.h>
#include <Framework/DataProcessorSpec.h>
#include <Framework/DataSpecUtils.h>
#include <Framework/CompletionPolicyHelpers.h>

#include "QualityControl/CheckRunner.h"
#include "QualityControl/CheckRunnerFactory.h"

namespace o2::quality_control::checker
{

using namespace o2::framework;
using namespace o2::quality_control::checker;

DataProcessorSpec CheckRunnerFactory::create(std::vector<Check> checks, std::string configurationSource, std::vector<std::string> storeVector)
{
  CheckRunner qcCheckRunner{ checks, configurationSource };
  qcCheckRunner.setTaskStoreSet({ storeVector.begin(), storeVector.end() });

  DataProcessorSpec newCheckRunner{ qcCheckRunner.getDeviceName(),
                                    qcCheckRunner.getInputs(),
                                    Outputs{ qcCheckRunner.getOutputs() },
                                    adaptFromTask<CheckRunner>(std::move(qcCheckRunner)),
                                    Options{},
                                    std::vector<std::string>{},
                                    std::vector<DataProcessorLabel>{} };

  return newCheckRunner;
}

DataProcessorSpec CheckRunnerFactory::createSinkDevice(o2::framework::InputSpec input, std::string configurationSource)
{
  CheckRunner qcCheckRunner{ input, configurationSource };
  qcCheckRunner.setTaskStoreSet({ DataSpecUtils::label(input) });

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
  auto callback = CompletionPolicyHelpers::consumeWhenAny().callback;

  policies.emplace_back("checkerCompletionPolicy", matcher, callback);
}

} // namespace o2::quality_control::checker
