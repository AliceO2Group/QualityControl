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
/// \file    testCheckWorkflow.cxx
/// \author  Rafal Pacholek
///
#include <Framework/DataSampling.h>
#include "QualityControl/InfrastructureGenerator.h"

using namespace o2;
using namespace o2::framework;

const std::string receiverName = "Receiver";

void customize(std::vector<CompletionPolicy>& policies)
{
  DataSampling::CustomizeInfrastructure(policies);
  quality_control::customizeInfrastructure(policies);

  auto matcher = [](framework::DeviceSpec const& device) {
    return device.name.find(receiverName) != std::string::npos;
  };
  auto callback = [](gsl::span<PartRef const> const& inputs) {
    for (auto& input : inputs) {
      if (!(input.header == nullptr || input.payload == nullptr)) {
        return framework::CompletionPolicy::CompletionOp::Consume;
      }
    }
    return framework::CompletionPolicy::CompletionOp::Wait;
  };

  framework::CompletionPolicy checkerCompletionPolicy{ "receiverCompletionPolicy", matcher, callback };
  policies.push_back(checkerCompletionPolicy);
}

#include "getTestDataDirectory.h"
#include "QualityControl/CheckRunner.h"
#include "QualityControl/QualityObject.h"
#include "QualityControl/runnerUtils.h"
#include <Framework/runDataProcessing.h>
#include <Framework/ControlService.h>

using namespace o2::quality_control::core;
using namespace o2::quality_control::checker;

/**
 * Test descrition
 * 
 * Test complex configuratio with 3 tasks and 3 checks.
 * Checks sources contain several tasks with different policies.
 *
 * The goal is to check whether all checks are triggered and generate Quality Objects.
 * It is expected to terminate whenever all task publish for the first time.
 */

class Receiver : public framework::Task
{
 public:
  Receiver(std::string configurationSource)
  {
    auto config = o2::configuration::ConfigurationFactory::getConfiguration(configurationSource);

    for (const auto& task : config->getRecursive("qc.checks")) {
      mNames.insert(task.first); // check name;
    }
  }

  Receiver(const Receiver& receiver) : mNames(receiver.mNames) {}

  /// Destructor
  ~Receiver() override{};

  /// \brief Receiver process callback
  void run(framework::ProcessingContext& pctx) override
  {
    for (auto& checkName : mNames) {
      if (pctx.inputs().isValid(checkName)) {
        auto qo = pctx.inputs().get<QualityObject*>(checkName);
        if (!qo) {
          LOG(ERROR) << qo->getName() << " - quality is NULL";
          pctx.services().get<ControlService>().readyToQuit(true);
        } else {
          LOG(DEBUG) << qo->getName() << " - qualit: " << qo->getQuality();

          mNames.erase(checkName);
        }
        // We ask to shut the topology down, returning 0 if there were no ERROR logs.
      }
    }

    if (!mNames.size()) {
      pctx.services().get<ControlService>().readyToQuit(true);
    }
    LOG(DEBUG) << "Requires " << mNames.size() << " quality objects";
  }

  Inputs getInputs()
  {
    Inputs inputs;
    for (auto& checkName : mNames) {
      inputs.push_back({ checkName, "QC", Check::createCheckerDataDescription(checkName) });
    }
    return inputs;
  }

 private:
  std::set<std::string> mNames;
};

WorkflowSpec defineDataProcessing(ConfigContext const&)
{
  WorkflowSpec specs;

  // The producer to generate some data in the workflow
  DataProcessorSpec producer{
    "producer",
    Inputs{},
    Outputs{
      { { "tst-data" }, "TST", "DATA" } },
    AlgorithmSpec{
      [](ProcessingContext& pctx) {
        usleep(100000);
        pctx.outputs().make<int>(OutputRef{ "tst-data" }, 1);
      } }
  };
  specs.push_back(producer);

  const std::string qcConfigurationSource = std::string("json://") + getTestDataDirectory() + "testCheckWorkflow.json";

  LOG(INFO) << "Using config file '" << qcConfigurationSource << "'";

  // Generation of Data Sampling infrastructure
  DataSampling::GenerateInfrastructure(specs, qcConfigurationSource);

  // Generation of the QC topology (one task, one checker in this case)
  quality_control::generateRemoteInfrastructure(specs, qcConfigurationSource);

  Receiver receiver(qcConfigurationSource);
  // Finally the receiver
  DataProcessorSpec receiverSpec{
    receiverName,
    receiver.getInputs(),
    Outputs{},
    adaptFromTask<Receiver>(std::move(receiver)),
    Options{},
    std::vector<std::string>{},
    std::vector<DataProcessorLabel>{}
  };
  specs.push_back(receiverSpec);

  return specs;
}
