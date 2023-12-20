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
/// \file    runBasic.cxx
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
/// ${QUALITYCONTROL_ROOT}/etc/basic.json or Framework/basic.json (original one).
///
/// To launch it, build the project, load the environment and run the executable:
///   \code{.sh}
///   > aliBuild build QualityControl --defaults o2
///   > alienv enter QualityControl/latest
///   > o2-qc-run-basic
///   \endcode
/// If you have glfw installed, you should see a window with the workflow visualization and sub-windows for each Data
/// Processor where their logs can be seen. The processing will continue until the main window it is closed. Regardless
/// of glfw being installed or not, in the terminal all the logs will be shown as well.

#include <DataSampling/DataSampling.h>
#include "QualityControl/InfrastructureGenerator.h"
#include "Common/Exceptions.h"

using namespace o2;
using namespace o2::framework;
using namespace o2::utilities;
using namespace AliceO2::Common;

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
    ConfigParamSpec{ "config-path", VariantType::String, "", { "Absolute path to the config file. Overwrite the default paths. Do not use with no-data-sampling." } });
  workflowOptions.push_back(
    ConfigParamSpec{ "no-data-sampling", VariantType::Bool, false, { "Skips data sampling, connects directly the task to the producer." } });
}

#include <string>

#include <Framework/runDataProcessing.h>
#include <Configuration/ConfigurationFactory.h>
#include <Configuration/ConfigurationInterface.h>

#include "QualityControl/CheckRunner.h"
#include "QualityControl/InfrastructureGenerator.h"
#include "QualityControl/runnerUtils.h"
#include "QualityControl/ExamplePrinterSpec.h"
#include "QualityControl/DataProducer.h"
#include "QualityControl/TaskRunner.h"

std::string getConfigPath(const ConfigContext& config);

using namespace o2;
using namespace o2::framework;
using namespace o2::configuration;
using namespace o2::quality_control::checker;
using namespace std::chrono;

WorkflowSpec defineDataProcessing(const ConfigContext& config)
{
  WorkflowSpec specs;
  std::string qcConfigurationSource = getConfigPath(config);

  auto configTree = ConfigurationFactory::getConfiguration(qcConfigurationSource)->getRecursive();
  auto infologgerFilterDiscardDebug = configTree.get<bool>("qc.config.infologger.filterDiscardDebug", false);
  auto infologgerDiscardLevel = configTree.get<int>("qc.config.infologger.filterDiscardLevel", 21);
  auto infologgerDiscardFile = configTree.get<std::string>("qc.config.infologger.filterDiscardFile", "");
  ILOG_INST.filterDiscardDebug(infologgerFilterDiscardDebug);
  ILOG_INST.filterDiscardLevel(infologgerDiscardLevel);
  ILOG_INST.filterDiscardSetFile(infologgerDiscardFile.c_str(), 0, 0, 0, true /*Do not store Debug messages in file*/);
  QcInfoLogger::setFacility("runBasic");

  // The producer to generate some data in the workflow
  DataProcessorSpec producer = getDataProducerSpec(1, 10000, 10);
  specs.push_back(producer);

  // Path to the config file
  ILOG(Info, Support) << "Using config file '" << qcConfigurationSource << "'" << ENDM;

  // Generation of Data Sampling infrastructure
  auto configInterface = ConfigurationFactory::getConfiguration(qcConfigurationSource);
  auto dataSamplingTree = configInterface->getRecursive("dataSamplingPolicies");
  DataSampling::GenerateInfrastructure(specs, dataSamplingTree);

  // Generation of the QC topology (one task, one checker in this case)
  quality_control::generateStandaloneInfrastructure(specs, configInterface->getRecursive());

  // Finally the printer
  if (hasChecks(qcConfigurationSource)) {
    DataProcessorSpec printer{
      .name = "printer",
      .inputs = Inputs{
        { "checked-mo", "QC", Check::createCheckDataDescription(getFirstCheckName(qcConfigurationSource)), 0, Lifetime::Sporadic } },
      .algorithm = adaptFromTask<o2::quality_control::example::ExampleQualityPrinterSpec>(),
      .labels = { { "resilient" } }
    };
    specs.push_back(printer);
  } else {
    DataProcessorSpec printer{
      .name = "printer",
      .inputs = Inputs{
        { "checked-mo", "QC", TaskRunner::createTaskDataDescription(getFirstTaskName(qcConfigurationSource)), 0, Lifetime::Sporadic } },
      .algorithm = adaptFromTask<o2::quality_control::example::ExamplePrinterSpec>(),
      .labels = { { "resilient" } }

    };
    specs.push_back(printer);
  }

  return specs;
}

// TODO merge this with the one from runReadout.cxx
std::string getConfigPath(const ConfigContext& config)
{
  // Determine the default config file path and name (based on option no-data-sampling and the QC_ROOT path)
  bool noDS = config.options().get<bool>("no-data-sampling");
  std::string filename = !noDS ? "basic.json" : "basic-no-sampling.json";
  char* qcPath = getenv("QUALITYCONTROL_ROOT");
  // if the var is not set, we just bail because it is most probably not reasonable to guess.
  if (qcPath == nullptr) {
    BOOST_THROW_EXCEPTION(FatalException() << errinfo_details("Env var QUALITYCONTROL_ROOT not set. We cannot continue."));
  }
  std::string defaultConfigPath = std::string(getenv("QUALITYCONTROL_ROOT")) + "/etc/" + filename;
  // The optional one by the user
  auto userConfigPath = config.options().get<std::string>("config-path");
  // Finally build the config path based on the default or the user-base one
  std::string path = std::string("json://") + (userConfigPath.empty() ? defaultConfigPath : userConfigPath);
  return path;
}
