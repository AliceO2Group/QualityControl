// Copyright CERN and copyright holders of ALICE O2. This software is
// distributed under the terms of the GNU General Public License v3 (GPL
// Version 3), copied verbatim in the file "COPYING".
//
// See http://alice-o2.web.cern.ch/license for full licensing information.
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.
//

/// \file    runReadoutForDataDump.cxx
/// \author  Piotr Konopka, Barthelemy von Haller
///
/// \brief This is a simplistic executable to be able to sample Readout data towards a non-DPL FairMQ device.
///
/// It uses a config file located at
/// ${QUALITYCONTROL_ROOT}/etc/readoutForDataDump.json or Framework/readoutForDataDump.json (original one).
/// The only thing that might have to be changed is the port (default : 26525) on which the data is sent.
/// \code{.json}
/// > "channelConfig": "name=fairReadoutRawOut,type=pub,method=bind,address=tcp://127.0.0.1:26525,rateLogging=1"
/// \endcode
///
/// To launch it, build the project, load the environment and run the executable:
///   \code{.sh}
///   > aliBuild build QualityControl --defaults o2
///   > alienv enter QualityControl/latest
///   > runReadoutDataSampling
///   \endcode
///
/// If you have glfw installed, you should see a window with the workflow visualization and sub-windows for each Data
/// Processor where their logs can be seen. The processing will continue until the main window is closed. Regardless
/// of glfw being installed or not, in the terminal all the logs will be shown as well.
///

#include <DataSampling/DataSampling.h>

using namespace o2::framework;
using namespace o2::utilities;

void customize(std::vector<CompletionPolicy>& policies)
{
  DataSampling::CustomizeInfrastructure(policies);
}
void customize(std::vector<ChannelConfigurationPolicy>& policies)
{
  DataSampling::CustomizeInfrastructure(policies);
}

#include <Framework/runDataProcessing.h>
#include <DataSampling/DataSamplingReadoutAdapter.h>
#include "QualityControl/QcInfoLogger.h"

#include <string>

WorkflowSpec defineDataProcessing(ConfigContext const&)
{
  WorkflowSpec specs{
    specifyExternalFairMQDeviceProxy(
      "readout-proxy",
      Outputs{ { "RO", "RAWDATA" } },
      "type=sub,method=connect,address=ipc:///tmp/readout-pipe-1,rateLogging=1",
      dataSamplingReadoutAdapter({ "RO", "RAWDATA" }))
  };

  const std::string qcConfigurationSource =
    std::string("json://") + getenv("QUALITYCONTROL_ROOT") + "/etc/readoutForDataDump.json";
  ILOG(Info) << "Using config file '" << qcConfigurationSource << "'";

  DataSampling::GenerateInfrastructure(specs, qcConfigurationSource);

  return specs;
}
