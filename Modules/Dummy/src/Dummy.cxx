///
/// \file   Dummy.cxx
/// \author Barthelemy von Haller
/// \author Piotr Konopka
///

#include <TCanvas.h>
#include <TH1.h>

#include "QualityControl/QcInfoLogger.h"
#include "/data/zhaozhong/alice/QualityControl/Modules/Dummy/include/Dummy/Dummy.h"
#include "Framework/ControlService.h"
#include "Framework/DataProcessorSpec.h"

#include <sstream>

#include <TStopwatch.h>
#include "DataFormatsParameters/GRPObject.h"
#include "FairLogger.h"
#include "FairRunAna.h"
#include "FairFileSource.h"
#include "FairRuntimeDb.h"
#include "FairParRootFileIo.h"
#include "FairSystemInfo.h"
#include "/data/zhaozhong/alice/O2/Detectors/ITSMFT/ITS/QCWorkFlow/include/ITSQCWorkflow/HisAnalyzerSpec.h"
//#include "/data/zhaozhong/alice/O2/Detectors/ITSMFT/common/base/include/ITSMFTBase/GeometryTGeo.h"

//#include "ITSSimulation/DigitizerTask.h"
//#include "../include/ITSQCWorkflow/HisAnalyzerSpec.h"
#include "/data/zhaozhong/alice/O2/Detectors/ITSMFT/common/base/include/ITSMFTBase/Digit.h"
#include "/data/zhaozhong/alice/O2/Detectors/ITSMFT/common/reconstruction/include/ITSMFTReconstruction/ChipMappingITS.h"

//#include "/data/zhaozhong/alice/O2/Detectors/ITSMFT/common/reconstruction/include/ITSMFTReconstruction/DigitPixelReader.h"
#include "DetectorsBase/GeometryManager.h"
//#include "ITSBase/GeometryTGeo.h"
#include <TCanvas.h>

namespace o2
{
	namespace quality_control_modules
	{
		namespace dummy
		{

			Dummy::Dummy() : TaskInterface(), mHistogram(nullptr) {	
				mHistogram = nullptr; 
				//o2::Base::GeometryManager::loadGeometry ();
				//o2::ITS::GeometryTGeo * geom = o2::ITS::GeometryTGeo::Instance ();
			}

			Dummy::~Dummy() {
				if (mHistogram) {
					delete mHistogram;
				}
			}

			void Dummy::initialize(o2::framework::InitContext& ctx)
			{
				QcInfoLogger::GetInstance() << "initialize Dummy" << AliceO2::InfoLogger::InfoLogger::endm;

				mHistogram = new TH1F("example", "example", 20, 0, 30000);
				getObjectsManager()->startPublishing(mHistogram);
				getObjectsManager()->addCheck(mHistogram, "checkFromDummy", "o2::quality_control_modules::dummy::DummyCheck",	"QcDummy");
			}


			void Dummy::init(o2::framework::InitContext& ic)
			{
				QcInfoLogger::GetInstance() << "START O2 Workflow init" << AliceO2::InfoLogger::InfoLogger::endm;


			}

			void Dummy::run(o2::framework::ProcessingContext& pc)
			{
				QcInfoLogger::GetInstance() << "START O2 Workflow Run" << AliceO2::InfoLogger::InfoLogger::endm;

				auto digits = pc.inputs().get<const std::vector<o2::ITSMFT::Digit>>("digits");
				auto labels = pc.inputs().get<const o2::dataformats::MCTruthContainer<o2::MCCompLabel>*>("labels");
				auto rofs = pc.inputs().get<const std::vector<o2::ITSMFT::ROFRecord>>("ROframes");
				auto mc2rofs = pc.inputs().get<const std::vector<o2::ITSMFT::MC2ROFRecord>>("MC2ROframes");

				LOG(INFO) << "ITSClusterer pulled " << digits.size() << " digits, "
					<< labels->getIndexedSize() << " MC label objects, in "
					<< rofs.size() << " RO frames and "
					<< mc2rofs.size() << " MC events";

				//o2::ITS::GeometryTGeo * geom = o2::ITS::GeometryTGeo::Instance();
				//geom->fillMatrixCache(o2::utils::bit2Mask(o2::TransformType::L2G));
				
			
				/*
	
				o2::ITSMFT::DigitPixelReader reader;
				reader.setDigits(&digits);
				reader.setROFRecords(&rofs);
				reader.setMC2ROFRecords(&mc2rofs);
				reader.setDigitsMCTruth(labels.get());
				reader.init();
				*/
				QcInfoLogger::GetInstance() << "DONE DIGI Initialization" << AliceO2::InfoLogger::InfoLogger::endm;
				
				TH1D * ZZHis = new TH1D("ZZHis","ZZHis",100,0,100);
				ZZHis->Fill(10);
				QcInfoLogger::GetInstance() << "DONE  Preparation" << AliceO2::InfoLogger::InfoLogger::endm;

			// getObjectsManager()->startPublishing(ZZHis);
			//	getObjectsManager()->addCheck(mHistogram, "checkFromDummy", "o2::quality_control_modules::dummy::DummyCheck",	"QcDummy");

				QcInfoLogger::GetInstance() << "DONE Publishing ZZ Histogram" << AliceO2::InfoLogger::InfoLogger::endm;

			}


