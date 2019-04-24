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
/// \file    runReadout.cxx
/// \author  Piotr Konopka
///
/// \brief This is an executable showing QC Task's usage in Data Processing Layer with Readout as external data source.
///
/// This is an executable showing QC Task's usage with Readout as a data producer. To get the Readout data, a proxy is
/// used. Its output is dispatched to QC task using Data Sampling infrastructure. QC Task runs exemplary user code
/// located in SkeletonDPL. The resulting histogram contents are printed by a fake checker.
/// QC task is instantiated by TaskDataProcessorFactory with preinstalled config file, which can be found in
/// ${QUALITYCONTROL_ROOT}/etc/readout.json or Framework/readout.json (original one).
///
/// To launch it, build the project, load the environment and run the executable:
///   \code{.sh}
///   > aliBuild build QualityControl --defaults o2
///   > alienv enter QualityControl/latest
///   > runReadoutChainTemplate
///   \endcode
/// If you have glfw installed, you should see a window with the workflow visualization and sub-windows for each Data
/// Processor where their logs can be seen. The processing will continue until the main window it is closed. Regardless
/// of glfw being installed or not, in the terminal all the logs will be shown as well.

#include "Framework/DataSampling.h"

using namespace o2::framework;

void customize(std::vector<CompletionPolicy>& policies)
{
  DataSampling::CustomizeInfrastructure(policies);
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

#include <TH1F.h>

#include "Framework/DataSamplingReadoutAdapter.h"
#include "Framework/runDataProcessing.h"
#include "QualityControl/InfrastructureGenerator.h"

#include "QualityControl/Checker.h"
#include "QualityControl/CheckerFactory.h"
#include "QualityControl/TaskRunnerFactory.h"
#include "QualityControl/TaskRunner.h"
#include "runnerUtils.h"
#include "ExamplePrinterSpec.h"

#include <iostream>
#include <string>

std::string getConfigPath(const ConfigContext& config);

using namespace o2;
using namespace o2::quality_control::checker;

WorkflowSpec defineDataProcessing(ConfigContext const& config)
{
  // Creating the Readout proxy
  WorkflowSpec specs{
    specifyExternalFairMQDeviceProxy(
      "readout-proxy",
      Outputs{{"ITS", "RAWDATA"}},
      "type=sub,method=connect,address=ipc:///tmp/readout-pipe-1,rateLogging=1",
      dataSamplingReadoutAdapter({"ITS", "RAWDATA"}))
  };

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
      { "checked-mo", "QC", Checker::createCheckerDataDescription(getFirstTaskName(qcConfigurationSource)), 0} },
    Outputs{},
    adaptFromTask<o2::quality_control::example::ExamplePrinterSpec>()
  };
  specs.push_back(printer);

  return specs;
}

std::string getConfigPath(const ConfigContext& config)
{
  // Determine the default config file path and name (based on option no-data-sampling and the QC_ROOT path)
  bool noDS = config.options().get<bool>("no-data-sampling");
  std::string filename = !noDS ? "readout.json" : "readout-no-sampling.json";
  std::string defaultConfigPath = getenv("QUALITYCONTROL_ROOT") != nullptr ? std::string(getenv("QUALITYCONTROL_ROOT")) + "/etc/" + filename : "$QUALITYCONTROL_ROOT undefined";
  // The the optional one by the user
  auto userConfigPath = config.options().get<std::string>("config-path");
  // Finally build the config path based on the default or the user-base one
  std::string path = std::string("json:/") + (userConfigPath.empty() ? defaultConfigPath : userConfigPath);
  return path;
}
