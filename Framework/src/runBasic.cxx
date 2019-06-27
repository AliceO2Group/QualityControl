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
/// \file    runBasic.cxx
/// \author  Piotr Konopka
///
/// \brief This is an executable showing QC Task's usage in Data Processing Layer.
///
/// This is an executable showing QC Task's usage in Data Processing Layer. The workflow consists of data producer,
/// which generates arrays of random size and content. Its output is dispatched to QC task using Data Sampling
/// infrastructure. QC Task runs exemplary user code located in SkeletonDPL. The checker performes a simple check of
/// the histogram shape and colorizes it. The resulting histogram contents are shown in logs by printer.
///
/// QC task and Checker are instantiated by respectively TaskFactory and CheckerFactory,
/// which use preinstalled config file, that can be found in
/// ${QUALITYCONTROL_ROOT}/etc/qcTaskDplConfig.json or Framework/qcTaskDplConfig.json (original one).
///
/// To launch it, build the project, load the environment and run the executable:
///   \code{.sh}
///   > aliBuild build QualityControl --defaults o2
///   > alienv enter QualityControl/latest
///   > runTaskDPL
///   \endcode
/// If you have glfw installed, you should see a window with the workflow visualization and sub-windows for each Data
/// Processor where their logs can be seen. The processing will continue until the main window it is closed. Regardless
/// of glfw being installed or not, in the terminal all the logs will be shown as well.

#include "Framework/DataSampling.h"
#include "QualityControl/InfrastructureGenerator.h"

using namespace o2;
using namespace o2::framework;

void customize(std::vector<CompletionPolicy>& policies)
{
  DataSampling::CustomizeInfrastructure(policies);
  quality_control::customizeInfrastructure(policies);
}

void customize(std::vector<ChannelConfigurationPolicy>& policies)
{
  DataSampling::CustomizeInfrastructure(policies);
}

void customize(std::vector<ConfigParamSpec>& workflowOptions)
{
  workflowOptions.push_back(
    ConfigParamSpec{ "config-path", VariantType::String, "", { "Path to the config file. Overwrite the default paths. Do not use with no-data-sampling." } });
  workflowOptions.push_back(
    ConfigParamSpec{ "no-data-sampling", VariantType::Bool, false, { "Skips data sampling, connects directly the task to the producer." } });
}

#include <FairLogger.h>
#include <TH1F.h>
#include <memory>
#include <random>

#include "Framework/runDataProcessing.h"

#include "QualityControl/Checker.h"
#include "QualityControl/InfrastructureGenerator.h"
#include "runnerUtils.h"
#include "ExamplePrinterSpec.h"

std::string getConfigPath(const ConfigContext& config);

using namespace o2::framework;
using namespace o2::quality_control::checker;
using namespace std::chrono;

WorkflowSpec defineDataProcessing(const ConfigContext& config)
{
  WorkflowSpec specs;

  // The producer to generate some data in the workflow
  DataProcessorSpec producer{
    "producer",
    Inputs{},
    Outputs{
      { "ITS", "RAWDATA", 0, Lifetime::Timeframe }
    },
    AlgorithmSpec{
      (AlgorithmSpec::InitCallback) [](InitContext&) {
        std::default_random_engine generator(11);
        return (AlgorithmSpec::ProcessCallback) [generator](ProcessingContext& processingContext) mutable {
          usleep(100000);
          size_t length = generator() % 10000;
          auto data = processingContext.outputs().make<char>(Output{ "ITS", "RAWDATA", 0, Lifetime::Timeframe },
                                                             length);
          for (auto&& item : data) {
            item = static_cast<char>(generator());
          }
        };
      }
    }
  };

  specs.push_back(producer);

  // Path to the config file
  std::string qcConfigurationSource = getConfigPath(config);
  LOG(INFO) << "Using config file '" << qcConfigurationSource << "'";

  // Generation of Data Sampling infrastructure
  DataSampling::GenerateInfrastructure(specs, qcConfigurationSource);

  // Generation of the QC topology (one task, one checker in this case)
  quality_control::generateRemoteInfrastructure(specs, qcConfigurationSource);

  // Finally the printer
  DataProcessorSpec printer{
    "printer",
    Inputs{
      { "checked-mo", "QC", Checker::createCheckerDataDescription(getFirstTaskName(qcConfigurationSource)), 0 } },
    Outputs{},
    adaptFromTask<o2::quality_control::example::ExamplePrinterSpec>()
  };
  specs.push_back(printer);

  return specs;
}

// TODO merge this with the one from runReadout.cxx
std::string getConfigPath(const ConfigContext& config)
{
  // Determine the default config file path and name (based on option no-data-sampling and the QC_ROOT path)
  bool noDS = config.options().get<bool>("no-data-sampling");
  std::string filename = !noDS ? "basic.json" : "basic-no-sampling.json";
  std::string defaultConfigPath = getenv("QUALITYCONTROL_ROOT") != nullptr ? std::string(getenv("QUALITYCONTROL_ROOT")) + "/etc/" + filename : "$QUALITYCONTROL_ROOT undefined";
  // The the optional one by the user
  auto userConfigPath = config.options().get<std::string>("config-path");
  // Finally build the config path based on the default or the user-base one
  std::string path = std::string("json:/") + (userConfigPath.empty() ? defaultConfigPath : userConfigPath);
  return path;
}
