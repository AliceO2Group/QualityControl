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
  const std::string qcConfigurationSource =
    std::string("json://") + getenv("QUALITYCONTROL_ROOT") + "/etc/readoutChainTemplate.json";

  LOG(INFO) << "Using config file '" << qcConfigurationSource << "'";
  o2::framework::DataSampling::GenerateInfrastructure(specs, qcConfigurationSource);

  return specs;
}
