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
/// \file    runReadoutChainTemplate.cxx
/// \author  Piotr Konopka
///
/// \brief This is an executable showing QC Task's usage in Data Processing Layer with Readout as external data source.
///
/// This is an executable showing QC Task's usage with Readout as a data producer. To get the Readout data, a proxy is
/// used. Its output is dispatched to QC task using Data Sampling infrastructure. QC Task runs exemplary user code
/// located in SkeletonDPL. The resulting histogram contents are printed by a fake checker.
/// QC task is instantiated by TaskDataProcessorFactory with preinstalled config file, which can be found in
/// ${QUALITYCONTROL_ROOT}/etc/readoutChainTemplate.json or Framework/readoutChainTemplate.json (original one).
///
/// To launch it, build the project, load the environment and run the executable:
///   \code{.sh}
///   > aliBuild build QualityControl --defaults o2
///   > alienv enter QualityControl/latest
///   > runReadoutChainTemplate
///   \endcode
/// If you have glfw installed, you should see a window with the workflow visualization and sub-windows for each Data
/// Processor where their logs can be seen. The processing will continue until the main window it is closed. Regardless
/// of glfw being installed or not, in the terminal all the logs will be shown as well.

#include <TH1F.h>

#include "Framework/DataSampling.h"
#include "Framework/runDataProcessing.h"
#include "QualityControl/TaskDataProcessorFactory.h"
#include "QualityControl/TaskDataProcessor.h"
#include "QualityControl/CheckerDataProcessorFactory.h"
#include "QualityControl/CheckerDataProcessor.h"

using namespace o2::framework;
using namespace o2::quality_control::core;
using namespace o2::quality_control::checker;

WorkflowSpec defineDataProcessing(ConfigContext const&)
{
  WorkflowSpec specs;

  // Exemplary initialization of QC Task:
  const std::string qcTaskName = "skeletonTask";
  const std::string qcConfigurationSource =
    std::string("json://") + getenv("QUALITYCONTROL_ROOT") + "/etc/readoutChainTemplate.json";
  TaskDataProcessorFactory qcFactory;
  specs.push_back(qcFactory.create(qcTaskName, qcConfigurationSource));
  CheckerDataProcessorFactory checkerFactory;
  specs.push_back(checkerFactory.create("checker_0", qcTaskName, qcConfigurationSource));

  DataProcessorSpec printer{
    "printer",
    Inputs{
      { "checked-mo", "QC", CheckerDataProcessor::checkerDataDescription(qcTaskName), 0 }
    },
    Outputs{},
    AlgorithmSpec{
      (AlgorithmSpec::InitCallback) [](InitContext& initContext) {

        return (AlgorithmSpec::ProcessCallback) [](ProcessingContext& processingContext) mutable {
          auto mo = processingContext.inputs().get<MonitorObject*>("checked-mo").get();

          if (mo->getName() == "example") {
            auto* g = dynamic_cast<TH1F*>(mo->getObject());
            std::string bins = "BINS:";
            for (int i = 0; i < g->GetNbinsX() + 2; i++) {
              bins += " " + std::to_string((int) g->GetBinContent(i));
            }
            LOG(INFO) << bins;
          }
        };
      }
    }
  };
  specs.push_back(printer);

  LOG(INFO) << "Using config file '" << qcConfigurationSource << "'";
  o2::framework::DataSampling::GenerateInfrastructure(specs, qcConfigurationSource);

  return specs;
}
