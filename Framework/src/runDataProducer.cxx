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
/// o2-qc-run-producer | o2-qc-run-qc --config json://${QUALITYCONTROL_ROOT}/etc/basic.json
/// \endcode
///
/// If you have glfw installed, you should see a window with the workflow visualization and sub-windows for each Data
/// Processor where their logs can be seen. The processing will continue until the main window it is closed. Regardless
/// of glfw being installed or not, in the terminal all the logs will be shown as well.

#include <random>
#include "Framework/runDataProcessing.h"

using namespace o2;
using namespace o2::framework;

// clang-format off
WorkflowSpec defineDataProcessing(const ConfigContext&)
{
  WorkflowSpec specs;

  // The producer to generate some data in the workflow
  DataProcessorSpec producer{
    "producer",
    Inputs{},
    Outputs{
      { "TST", "RAWDATA", 0 } },
    AlgorithmSpec{
      (AlgorithmSpec::InitCallback)[](InitContext&){
        // this is the initialization code
        std::default_random_engine generator(11);

        // after the initialization, we return the processing callback
        return (AlgorithmSpec::ProcessCallback)[generator](ProcessingContext & processingContext) mutable
        {
          // everything inside this lambda function is invoked in a loop, because it this Data Processor has no inputs
          usleep(100000);

          size_t length = generator() % 10000;
          auto data = processingContext.outputs().make<char>(Output{ "TST", "RAWDATA" }, length);
          for (auto&& item : data) {
            item = static_cast<char>(generator());
          }
        };
      }
    }
  };
  specs.push_back(producer);

  return specs;
}
// clang-format on
