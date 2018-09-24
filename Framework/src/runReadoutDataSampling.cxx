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

/// \file    runReadoutDataSampling.cxx
/// \author  Piotr Konopka, Barthelemy von Haller
///
/// \brief This is a simplistic executable to be able to sample Readout data towards a non-DPL FairMQ device.
///
/// It uses a config file located at
/// ${QUALITYCONTROL_ROOT}/etc/readoutDataSampling.json or Framework/readoutDataSampling.json (original one).
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

#include <TH1F.h>

#include "Framework/DataSampling.h"
#include "Framework/runDataProcessing.h"

using namespace o2::framework;

WorkflowSpec defineDataProcessing(ConfigContext const&)
{
  WorkflowSpec specs;

  const std::string qcConfigurationSource =
    std::string("json://") + getenv("QUALITYCONTROL_ROOT") + "/etc/readoutDataSampling.json";
  LOG(INFO) << "Using config file '" << qcConfigurationSource << "'";

  o2::framework::DataSampling::GenerateInfrastructure(specs, qcConfigurationSource);

  return specs;
}
