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
/// \file    runQC.cxx
/// \author  Piotr Konopka
///
/// \brief This is an executable which generates a QC topology given a configuration file.
///
/// This is an executable which generates a QC topology given a configuration file. It can be attached to any other
/// topology which can provide data to Data Sampling and QC. This also means that cannot work on its own, as it would
/// lack input data. A typical usage would be:
/// \code{.sh}
/// o2-qc-run-producer | o2-qc --config json://${QUALITYCONTROL_ROOT}/etc/basic.json
/// \endcode
/// Please refer to Framework/example-default.json and Framework/basic.json to see how to configure a QC topology.
/// To generate only the local part of the topology (which would run on main processing servers) use the '--local' flag.
/// Similarly, to generate only the remote part (running on QC servers) add '--remote'. By default, the executable
/// generates both local and remote topologies, as it is the usual use-case for local development.

#include <boost/asio/ip/host_name.hpp>
#include <DataSampling/DataSampling.h>
#include <Configuration/ConfigurationFactory.h>
#include <Configuration/ConfigurationInterface.h>
#include "QualityControl/InfrastructureGenerator.h"
#include "QualityControl/QcInfoLogger.h"

using namespace o2;
using namespace o2::framework;
using namespace o2::utilities;
using namespace o2::configuration;

// The customize() functions are used to declare the executable arguments and to specify custom completion and channel
// configuration policies. They have to be above `#include "Framework/runDataProcessing.h"` - that header checks if
// these functions are defined by user and if so, it invokes them. It uses a trick with SFINAE expressions to do that.

void customize(std::vector<ConfigParamSpec>& workflowOptions)
{
  workflowOptions.push_back(
    ConfigParamSpec{ "config", VariantType::String, "", { "Absolute path to QC and Data Sampling configuration file." } });

  workflowOptions.push_back(
    ConfigParamSpec{ "local", VariantType::Bool, false, { "Runs only the local part of the QC workflow." } });
  workflowOptions.push_back(
    ConfigParamSpec{ "host", VariantType::String, "", { "Name of the host of the local part of the QC workflow. "
                                                        "Necessary to specify when creating workflows on multiple"
                                                        " machines. If not specified, hostname of the current machine"
                                                        " will be used" } });
  workflowOptions.push_back(
    ConfigParamSpec{ "remote", VariantType::Bool, false, { "Runs only the remote part of the QC workflow." } });
  workflowOptions.push_back(
    ConfigParamSpec{ "no-data-sampling", VariantType::Bool, false, { "Do not add Data Sampling infrastructure." } });

  workflowOptions.push_back(
    ConfigParamSpec{ "local-batch", VariantType::String, "", { "Runs the local part of the QC workflow and dumps results to a file. "
                                                               "Takes the file path as argument. If it exists, the results are merged. "
                                                               "Do not run many QC workflows on the same file at the same time." } });
  workflowOptions.push_back(
    ConfigParamSpec{ "remote-batch", VariantType::String, "", { "Runs the remote part of the QC workflow reading the inputs from a file (files). "
                                                                "Takes the file path as argument." } });
}

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

using namespace std::chrono;

bool validateArguments(const ConfigContext& config)
{
  const std::string qcConfigurationSource = config.options().get<std::string>("config");
  if (qcConfigurationSource.empty()) {
    ILOG(Warning, Support) << "No configuration path specified, returning an empty workflow." << ENDM;
    return false;
  }

  size_t exclusiveOptions = 0;
  exclusiveOptions += config.options().get<bool>("local");
  exclusiveOptions += config.options().get<bool>("remote");
  exclusiveOptions += !config.options().get<std::string>("local-batch").empty();
  exclusiveOptions += !config.options().get<std::string>("remote-batch").empty();
  if (exclusiveOptions > 1) {
    ILOG(Error, Support) << "More than one of the following options was specified: --local, --remote, --local-batch, --remote--batch. This is not allowed, returning an empty workflow." << ENDM;
    return false;
  }

  return true;
}

enum class WorkflowType {
  Standalone,
  Local,
  Remote,
  LocalBatch,
  RemoteBatch
};

WorkflowType getWorkflowType(const ConfigContext& config)
{
  if (config.options().get<bool>("local")) {
    return WorkflowType::Local;
  } else if (config.options().get<bool>("remote")) {
    return WorkflowType::Remote;
  } else if (!config.options().get<std::string>("local-batch").empty()) {
    return WorkflowType::LocalBatch;
  } else if (!config.options().get<std::string>("remote-batch").empty()) {
    return WorkflowType::RemoteBatch;
  } else {
    return WorkflowType::Standalone;
  }
}

