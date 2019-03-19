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
/// \file    runBasic.cxx
/// \author  Piotr Konopka
///
/// \brief This is an executable showing QC Task's usage in Data Processing Layer.
///
/// This is an executable showing QC Task's usage in Data Processing Layer. The workflow consists of data producer,
/// which generates arrays of random size and content. Its output is dispatched to QC task using Data Sampling
/// infrastructure. QC Task runs exemplary user code located in SkeletonDPL. The checker performes a simple check of
/// the histogram shape and colorizes it. The resulting histogram contents are shown in logs by printer.
///
/// QC task and Checker are instantiated by respectively TaskFactory and CheckerFactory,
/// which use preinstalled config file, that can be found in
/// ${QUALITYCONTROL_ROOT}/etc/qcTaskDplConfig.json or Framework/qcTaskDplConfig.json (original one).
///
/// To launch it, build the project, load the environment and run the executable:
///   \code{.sh}
///   > aliBuild build QualityControl --defaults o2
///   > alienv enter QualityControl/latest
///   > runTaskDPL
///   \endcode
/// If you have glfw installed, you should see a window with the workflow visualization and sub-windows for each Data
/// Processor where their logs can be seen. The processing will continue until the main window it is closed. Regardless
/// of glfw being installed or not, in the terminal all the logs will be shown as well.

#include "Framework/DataSampling.h"


using namespace o2::framework;
void customize(std::vector<CompletionPolicy>& policies)
{
	DataSampling::CustomizeInfrastructure(policies);
}
void customize(std::vector<ChannelConfigurationPolicy>& policies)
{
	DataSampling::CustomizeInfrastructure(policies);
}






#include <FairLogger.h>
#include <TH1F.h>
#include <memory>
#include <random>

#include "Framework/runDataProcessing.h"

#include "QualityControl/Checker.h"
#include "QualityControl/InfrastructureGenerator.h"
#include "DetectorsBase/Propagator.h"
#include "Framework/WorkflowSpec.h"
#include "Framework/ConfigParamSpec.h"
#include "Framework/CompletionPolicy.h"
#include "Framework/DeviceSpec.h"
#include "DetectorsCommonDataFormats/DetID.h"
#include "Framework/runDataProcessing.h"
#include "/data/zhaozhong/alice/O2/Detectors/ITSMFT/ITS/workflow/include/ITSWorkflow/DigitReaderSpec.h"
#include "/data/zhaozhong/alice/O2/Detectors/ITSMFT/ITS/workflow/include/ITSWorkflow/RecoWorkflow.h"
#include "/data/zhaozhong/alice/O2/Detectors/ITSMFT/ITS/workflow/src/DigitReaderSpec.cxx"
//#include "/data/zhaozhong/alice/O2/Detectors/ITSMFT/ITS/QCWorkFlow/include/ITSQCWorkflow/HisAnalyzerSpec.h"
//#include "/data/zhaozhong/alice/O2/Detectors/ITSMFT/ITS/QCWorkFlow/src/HisAnalyzerSpec.cxx"

#include "/data/zhaozhong/alice/O2/Detectors/ITSMFT/ITS/workflow/src/DummySpec.cxx"
//#include "ITSDIGIRECOWorkflow/HisAnalyzerSpec.h"
/*
#include "/data/zhaozhong/alice/O2/Steer/DigitizerWorkflow/src/ITSMFTDigitizerSpec.h"
#include "/data/zhaozhong/alice/O2/Steer/DigitizerWorkflow/src/ITSMFTDigitWriterSpec.h"
#include "/data/zhaozhong/alice/O2/Steer/DigitizerWorkflow/src/ITSMFTDigitWriterSpec.cxx"
#include "/data/zhaozhong/alice/O2/Steer/DigitizerWorkflow/src/ITSMFTDigitizerSpec.cxx"
*/
#include "DetectorsBase/GeometryManager.h"
#include "ITSBase/GeometryTGeo.h"

using namespace o2;
using namespace o2::framework;
using namespace o2::quality_control::checker;
using namespace std::chrono;

WorkflowSpec defineDataProcessing(ConfigContext const&)
{

	WorkflowSpec specs;


	const std::string qcConfigurationSource = std::string("json://") + getenv("QUALITYCONTROL_ROOT") + "/etc/PrintTest.json";
	o2::Base::GeometryManager::loadGeometry();

	LOG(INFO) << "START READER";


	specs.emplace_back(o2::ITS::getDigitReaderSpec());
	//specs.emplace_back(o2::ITS::getDummySpec());

	LOG(INFO) << "DONE READER";


	LOG(INFO) << "Using config file '" << qcConfigurationSource << "'";

	LOG(INFO) << "START INFRASTRUCTURE ";

	// Generation of Data Sampling infrastructure
	DataSampling::GenerateInfrastructure(specs, qcConfigurationSource);


	LOG(INFO) << "DONE INFRASTRUCTURE ";

	//std::string	detStrL = "its";
	// Generation of the QC topology (one task, one checker in this case)
//	quality_control::generateRemoteInfrastructure(specs, qcConfigurationSource);

	LOG(INFO) << "START PRINTING PROCESS NOW ";
	// Finally the printer

	return specs;
}
