#include <string>
#include <TH1.h>

#include <Framework/DataSampling.h>
#include <DataFormatsEMCAL/Digit.h>
#include <EMCALWorkflow/PublisherSpec.h>
#include "QualityControl/InfrastructureGenerator.h"
#include "QualityControl/CheckRunner.h"
#include "QualityControl/CheckRunnerFactory.h"

void customize(std::vector<o2::framework::CompletionPolicy>& policies)
{
  o2::framework::DataSampling::CustomizeInfrastructure(policies);
  o2::quality_control::customizeInfrastructure(policies);
}

void customize(std::vector<o2::framework::ChannelConfigurationPolicy>& policies)
{
  o2::framework::DataSampling::CustomizeInfrastructure(policies);
}

void customize(std::vector<o2::framework::ConfigParamSpec>& workflowOptions)
{
  workflowOptions.push_back(
    o2::framework::ConfigParamSpec{ "config-path", o2::framework::VariantType::String, "", { "Path to the config file. Overwrite the default paths. Do not use with no-data-sampling." } });
  workflowOptions.push_back(
    o2::framework::ConfigParamSpec{ "no-data-sampling", o2::framework::VariantType::Bool, false, { "Skips data sampling, connects directly the task to the producer." } });
}

#include "Framework/runDataProcessing.h"

std::string getConfigPath(const o2::framework::ConfigContext& config);

o2::framework::WorkflowSpec defineDataProcessing(o2::framework::ConfigContext const& config)
{
  o2::framework::WorkflowSpec specs;
  using digitInputType = std::vector<o2::emcal::Digit>;
  specs.push_back(o2::emcal::getPublisherSpec<digitInputType>(o2::emcal::PublisherConf{
                                                                "emcal-digit-reader",
                                                                "o2sim",
                                                                { "digitbranch", "EMCALDigit", "Digit branch" },
                                                                { "triggerrecordbranch", "EMCALDigitTRGR", "Trigger record branch" },
                                                                { "mcbranch", "EMCALDigitMCTruth", "MC label branch" },
                                                                o2::framework::OutputSpec{ "EMC", "DIGITS" },
                                                                o2::framework::OutputSpec{ "EMC", "DIGITSTRGR" },
                                                                o2::framework::OutputSpec{ "EMC", "DIGITSMCTR" } },
                                                              false));

  // Path to the config file
  std::string qcConfigurationSource = getConfigPath(config);

  // Generation of the QC topology
  o2::quality_control::generateRemoteInfrastructure(specs, qcConfigurationSource);

  /*
  o2::framework::DataProcessorSpec printer{
    "printer",
    o2::framework::Inputs{
      {"checked-mo", "QC", o2::quality_control::checker::CheckRunner::createCheckRunnerDataDescription("DigitsQcTask"), 0}
    },
    o2::framework::Outputs{},
    o2::framework::AlgorithmSpec{
      (o2::framework::AlgorithmSpec::InitCallback) [](o2::framework::InitContext& initContext) {

        return (o2::framework::AlgorithmSpec::ProcessCallback) [](o2::framework::ProcessingContext& processingContext) mutable {
          std::shared_ptr<TObjArray> moArray{
            std::move(o2::framework::DataRefUtils::as<TObjArray>(*processingContext.inputs().begin()))
          };

          for (const auto& to : *moArray) {
            MonitorObject* mo = dynamic_cast<MonitorObject*>(to);

            if (mo->getName() == "example") {
              auto* g = dynamic_cast<TH1F*>(mo->getObject());
              std::string bins = "BINS:";
              for (int i = 0; i < g->GetNbinsX(); i++) {
                bins += " " + std::to_string((int) g->GetBinContent(i));
              }
              LOG(INFO) << bins;
            }
          }
        };
      }
    }
  };
  specs.push_back(printer);
*/
  LOG(INFO) << "Using config file '" << qcConfigurationSource << "'";
  o2::framework::DataSampling::GenerateInfrastructure(specs, qcConfigurationSource);

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
