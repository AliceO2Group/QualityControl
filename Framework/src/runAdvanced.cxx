// Copyright 2019-2020 CERN and copyright holders of ALICE O2.
// See https://alice-o2.web.cern.ch/copyright for details of the copyright holders.
// All rights not expressly granted are reserved.
//
// This software is distributed under the terms of the GNU General Public
// License v3 (GPL Version 3), copied verbatim in the file "COPYING".
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.

///
/// \file    runAdvanced.cxx
/// \author  Piotr Konopka
/// \author Barthelemy von Haller
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
///
/// In case you want to run only the processing part, use the option `--no-qc`.
/// In such case, the workflow can be piped to the QC or another workflow:
///   \code{.sh}
///   > o2-qc-run-advanced --no-qc | o2-qc --config json://${QUALITYCONTROL_ROOT}/etc/advanced.json
///   \endcode

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

void customize(std::vector<ConfigParamSpec>& workflowOptions)
{
  workflowOptions.push_back(
    ConfigParamSpec{ "no-qc", VariantType::Bool, false, { "Disable the QC part of this advanced workflow." } });
  workflowOptions.push_back(
    ConfigParamSpec{ "no-debug-output", VariantType::Bool, false, { "Disable the Debug output." } });
}

#include <Framework/runDataProcessing.h>
#include <Framework/DataSpecUtils.h>
#include <Configuration/ConfigurationFactory.h>
#include <Configuration/ConfigurationInterface.h>
#include "QualityControl/QcInfoLogger.h"
#include "QualityControl/AdvancedWorkflow.h"

using namespace o2;
using namespace o2::header;
using namespace o2::quality_control::core;
using SubSpecificationType = o2::header::DataHeader::SubSpecificationType;
using namespace o2::configuration;

WorkflowSpec defineDataProcessing(ConfigContext const& config)
{
  QcInfoLogger::setFacility("runAdvanced");

  bool noQC = config.options().get<bool>("no-qc");
  bool noDebug = config.options().get<bool>("no-debug-output");
  const std::string qcConfigurationSource =
    std::string("json://") + getenv("QUALITYCONTROL_ROOT") + "/etc/advanced.json";
  ILOG(Info, Support) << "Using config file '" << qcConfigurationSource << "'";
  ILOG_INST.filterDiscardDebug(noDebug);

  // Full processing topology.
  // We pretend to spawn topologies on three processing machines
  WorkflowSpec specs = getFullProcessingTopology();

  if (!noQC) {
    auto configInterface = ConfigurationFactory::getConfiguration(qcConfigurationSource);
    auto dataSamplingTree = configInterface->getRecursive("dataSamplingPolicies");
    DataSampling::GenerateInfrastructure(specs, dataSamplingTree);
    // Generation of the remote QC topology (for the QC servers)
    quality_control::generateStandaloneInfrastructure(specs, configInterface->getRecursive());
  }
  return specs;
}