			void Dummy::startOfActivity(Activity& activity)
			{
				QcInfoLogger::GetInstance() << "startOfActivity" << AliceO2::InfoLogger::InfoLogger::endm;
				mHistogram->Reset();
			}

			void Dummy::startOfCycle()
			{
				QcInfoLogger::GetInstance() << "startOfCycle" << AliceO2::InfoLogger::InfoLogger::endm;
			}

			void Dummy::monitorData(o2::framework::ProcessingContext& ctx)
			{
				// In this function you can access data inputs specified in the JSON config file, for example:
				//  {
				//    "binding": "random",
				//    "dataOrigin": "ITS",
				//    "dataDescription": "RAWDATA"
				//  }

				// Use Framework/DataRefUtils.h or Framework/InputRecord.h to access and unpack inputs (both are documented)
				// One can find additional examples at:
				// https://github.com/AliceO2Group/AliceO2/blob/dev/Framework/Core/README.md#using-inputs---the-inputrecord-api

				// Some examples:

				// 1. In a loop
				for (auto&& input : ctx.inputs()) {
					// get message header
					QcInfoLogger::GetInstance() << "BEEN HERE BRO" << AliceO2::InfoLogger::InfoLogger::endm;
					auto digits = ctx.inputs().get<const std::vector<o2::ITSMFT::Digit>>("digits");
					LOG(INFO) << "ITSClusterer pulled " << digits.size() << " digits, ";

					const auto* header = header::get<header::DataHeader*>(input.header);
					// get payload of a specific input, which is a char array.
					//    const char* payload = input.payload;

					// for the sake of an example, let's fill the histogram with payload sizes
					mHistogram->Fill(header->payloadSize);
				}

				// 2. Using get("<binding>")

				// get the payload of a specific input, which is a char array. "random" is the binding specified in the config file.
				//   auto payload = ctx.inputs().get("random").payload;

				// get payload of a specific input, which is a structure array:
				//  const auto* header = header::get<header::DataHeader*>(ctx.inputs().get("random").header);
				//  struct s {int a; double b;};
				//  auto array = ctx.inputs().get<s*>("random");
				//  for (int j = 0; j < header->payloadSize / sizeof(s); ++j) {
				//    int i = array.get()[j].a;
				//  }

				// get payload of a specific input, which is a root object
				//   auto h = ctx.inputs().get<TH1F*>("histos");
				//   Double_t stats[4];
				//   h->GetStats(stats);
				//   auto s = ctx.inputs().get<TObjString*>("string");
				//   LOG(INFO) << "String is " << s->GetString().Data();
			}

			void Dummy::endOfCycle()
			{
				QcInfoLogger::GetInstance() << "endOfCycle" << AliceO2::InfoLogger::InfoLogger::endm;
			}

			void Dummy::endOfActivity(Activity& activity)
			{
				QcInfoLogger::GetInstance() << "endOfActivity" << AliceO2::InfoLogger::InfoLogger::endm;
			}

			void Dummy::reset()
			{
				// clean all the monitor objects here

				QcInfoLogger::GetInstance() << "Resetting the histogram" << AliceO2::InfoLogger::InfoLogger::endm;
				mHistogram->Reset();
			}

			DataProcessorSpec getDummySpec()
			{
				o2::Base::GeometryManager::loadGeometry();
				return DataProcessorSpec{
					"its-dummy-QC",
						Inputs{
							InputSpec{ "digits", "ITS", "DIGITS", 0, Lifetime::Timeframe },
							InputSpec{ "labels", "ITS", "DIGITSMCTR", 0, Lifetime::Timeframe },
							InputSpec{ "ROframes", "ITS", "ITSDigitROF", 0, Lifetime::Timeframe },
							InputSpec{ "MC2ROframes", "ITS", "ITSDigitMC2ROF", 0, Lifetime::Timeframe } },
						Outputs{
							OutputSpec{ "ITS", "COMPCLUSTERS", 0, Lifetime::Timeframe },
							OutputSpec{ "ITS", "CLUSTERS", 0, Lifetime::Timeframe },
							OutputSpec{ "ITS", "CLUSTERSMCTR", 0, Lifetime::Timeframe },
							OutputSpec{ "ITS", "ITSClusterROF", 0, Lifetime::Timeframe },
							OutputSpec{ "ITS", "ITSClusterMC2ROF", 0, Lifetime::Timeframe } },
						AlgorithmSpec{ adaptFromTask<Dummy>() },
						Options{
							{ "its-dictionary-file", VariantType::String, "complete_dictionary.bin", { "Name of the cluster-topology dictionary file" } } }
				};
			}


		} // namespace dummy
	} // namespace quality_control_modules
} // namespace o2

