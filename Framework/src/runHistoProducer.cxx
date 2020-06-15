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
/// \file    runHistoProducer.cxx
/// \author  Barthélémy von Haller
///
/// \brief This is an executable with a basic histogram producer in the Data Processing Layer.
///
/// This is an executable with a histogram producer in the Data Processing Layer. It does not serve a real purpose
/// on its own, but it can be used as an external data (TObjArray of histograms) source for QC development. For example, one can do:
/// \code{.sh}
/// o2-qc-run-histo-producer | o2-qc --config  json://${QUALITYCONTROL_ROOT}/etc/basic-external-histo.json
/// \endcode
/// Histograms have 100 bins between -3 and 3 and filled+published randomly (incremental) every 2 seconds.
/// They are encapsulated in a TObjArray and named `histo_<index>`.
/// In case, there is no encapsulation, the histogram is named `histo`.
///
/// The option `producers` specifies how many producers to spawn.
/// The option `histograms` specifies how many histograms to publish in each producer.
/// The option `no-tobjarray` is only valid if histograms=1 and will prevent the producer from embedding the histogram in a TObjArray.
/// The option `printer` adds a printer attached to the first producer.
///

#include <vector>
#include <Framework/ConfigParamSpec.h>
#include <iostream>

using namespace o2;
using namespace o2::framework;

void customize(std::vector<ConfigParamSpec>& workflowOptions)
{
  workflowOptions.push_back(
    ConfigParamSpec{ "producers", VariantType::Int, 1, { "Number of histograms producers. Each will have unique SubSpec, counting from 0." } });
  workflowOptions.push_back(
    ConfigParamSpec{ "printer", VariantType::Bool, false, { "Add a printer to output the histograms content." } });
  workflowOptions.push_back(
    ConfigParamSpec{ "histograms", VariantType::Int, 1, { "Number of histograms each producer should produce." } });
  workflowOptions.push_back(
    ConfigParamSpec{ "no-tobjarray", VariantType::Bool, false, { "In case option `histograms=1` do not embed the histogram in a TObjArray." } });
}

#include <Framework/runDataProcessing.h>
#include "QualityControl/HistoProducer.h"

using namespace o2::quality_control::core;

WorkflowSpec defineDataProcessing(const ConfigContext& config)
{
  size_t histoProducers = config.options().get<int>("producers");
  size_t histograms = config.options().get<int>("histograms");
  bool printer = config.options().get<bool>("printer");
  bool noTobjArray = config.options().get<bool>("no-tobjarray");

  if(noTobjArray && histograms > 1) {
    std::cerr << "Option no-tobjarray is only valid if histograms=1." << std::endl;
    exit(1);
  }

  WorkflowSpec specs;
  for (size_t i = 0; i < histoProducers; i++) {
    specs.push_back(getHistoProducerSpec(i, histograms, noTobjArray));
  }

  if(printer) {
    specs.push_back(getHistoPrinterSpec(0));
  }

  return specs;
}