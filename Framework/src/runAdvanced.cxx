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
/// \file    runAdvanced.cxx
/// \author  Piotr Konopka
///
/// \brief This is an executable showing a more complicated QC topology.
///
/// This is an executable showing a more complicated QC topology. It pretends to spawn 4 separate topologies - 3 of them
/// consist of some dummy processing chain, a dispatcher and a local QC task. The last one represents the remote
/// QC servers topology, which has a merger (joining the results from local QC tasks), a checker (checks the result of
/// the previous) and a different, remote QC task with associated checker. Here they are joined into one big topology
/// just to present the concept.
/// \image html qcRunAdvanced.png
///
/// To launch it, build the project, load the environment and run the executable:
///   \code{.sh}
///   > aliBuild build QualityControl --defaults o2
///   > alienv enter QualityControl/latest
///   > qcRunAdvanced
///   \endcode
/// If you have glfw installed, you should see a window with the workflow visualization and sub-windows for each Data
/// Processor where their logs can be seen. The processing will continue until the main window it is closed. Regardless
/// of glfw being installed or not, in the terminal all the logs will be shown as well.

#include <Framework/CompletionPolicyHelpers.h>
#include <Framework/DataSampling.h>

using namespace o2::framework;

// Additional configuration of the topology, which is done by implementing `customize` functions and placing them
// before `runDataProcessing.h` header. In this case, both Dispatcher and Merger are configured to accept incoming
// messages without waiting for the rest of inputs.
void customize(std::vector<CompletionPolicy>& policies)
{
  DataSampling::CustomizeInfrastructure(policies);

  CompletionPolicy mergerConsumesASAP{
    "mergers-always-consume",
    [](DeviceSpec const& device) {
      return device.name.find("merger") != std::string::npos;
    },
    [](gsl::span<PartRef const> const& inputs) {
      return CompletionPolicy::CompletionOp::Consume;
    }
  };
  policies.push_back(mergerConsumesASAP);
}

void customize(std::vector<ChannelConfigurationPolicy>& policies)
{
  DataSampling::CustomizeInfrastructure(policies);
}

#include "QualityControl/InfrastructureGenerator.h"
#include <Framework/runDataProcessing.h>
#include <random>

using namespace o2::header;
using namespace o2::quality_control::core;
using SubSpecificationType = o2::header::DataHeader::SubSpecificationType;

// clang-format off
WorkflowSpec processingTopology(SubSpecificationType subspec)
{
  DataProcessorSpec source{
    "source-" + std::to_string(subspec),
    Inputs{},
    Outputs{{ "TST", "DATA",  subspec },
            { "TST", "PARAM", subspec }},
    AlgorithmSpec{
      (AlgorithmSpec::ProcessCallback)
        [generator = std::default_random_engine{ time(nullptr) }, subspec](ProcessingContext & ctx) mutable {
          usleep(200000);
          auto data = ctx.outputs().make<int>(Output{ "TST", "DATA", subspec }, generator() % 10000);
          for (auto&& item : data) {
            item = static_cast<int>(generator());
          }
          ctx.outputs().make<double>(Output{ "TST", "PARAM", subspec }, 1)[0] = 1 / static_cast<double>(1 + generator());
        }
    }
  };

  DataProcessorSpec step{
    "step-" + std::to_string(subspec),
    Inputs{{ "data", "TST", "DATA", subspec }},
    Outputs{{ "TST", "SUM", subspec }},
    AlgorithmSpec{
      (AlgorithmSpec::ProcessCallback)[subspec](ProcessingContext & ctx) {
        const auto* header = get<DataHeader*>(ctx.inputs().get("data").header);
        auto data = DataRefUtils::as<int>(ctx.inputs().get("data"));
        long long sum = 0;
        for (auto d : data) { sum += d; }
        ctx.outputs().snapshot(Output{ "TST", "SUM", subspec }, sum);
      }
    }
  };

  DataProcessorSpec sink{
    "sink-" + std::to_string(subspec),
    Inputs{{ "sum",   "TST", "SUM",   subspec },
           { "param", "TST", "PARAM", subspec }},
    Outputs{},
    AlgorithmSpec{
      (AlgorithmSpec::ProcessCallback)[](ProcessingContext & ctx) {
        LOG(INFO) << "Sum is: " << DataRefUtils::as<long long>(ctx.inputs().get("sum"))[0];
        LOG(INFO) << "Param is: " << DataRefUtils::as<double>(ctx.inputs().get("param"))[0];
      }
    }
  };

  return { source, step, sink };
}
// clang-format on

WorkflowSpec defineDataProcessing(ConfigContext const&)
{
  const std::string qcConfigurationSource =
    std::string("json://") + getenv("QUALITYCONTROL_ROOT") + "/etc/advanced.json";
  LOG(INFO) << "Using config file '" << qcConfigurationSource << "'";

  WorkflowSpec specs;
  // here we pretend to spawn topologies on three processing machines
  for (int i = 1; i < 4; i++) {
    auto localTopology = processingTopology(i);

    DataSampling::GenerateInfrastructure(localTopology, qcConfigurationSource);
    // a fix to make the topologies work when merged together
    localTopology.back().name += std::to_string(i);

    std::string host = "o2flptst" + std::to_string(i);
    auto qcInfrastructure = InfrastructureGenerator::generateLocalInfrastructure(qcConfigurationSource, host);
    // a fix to make the topologies work when merged together
    qcInfrastructure.back().name += std::to_string(i);
    // temporary fix, which shouldn't be necessary when data sampling uses matchers
    qcInfrastructure.back().inputs[0].subSpec = i;
    localTopology.insert(localTopology.end(), qcInfrastructure.begin(), qcInfrastructure.end());

    specs.insert(std::end(specs), std::begin(localTopology), std::end(localTopology));
  }

  // Generation of the remote QC topology (for the QC servers)
  auto qcInfrastructure = InfrastructureGenerator::generateRemoteInfrastructure(qcConfigurationSource);
  specs.insert(std::end(specs), std::begin(qcInfrastructure), std::end(qcInfrastructure));

  return specs;
}