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
#if __has_include(<Framework/DataSampling.h>)
#include <Framework/DataSampling.h>
#else
#include <DataSampling/DataSampling.h>
using namespace o2::utilities;
#endif
#include "QualityControl/InfrastructureGenerator.h"
#include "QualityControl/QcInfoLogger.h"

using namespace o2;
using namespace o2::framework;

// The customize() functions are used to declare the executable arguments and to specify custom completion and channel
// configuration policies. They have to be above `#include "Framework/runDataProcessing.h"` - that header checks if
// these functions are defined by user and if so, it invokes them. It uses a trick with SFINAE expressions to do that.

void customize(std::vector<ConfigParamSpec>& workflowOptions)
{
  workflowOptions.push_back(
    ConfigParamSpec{ "config", VariantType::String, "", { "Path to QC and Data Sampling configuration file." } });

  workflowOptions.push_back(
    ConfigParamSpec{ "local", VariantType::Bool, false, { "Creates only the local part of the QC topology." } });
  workflowOptions.push_back(
    ConfigParamSpec{ "host", VariantType::String, "", { "Name of the host of the local part of the QC topology."
                                                        "Necessary to specify when creating topologies on multiple"
                                                        " machines. If not specified, hostname of the current machine"
                                                        " will be used" } });
  workflowOptions.push_back(
    ConfigParamSpec{ "remote", VariantType::Bool, false, { "Creates only the remote part of the QC topology." } });
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

WorkflowSpec defineDataProcessing(const ConfigContext& config)
{
  WorkflowSpec specs;

  const std::string qcConfigurationSource = config.options().get<std::string>("config");
  ILOG(Info) << "Using config file '" << qcConfigurationSource << "'" << ENDM;

  // The QC infrastructure is divided into two parts:
  // - local - QC tasks which are on the same machines as the main processing. We also put Data Sampling there.
  // - remote - QC tasks, mergers and checkers that reside on QC servers
  //
  // The user can specify to create either one of these parts by selecting corresponding option,
  // or both of them, which is the default option (no flags needed).

  if (!config.options().get<bool>("local") && !config.options().get<bool>("remote")) {
    ILOG(Info) << "Creating a standalone QC topology." << ENDM;
    DataSampling::GenerateInfrastructure(specs, qcConfigurationSource);
    quality_control::generateStandaloneInfrastructure(specs, qcConfigurationSource);
  }

  if (config.options().get<bool>("local")) {
    ILOG(Info) << "Creating a local QC topology." << ENDM;

    // Generation of Data Sampling infrastructure
    DataSampling::GenerateInfrastructure(specs, qcConfigurationSource);

    // Generation of the local QC topology (local QC tasks and their output proxies)
    auto host = config.options().get<std::string>("host").empty()
                  ? boost::asio::ip::host_name()
                  : config.options().get<std::string>("host");
    quality_control::generateLocalInfrastructure(specs, qcConfigurationSource, host);
  }
  if (config.options().get<bool>("remote")) {
    ILOG(Info) << "Creating a remote QC topology." << ENDM;

    // Generation of the remote QC topology (task for QC servers, input proxies, mergers and all check runners)
    quality_control::generateRemoteInfrastructure(specs, qcConfigurationSource);
  }

  return specs;
}
