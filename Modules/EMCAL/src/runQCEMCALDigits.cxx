#include <string>
#include <TH1.h>

#if __has_include(<Framework/DataSampling.h>)
#include <Framework/DataSampling.h>
using namespace o2::framework;
// TODO bring back full namespaces after the migration
#else
#include <DataSampling/DataSampling.h>
using namespace o2::utilities;
#endif
#include <DataFormatsEMCAL/Digit.h>
#include <DataFormatsEMCAL/Cell.h>
#include <EMCALWorkflow/PublisherSpec.h>
#include "QualityControl/InfrastructureGenerator.h"
#include "QualityControl/CheckRunner.h"
#include "QualityControl/CheckRunnerFactory.h"

void customize(std::vector<o2::framework::CompletionPolicy>& policies)
{
  DataSampling::CustomizeInfrastructure(policies);
  o2::quality_control::customizeInfrastructure(policies);
}

void customize(std::vector<o2::framework::ChannelConfigurationPolicy>& policies)
{
  DataSampling::CustomizeInfrastructure(policies);
}

void customize(std::vector<o2::framework::ConfigParamSpec>& workflowOptions)
{
  workflowOptions.push_back(
    o2::framework::ConfigParamSpec{ "config-path", o2::framework::VariantType::String, "", { "Path to the config file. Overwrite the default paths. Do not use with no-data-sampling." } });
  workflowOptions.push_back(
    o2::framework::ConfigParamSpec{ "input-type", o2::framework::VariantType::String, "cell", { "Input data type. Can be \"digit\" or \"cell\"." } });
  workflowOptions.push_back(
    o2::framework::ConfigParamSpec{ "no-data-sampling", o2::framework::VariantType::Bool, false, { "Skips data sampling, connects directly the task to the producer." } });
  workflowOptions.push_back(
    o2::framework::ConfigParamSpec{ "local", o2::framework::VariantType::Bool, false, { "Creates only the local part of the QC topology." } });
  workflowOptions.push_back(
    o2::framework::ConfigParamSpec{ "remote", o2::framework::VariantType::Bool, false, { "Creates only the remote part of the QC topology." } });
  workflowOptions.push_back(
    o2::framework::ConfigParamSpec{ "host", o2::framework::VariantType::String, "", { "Name of the host of the local part of the QC topology."
                                                                                      "Necessary to specify when creating topologies on multiple"
                                                                                      " machines, can be omitted for the local development" } });
}

#include "Framework/runDataProcessing.h"

std::string getConfigPath(const o2::framework::ConfigContext& config);

o2::framework::DataProcessorSpec getDigitsPublisher()
{
  using digitInputType = std::vector<o2::emcal::Digit>;
  return o2::emcal::getPublisherSpec<digitInputType>(o2::emcal::PublisherConf{
                                                       "emcal-digit-reader",
                                                       "o2sim",
                                                       { "digitbranch", "EMCALDigit", "Digit branch" },
                                                       { "triggerrecordbranch", "EMCALDigitTRGR", "Trigger record branch" },
                                                       { "mcbranch", "EMCALDigitMCTruth", "MC label branch" },
                                                       o2::framework::OutputSpec{ "EMC", "DIGITS" },
                                                       o2::framework::OutputSpec{ "EMC", "DIGITSTRGR" },
                                                       o2::framework::OutputSpec{ "EMC", "DIGITSMCTR" } },
                                                     false);
}

o2::framework::DataProcessorSpec getCellPublisher()
{
  using digitInputType = std::vector<o2::emcal::Cell>;
  return o2::emcal::getPublisherSpec<digitInputType>(o2::emcal::PublisherConf{
                                                       "emcal-digit-reader",
                                                       "o2sim",
                                                       { "digitbranch", "EMCALCell", "Digit branch" },
                                                       { "triggerrecordbranch", "EMCALCellTRGR", "Trigger record branch" },
                                                       { "mcbranch", "EMCALDigitMCTruth", "MC label branch" },
                                                       o2::framework::OutputSpec{ "EMC", "CELLS" },
                                                       o2::framework::OutputSpec{ "EMC", "CELLSTRGR" },
                                                       o2::framework::OutputSpec{ "EMC", "CELLSMCTR" } },
                                                     false);
}

o2::framework::WorkflowSpec defineDataProcessing(o2::framework::ConfigContext const& config)
{
  o2::framework::WorkflowSpec specs;
  auto inputtype = config.options().get<std::string>("input-type");
  if (inputtype == "cell") {
    specs.push_back(getCellPublisher());
  } else {
    specs.push_back(getDigitsPublisher());
  }

  // Path to the config file
  std::string qcConfigurationSource = getConfigPath(config);
  LOG(INFO) << "Using config file '" << qcConfigurationSource << "'";

  if (!config.options().get<bool>("local") && !config.options().get<bool>("remote")) {
    ILOG(Info, Support) << "Creating a standalone QC topology." << ENDM;
    o2::quality_control::generateStandaloneInfrastructure(specs, qcConfigurationSource);
  }

  if (config.options().get<bool>("local")) {
    ILOG(Info, Support) << "Creating a local QC topology." << ENDM;

    // Generation of Data Sampling infrastructure
    DataSampling::GenerateInfrastructure(specs, qcConfigurationSource);

    // Generation of the local QC topology (local QC tasks and their output proxies)
    o2::quality_control::generateLocalInfrastructure(specs, qcConfigurationSource, config.options().get<std::string>("host"));
  }
  if (config.options().get<bool>("remote")) {
    ILOG(Info, Support) << "Creating a remote QC topology." << ENDM;

    // Generation of the remote QC topology (task for QC servers, input proxies, mergers and all check runners)
    o2::quality_control::generateRemoteInfrastructure(specs, qcConfigurationSource);
  }

  return specs;
}

std::string getConfigPath(const o2::framework::ConfigContext& config)
{
  std::string userConfigPath = config.options().get<std::string>("config-path");
  std::string defaultConfigPath = getenv("QUALITYCONTROL_ROOT") != nullptr ? std::string(getenv("QUALITYCONTROL_ROOT")) + "/Modules/EMCAL/etc/digits.json" : "$QUALITYCONTROL_ROOT undefined";
  std::string path = userConfigPath == "" ? defaultConfigPath : userConfigPath;
  const std::string qcConfigurationSource = std::string("json:/") + path;
  return qcConfigurationSource;
}
