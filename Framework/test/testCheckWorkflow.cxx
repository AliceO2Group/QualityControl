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

#include <DataSampling/DataSampling.h>
#include <Framework/CompletionPolicyHelpers.h>
#include <Framework/DeviceSpec.h>
#include "QualityControl/InfrastructureGenerator.h"

using namespace o2;
using namespace o2::framework;
using namespace o2::utilities;

const std::string receiverName = "Receiver";

void customize(std::vector<CompletionPolicy>& policies)
{
  DataSampling::CustomizeInfrastructure(policies);
  quality_control::customizeInfrastructure(policies);

  auto matcher = [](framework::DeviceSpec const& device) {
    return device.name.find(receiverName) != std::string::npos;
  };
  auto callback = CompletionPolicyHelpers::consumeWhenAny().callback;

  policies.emplace_back("receiverCompletionPolicy", matcher, callback);
}

#include "getTestDataDirectory.h"
#include "QualityControl/CheckRunner.h"
#include "QualityControl/QualityObject.h"
#include "QualityControl/runnerUtils.h"
#include <Framework/runDataProcessing.h>
#include <Framework/ControlService.h>
#include <set>
#include <vector>

using namespace o2::quality_control::core;
using namespace o2::quality_control::checker;

/**
 * Test description
 * 
 * Test a complex configuration with 3 tasks and 4 checks.
 * Checks sources contain several tasks with different policies.
 *
 * The goal is to check whether all checks are triggered and generate Quality Objects.
 * It is expected to terminate as soon as all task publish for the first time.
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

  /// Destructor
  ~Receiver() override{};

  /// \brief Receiver process callback
  void run(framework::ProcessingContext& pctx) override
  {
    std::vector<std::string> namesToErase;

    for (const auto& checkName : mNames) {
      if (pctx.inputs().isValid(checkName)) {
        auto qo = pctx.inputs().get<QualityObject*>(checkName);
        if (!qo) {
          ILOG(Error, Devel) << qo->getName() << " - quality is NULL" << ENDM;
          pctx.services().get<ControlService>().readyToQuit(QuitRequest::All);
        } else {
          ILOG(Debug, Devel) << qo->getName() << " - quality: " << qo->getQuality() << ENDM;
          namesToErase.emplace_back(checkName);
        }
      }
    }

    for (const auto& nameToErase : namesToErase) {
      mNames.erase(nameToErase);
    }

    if (mNames.empty()) {
      // We ask to shut the topology down, returning 0 if there were no ERROR logs.
      pctx.services().get<ControlService>().readyToQuit(QuitRequest::All);
    }
    ILOG(Debug, Devel) << "Requires " << mNames.size() << " quality objects" << ENDM;
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

  ILOG(Info) << "Using config file '" << qcConfigurationSource << "'" << ENDM;

  // Generation of Data Sampling infrastructure
  DataSampling::GenerateInfrastructure(specs, qcConfigurationSource);

  // Generation of the QC topology (one task, one checker in this case)
  quality_control::generateStandaloneInfrastructure(specs, qcConfigurationSource);

  Receiver receiver(qcConfigurationSource);
  // Finally the receiver
  DataProcessorSpec receiverSpec{ receiverName, receiver.getInputs(), {}, {} };
  // We move the task at the end, so receiver.getInputs() is not called first.
  receiverSpec.algorithm = adaptFromTask<Receiver>(std::move(receiver));
  specs.push_back(receiverSpec);

  return specs;
}
