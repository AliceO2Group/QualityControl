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
/// \file    runTOF.cxx
/// \author  Piotr Konopka
///
/// \brief This is an executable showing QC Task's usage in Data Processing Layer.
///
/// This is an executable showing QC Task's usage in Data Processing Layer. The workflow consists of data producer,
/// which generates arrays of random size and content. Its output is dispatched to QC task using Data Sampling
/// infrastructure. QC Task runs exemplary user code located in SkeletonDPL. The checker performs a simple check of
/// the histogram shape and colorizes it. The resulting histogram contents are shown in logs by printer.
///
/// QC task and CheckRunner are instantiated by respectively TaskFactory and CheckRunnerFactory,
/// which use preinstalled config file, that can be found in
/// ${QUALITYCONTROL_ROOT}/etc/tof.json or Framework/tof.json (original one).
///
/// To launch it, build the project, load the environment and run the executable:
///   \code{.sh}
///   > aliBuild build QualityControl --defaults o2
///   > alienv enter QualityControl/latest
///   > o2-qc-run-tof
///   \endcode
/// If you have glfw installed, you should see a window with the workflow visualization and sub-windows for each Data
/// Processor where their logs can be seen. The processing will continue until the main window it is closed. Regardless
/// of glfw being installed or not, in the terminal all the logs will be shown as well.

#include <Framework/DataSampling.h>
#include "QualityControl/InfrastructureGenerator.h"

using namespace o2;
using namespace o2::framework;

// The customize() functions are used to declare the executable arguments and to specify custom completion and channel
// configuration policies. They have to be above `#include "Framework/runDataProcessing.h"` - that header checks if
// these functions are defined by user and if so, it invokes them. It uses a trick with SFINAE expressions to do that.

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

#include <string>
#include <fairlogger/Logger.h>
#include <TH1F.h>
#include <memory>
#include <random>

#include <Framework/runDataProcessing.h>

#include "QualityControl/CheckRunner.h"
#include "QualityControl/InfrastructureGenerator.h"
#include "QualityControl/runnerUtils.h"
#include "QualityControl/ExamplePrinterSpec.h"
#include "QualityControl/DataProducer.h"

std::string getConfigPath(const ConfigContext& config);

using namespace o2;
using namespace o2::framework;
using namespace o2::quality_control::checker;
using namespace std::chrono;

WorkflowSpec defineDataProcessing(const ConfigContext& config)
{
  WorkflowSpec specs;

  // The producer to generate some data in the workflow
  DataProcessorSpec producer = getDataProducerSpec(1, 10000, 10);
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
      { "checked-mo", "QC", CheckRunner::createCheckRunnerDataDescription(getFirstCheckerName(qcConfigurationSource)), 0 } },
    Outputs{},
    adaptFromTask<o2::quality_control::example::ExampleQualityPrinterSpec>()
  };
  specs.push_back(printer);

  return specs;
}

// TODO merge this with the one from runReadout.cxx
std::string getConfigPath(const ConfigContext& config)
{
  // Determine the default config file path and name (based on option no-data-sampling and the QC_ROOT path)
  std::string filename = "tof.json";
  std::string defaultConfigPath = getenv("QUALITYCONTROL_ROOT") != nullptr ? std::string(getenv("QUALITYCONTROL_ROOT")) + "/etc/" + filename : "$QUALITYCONTROL_ROOT undefined";
  // The the optional one by the user
  auto userConfigPath = config.options().get<std::string>("config-path");
  // Finally build the config path based on the default or the user-base one
  std::string path = std::string("json:/") + (userConfigPath.empty() ? defaultConfigPath : userConfigPath);
  return path;
}