WorkflowSpec defineDataProcessing(const ConfigContext& config)
{
  WorkflowSpec specs;

  if (!validateArguments(config)) {
    return {};
  }

  const std::string qcConfigurationSource = config.options().get<std::string>("config");
  ILOG(Info, Support) << "Using config file '" << qcConfigurationSource << "'" << ENDM;

  try {
    // The online QC infrastructure is divided into two parts:
    // - local - QC tasks which are on the same machines as the main processing. We also put Data Sampling there.
    // - remote - QC tasks, mergers and checkers that reside on QC servers
    //
    // The user can specify to create either one of these parts by selecting corresponding option,
    // or both of them, which is the default option (no flags needed).
    //
    // For file-based processing, there are also:
    // - local-batch - QC tasks are run, the results are stored in the specified file. If the file exists,
    //                 QC objects are merged. Multiple local-batch workflows should not run at the same time,
    //                 as they would modify the same file.
    // - remote-batch - Checks and Aggregators are run on the QC objects inside a file created by a local-batch workflow.
    //                  The results are stored in the database specified in the config file.

    auto workflowType = getWorkflowType(config);
    switch (workflowType) {
      case WorkflowType::Standalone: {
        ILOG(Info, Support) << "Creating a standalone QC workflow." << ENDM;

        if (!config.options().get<bool>("no-data-sampling")) {
          ILOG(Info, Support) << "Generating Data Sampling" << ENDM;
          auto configInterface = ConfigurationFactory::getConfiguration(qcConfigurationSource);
          auto dataSamplingTree = configInterface->getRecursive("dataSamplingPolicies");
          DataSampling::GenerateInfrastructure(specs, dataSamplingTree);
        } else {
          ILOG(Info, Support) << "Omitting Data Sampling" << ENDM;
        }
        quality_control::generateStandaloneInfrastructure(specs, qcConfigurationSource);
        break;
      }
      case WorkflowType::Local: {
        ILOG(Info, Support) << "Creating a local QC topology." << ENDM;
        auto host = config.options().get<std::string>("host").empty()
                      ? boost::asio::ip::host_name()
                      : config.options().get<std::string>("host");

        if (!config.options().get<bool>("no-data-sampling")) {
          ILOG(Info, Support) << "Generating Data Sampling" << ENDM;
          auto configInterface = ConfigurationFactory::getConfiguration(qcConfigurationSource);
          auto dataSamplingTree = configInterface->getRecursive("dataSamplingPolicies");
          DataSampling::GenerateInfrastructure(specs, dataSamplingTree, 1, host);
        } else {
          ILOG(Info, Support) << "Omitting Data Sampling" << ENDM;
        }

        // Generation of the local QC topology (local QC tasks and their output proxies)
        quality_control::generateLocalInfrastructure(specs, qcConfigurationSource, host);
        break;
      }
      case WorkflowType::Remote: {
        ILOG(Info, Support) << "Creating a remote QC workflow." << ENDM;

        // Generation of the remote QC topology (task for QC servers, input proxies, mergers and all check runners)
        quality_control::generateRemoteInfrastructure(specs, qcConfigurationSource);
        break;
      }
      case WorkflowType::LocalBatch: {
        ILOG(Info, Support) << "Creating a local batch QC workflow." << ENDM;
        if (!config.options().get<bool>("no-data-sampling")) {
          ILOG(Info, Support) << "Generating Data Sampling" << ENDM;
          auto configInterface = ConfigurationFactory::getConfiguration(qcConfigurationSource);
          auto dataSamplingTree = configInterface->getRecursive("dataSamplingPolicies");
          DataSampling::GenerateInfrastructure(specs, dataSamplingTree);
        } else {
          ILOG(Info, Support) << "Omitting Data Sampling" << ENDM;
        }

        auto localBatchFilePath = config.options().get<std::string>("local-batch");
        // Generation of the local batch QC workflow (QC tasks and file sink)
        quality_control::generateLocalBatchInfrastructure(specs, qcConfigurationSource, localBatchFilePath);
        break;
      }
      case WorkflowType::RemoteBatch: {
        auto remoteBatchFilePath = config.options().get<std::string>("remote-batch");
        // Creating the remote batch QC topology (file reader, check runners, aggregator runners, postprocessing)
        quality_control::generateRemoteBatchInfrastructure(specs, qcConfigurationSource, remoteBatchFilePath);
        break;
      }
    }
  } catch (const std::runtime_error& re) {
    ILOG(Fatal, Ops) << "Failed to build the workflow: " << re.what() << ENDM;
    return {};
  }

  return specs;
}
