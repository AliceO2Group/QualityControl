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
/// \file    runDataProducer.cxx
/// \author  Piotr Konopka
///
/// \brief This is an executable with a basic data producer in Data Processing Layer.
///
/// This is an executable with a basic random data producer in Data Processing Layer. It does not serve a real purpose
/// on its own, but it can be used as a data source for QC development. For example, one can do:
/// \code{.sh}
/// o2-qc-run-producer | o2-qc --config json://${QUALITYCONTROL_ROOT}/etc/basic.json
/// \endcode
/// Check out the help message to see how to configure data rate and message size.
///
/// If you have glfw installed, you should see a window with the workflow visualization and sub-windows for each Data
/// Processor where their logs can be seen. The processing will continue until the main window it is closed. Regardless
/// of glfw being installed or not, in the terminal all the logs will be shown as well.

#include <vector>
#include <Framework/ConfigParamSpec.h>

using namespace o2;
using namespace o2::framework;

void customize(std::vector<ConfigParamSpec>& workflowOptions)
{
  workflowOptions.push_back(
    ConfigParamSpec{ "min-size", VariantType::Int, 1, { "Minimum size in bytes of produced messages." } });
  workflowOptions.push_back(
    ConfigParamSpec{ "max-size", VariantType::Int, 10000, { "Maximum size in bytes of produced messages." } });
  workflowOptions.push_back(
    ConfigParamSpec{ "empty", VariantType::Bool, false, { "Don't fill messages with random data." } });
  workflowOptions.push_back(
    ConfigParamSpec{ "message-rate", VariantType::Double, 10.0, { "Rate of messages per second." } });
  workflowOptions.push_back(
    ConfigParamSpec{ "message-amount", VariantType::Int, 0, { "Amount of messages to be produced in total (0 for inf)." } });
  workflowOptions.push_back(
    ConfigParamSpec{ "producers", VariantType::Int, 1, { "Number of producers. Each will have unique SubSpec, counting from 0." } });
  workflowOptions.push_back(
    ConfigParamSpec{ "monitoring-url", VariantType::String, "", { "URL of the Monitoring backend." } });
  workflowOptions.push_back(
    ConfigParamSpec{ "histo-producers", VariantType::Int, 0, { "Number of histograms producers. Each will have unique SubSpec, counting from 0." } });
}

#include <Framework/runDataProcessing.h>
#include "QualityControl/DataProducer.h"
#include "QualityControl/HistoProducer.h"

using namespace o2::quality_control::core;

WorkflowSpec defineDataProcessing(const ConfigContext& config)
{
  size_t minSize = config.options().get<int>("min-size");
  size_t maxSize = config.options().get<int>("max-size");
  bool fill = !config.options().get<bool>("empty");
  double rate = config.options().get<double>("message-rate");
  uint64_t amount = config.options().get<int>("message-amount");
  size_t producers = config.options().get<int>("producers");
  std::string monitoringUrl = config.options().get<std::string>("monitoring-url");
  size_t histoProducers = config.options().get<bool>("histo-producers");

  WorkflowSpec specs;
  for (size_t i = 0; i < producers; i++) {
    specs.push_back(getDataProducerSpec(minSize, maxSize, rate, amount, i, monitoringUrl, fill));
  }
  for (size_t i = 0; i < histoProducers; i++) {
    specs.push_back(getHistoProducerSpec(i));
  }
  return specs;
}