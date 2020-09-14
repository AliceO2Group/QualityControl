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
/// This is an executable showing a more complicated QC topology. It spawns 3 separate dummy processing chains,
/// a Dispatcher, two QC Tasks which require different data and CheckRunners which run Checks on MonitorObjects
/// produced by these QC Tasks.
/// \image html qcRunAdvanced.png
///
/// To launch it, build the project, load the environment and run the executable:
///   \code{.sh}
///   > aliBuild build QualityControl --defaults o2
///   > alienv enter QualityControl/latest
///   > o2-qc-run-advanced
///   \endcode
/// If you have glfw installed, you should see a window with the workflow visualization and sub-windows for each Data
/// Processor where their logs can be seen. The processing will continue until the main window it is closed. Regardless
/// of glfw being installed or not, in the terminal all the logs will be shown as well.

#include <Framework/CompletionPolicyHelpers.h>
#include <Framework/DataSpecUtils.h>
#include <Framework/CompletionPolicyHelpers.h>
#include <DataSampling/DataSampling.h>
#include "QualityControl/InfrastructureGenerator.h"

using namespace o2;
using namespace o2::framework;
using namespace o2::utilities;

// Additional configuration of the topology, which is done by implementing `customize` functions and placing them
// before `runDataProcessing.h` header. In this case, both Dispatcher and Merger are configured to accept incoming
// messages without waiting for the rest of inputs. The `customize` functions have to be above
// `#include "Framework/runDataProcessing.h"` - that header checks if these functions are defined by user and if so, it
// invokes them. It uses a trick with SFINAE expressions to do that.
void customize(std::vector<CompletionPolicy>& policies)
{
  DataSampling::CustomizeInfrastructure(policies);
  quality_control::customizeInfrastructure(policies);
}

void customize(std::vector<ChannelConfigurationPolicy>& policies)
{
  DataSampling::CustomizeInfrastructure(policies);
}

#include <Framework/runDataProcessing.h>
#include <random>

using namespace o2;
using namespace o2::header;
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
        [generator = std::default_random_engine{ static_cast<unsigned int>(time(nullptr)) }, subspec](ProcessingContext & ctx) mutable {
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
        LOG(DEBUG) << "Sum is: " << DataRefUtils::as<long long>(ctx.inputs().get("sum"))[0];
        LOG(DEBUG) << "Param is: " << DataRefUtils::as<double>(ctx.inputs().get("param"))[0];
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
    specs.insert(std::end(specs), std::begin(localTopology), std::end(localTopology));
  }

  DataSampling::GenerateInfrastructure(specs, qcConfigurationSource);

  // Generation of the remote QC topology (for the QC servers)
  quality_control::generateStandaloneInfrastructure(specs, qcConfigurationSource);

  return specs;
}
