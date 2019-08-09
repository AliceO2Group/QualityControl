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
/// \brief This is an executable showing how to connect to Readout as external data source.
///
/// This is an executable showing how to connect to Readout as external data source. It consists only of a proxy,
/// which can inject the Readout data into DPL. This workflow is intended to be merged with the QC workflow by doing:
/// \code{.sh}
/// o2-qc-run-readout | o2-qc-run-qc --config json://${QUALITYCONTROL_ROOT}/etc/readout.json
/// \endcode
/// If you do not need to sample data, use the readout-no-sampling.json file instead.
///
/// If you have glfw installed, you should see a window with the workflow visualization and sub-windows for each Data
/// Processor where their logs can be seen. The processing will continue until the main window it is closed. Regardless
/// of glfw being installed or not, in the terminal all the logs will be shown as well.

#include <Framework/DataSamplingReadoutAdapter.h>
#include <Framework/runDataProcessing.h>

using namespace o2;
using namespace o2::framework;

WorkflowSpec defineDataProcessing(ConfigContext const&)
{
  // Creating the Readout proxy
  WorkflowSpec specs{
    specifyExternalFairMQDeviceProxy(
      "readout-proxy",
      Outputs{ { { "readout" }, { "ROUT", "RAWDATA" } } },
      "type=sub,method=connect,address=ipc:///tmp/readout-pipe-1,rateLogging=1",
      dataSamplingReadoutAdapter({ { "readout" }, { "ROUT", "RAWDATA" } }))
  };

  return specs;
}
