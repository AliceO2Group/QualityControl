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
/// \file   CheckerFactory.cxx
/// \author Piotr Konopka
///

#include <Framework/DataProcessorSpec.h>
#include <Framework/DeviceSpec.h>
#include <Framework/DataProcessorSpec.h>

#include "QualityControl/CheckerFactory.h"
#include "QualityControl/Checker.h"

namespace o2::quality_control::checker
{

using namespace o2::framework;
using namespace o2::quality_control::checker;

DataProcessorSpec CheckerFactory::create(Check check, std::string configurationSource)
{
  Checker qcChecker{ check, configurationSource };

  DataProcessorSpec newChecker{ qcChecker.getDeviceName(),
                                qcChecker.getInputs(),
                                Outputs{ qcChecker.getOutputSpec() },
                                adaptFromTask<Checker>(std::move(qcChecker)),
                                Options{},
                                std::vector<std::string>{},
                                std::vector<DataProcessorLabel>{} };

  return newChecker;
}

DataProcessorSpec CheckerFactory::create(std::vector<Check> checks, std::string configurationSource)
{
  Checker qcChecker{ checks, configurationSource };

  DataProcessorSpec newChecker{ qcChecker.getDeviceName(),
                                qcChecker.getInputs(),
                                Outputs{ qcChecker.getOutputSpec() },
                                adaptFromTask<Checker>(std::move(qcChecker)),
                                Options{},
                                std::vector<std::string>{},
                                std::vector<DataProcessorLabel>{} };

  return newChecker;
}

void CheckerFactory::customizeInfrastructure(std::vector<framework::CompletionPolicy>& policies)
{
  auto matcher = [](framework::DeviceSpec const& device) {
    return device.name.find(Checker::createCheckerIdString()) != std::string::npos;
  };
  auto callback = [](gsl::span<PartRef const> const& inputs){
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
