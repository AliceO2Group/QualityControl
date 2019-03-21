///
/// \file   ITSQCTask.cxx
/// \author Barthelemy von Haller
/// \author Piotr Konopka
///

#include <sstream>

#include <TStopwatch.h>
#include "DataFormatsParameters/GRPObject.h"
#include "FairLogger.h"
#include "FairRunAna.h"
#include "FairFileSource.h"
#include "FairRuntimeDb.h"
#include "FairParRootFileIo.h"
#include "FairSystemInfo.h"



#include "../../../O2/Detectors/ITSMFT/common/reconstruction/include/ITSMFTReconstruction/DigitPixelReader.h"
#include "DetectorsBase/GeometryManager.h"
//#include "ITSBase/GeometryTGeo.h"
#include <TCanvas.h>


#include "QualityControl/QcInfoLogger.h"
#include "ITSQCTask/ITSQCTask.h"



using o2::ITSMFT::Digit;
using Segmentation = o2::ITSMFT::SegmentationAlpide;


using namespace std;
using namespace o2::ITSMFT;
using namespace o2::ITS;




namespace o2
{
	namespace quality_control_modules
	{
		namespace itsqctask
		{

			//		o2::Base::GeometryManager::loadGeometry();

			ITSQCTask::ITSQCTask() : TaskInterface(), mHistogram(nullptr) { 
				mHistogram = nullptr;
				gSystem->Load("/data/zhaozhong/alice/sw/slc7_x86-64/O2/1.0.0-1/lib/libITSBase.so");
				gSystem->Load("/data/zhaozhong/alice/sw/slc7_x86-64/O2/1.0.0-1/lib/libITSSimulation.so");
				o2::Base::GeometryManager::loadGeometry();
				ChipStave->GetXaxis ()->SetTitle ("Chip ID");
				ChipStave->GetYaxis ()->SetTitle ("Number of Hits");
				ChipStave->SetTitle ("Occupancy for ITS Layer 1");

				ChipProj->GetXaxis ()->SetTitle ("Chip ID");
				ChipStave->GetYaxis ()->SetTitle ("Average Number of Hits");
				ChipStave->SetTitle ("Occupancy Projection for ITS Layer 1");

				Lay1EtaPhi->GetXaxis()->SetTitle("#eta");
				Lay1EtaPhi->GetYaxis()->SetTitle("#phi");
				Lay1EtaPhi->GetZaxis()->SetTitle("Number of Hits");
				Lay1EtaPhi->GetZaxis()->SetTitleOffset(0.07);

				Lay1EtaPhi->SetTitle("Number of Hits for Layer 1 #eta and #phi Distribution");

				cout << "Clear " << endl;
			}

			ITSQCTask::~ITSQCTask() {
				if (mHistogram) {
					delete mHistogram;
				}
			}

			void ITSQCTask::initialize(o2::framework::InitContext& ctx)
			{


				QcInfoLogger::GetInstance() << "initialize ITSQCTask" << AliceO2::InfoLogger::InfoLogger::endm;

				//auto filename = ctx.options().get < std::string > ("itsdigits.root");
				auto filename = ctx.options().get<std::string>("its-digits");
				//std::unique_ptr<TFile> mFile = nullptr;
				//mFile = std::make_unique<TFile>(filename.c_str(), "OLD");

				QcInfoLogger::GetInstance() << "Input File Name is " << filename.c_str () <<  AliceO2::InfoLogger::InfoLogger::endm;
				QcInfoLogger::GetInstance() << "It WORK, we start plotting histograms" <<  AliceO2::InfoLogger::InfoLogger::endm;

				/*
				   std::vector<ChipPixelData> mChips;
				   std::vector<ChipPixelData> mChipsOld;
				   o2::ITSMFT::PixelReader* mReader = nullptr; 
				   std::unique_ptr<o2::ITSMFT::DigitPixelReader> mReaderMC;   
				   const std::string inpName = "itsdigits.root";
				   */
				bool mRawDataMode = 0;

				if (mRawDataMode)
				{

					int a = 1;
				}
				else
				{				
					mReaderMC = std::make_unique < o2::ITSMFT::DigitPixelReader > ();
					mReader = mReaderMC.get ();
				}

				o2::ITS::GeometryTGeo * geom = o2::ITS::GeometryTGeo::Instance ();
				geom->fillMatrixCache (o2::utils::bit2Mask (o2::TransformType::L2G));	

				QcInfoLogger::GetInstance() << "It WORK, PASS 3" <<  AliceO2::InfoLogger::InfoLogger::endm;

				const Int_t numOfChips = geom->getNumberOfChips ();

				QcInfoLogger::GetInstance() << "numOfChips = " << numOfChips <<  AliceO2::InfoLogger::InfoLogger::endm;

				setNChips (numOfChips);
				int IndexNow = 0;

				QcInfoLogger::GetInstance() << "START LOOPING BRO" <<  AliceO2::InfoLogger::InfoLogger::endm;

				mReaderMC->openInput ("itsdigits.root", o2::detectors::DetID ("ITS"));



				while (mReaderMC->readNextEntry())
				{
					QcInfoLogger::GetInstance()  << "Now Working on Event = " << IndexNow << AliceO2::InfoLogger::InfoLogger::endm;
					process (*mReader);
					IndexNow = IndexNow + 1;
				}


				TCanvas *c = new TCanvas ("c", "c", 600, 600);
				c->cd ();

				ChipStave->Draw ("colz");
				c->SaveAs ("Occupancy.png");
				QcInfoLogger::GetInstance() << "Plot Draw" << AliceO2::InfoLogger::InfoLogger::endm;

				TH1D *Proj = new TH1D ("Proj", "CProj", NEventMax, 0, NEventMax);
				for (int i = 0; i < NLay1; i++)
				{
					int XBin = ChipStave->GetXaxis ()->FindBin (i);
					ChipStave->ProjectionY ("Proj", i, i);
					ChipProj->SetBinContent (i, Proj->GetMean ());
					ChipProj->SetBinError (i, Proj->GetRMS () / Proj->Integral ());
				}
				ChipProj->SetMarkerStyle (22);
				ChipProj->SetMarkerSize (1.5);
				ChipProj->Draw ("ep");
				c->SaveAs ("OccupancyProj.png");

				QcInfoLogger::GetInstance() << "WE FUCKING START PUBLISHING NOW BRO IT WORK BRO!!!" << AliceO2::InfoLogger::InfoLogger::endm;

				getObjectsManager()->startPublishing(ChipProj);
				getObjectsManager()->addCheck(ChipProj, "checkFromITSQCTask", "o2::quality_control_modules::itsqctask::ITSQCCheck","QcITSQCTask");

				mHistogram = new TH1F("example", "example", 20, 0, 30000);

				getObjectsManager()->startPublishing(ChipStave);
				getObjectsManager()->addCheck(ChipStave, "checkFromITSQCTask", "o2::quality_control_modules::itsqctask::ITSQCCheck","QcITSQCTask");
				Lay1EtaPhi->Draw("COLZ");
				c->SaveAs("EtaPhiLay1.png");

				//	getObjectsManager()->startPublishing(mHistogram);
				//		getObjectsManager()->addCheck(mHistogram, "checkFromITSQCTask", "o2::quality_control_modules::itsqctask::ITSQCCheck","QcITSQCTask");
				QcInfoLogger::GetInstance() << "DONE BRO!!!" << AliceO2::InfoLogger::InfoLogger::endm;
			}

			void ITSQCTask::process (PixelReader & reader)
			{


				QcInfoLogger::GetInstance()  << "START PROCESSING" <<  AliceO2::InfoLogger::InfoLogger::endm;



				for (int i = 0; i < NLay1; i++)
				{
					Occupancy[i] = 0;
				}
				//cout << "START MCHIPDATA" << endl;
				while ((mChipData = reader.getNextChipData (mChips)))
				{
					//	cout << "ChipID Before = " << ChipID << endl; 
					ChipID = mChipData->getChipID ();

					gm->getChipId (ChipID, lay, sta, ssta, mod, chip);
					// lay =  gm->GetLayer(ChipID);
					// sta =  gm->getStave(ChipID);
					gm->fillMatrixCache(o2::utils::bit2Mask(o2::TransformType::L2G));
					const Point3D<float> loc(0., 0.,0.); 
					auto glo = gm->getMatrixL2G(ChipID)(loc);
			
					if (lay < 1)
					{

						//	QcInfoLogger::GetInstance()  << "ChipID = " << ChipID <<  AliceO2::InfoLogger::InfoLogger::endm;
						ActPix = mChipData->getData ().size ();
						eta = glo.eta();
						phi = glo.phi();
						Occupancy[ChipID] = Occupancy[ChipID] + ActPix;
						Lay1EtaPhi->Fill(eta,phi,ActPix);
					}
				}
				QcInfoLogger::GetInstance()  << "Start Filling" <<  AliceO2::InfoLogger::InfoLogger::endm;
				for (int i = 0; i < NLay1; i++)
				{
					int XBin = ChipStave->GetXaxis ()->FindBin (i);
					AveOcc = Occupancy[i] / NPixels;
					ChipStave->Fill (XBin, Occupancy[i]);
				}


			}




			void ITSQCTask::startOfActivity(Activity& activity)
			{
				QcInfoLogger::GetInstance() << "startOfActivity" << AliceO2::InfoLogger::InfoLogger::endm;
				mHistogram->Reset();
			}

			void ITSQCTask::startOfCycle()
			{
				QcInfoLogger::GetInstance() << "startOfCycle" << AliceO2::InfoLogger::InfoLogger::endm;
			}

			void ITSQCTask::monitorData(o2::framework::ProcessingContext& ctx)
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
	/*

				cout << "START Here" << endl;
				auto digits = ctx.inputs().get<const std::vector<o2::ITSMFT::Digit>>("digits");
				auto labels = ctx.inputs().get<const o2::dataformats::MCTruthContainer<o2::MCCompLabel>*>("labels");
				auto rofs = ctx.inputs().get<const std::vector<o2::ITSMFT::ROFRecord>>("ROframes");
				auto mc2rofs = ctx.inputs().get<const std::vector<o2::ITSMFT::MC2ROFRecord>>("MC2ROframes");

				LOG(INFO) << "ITSClusterer pulled " << digits.size() << " digits, "
					<< labels->getIndexedSize() << " MC label objects, in "
					<< rofs.size() << " RO frames and "
					<< mc2rofs.size() << " MC events";
	*/
				for (auto&& input : ctx.inputs()) {
					// get message header
					const auto* header = header::get<header::DataHeader*>(input.header);
					//	const auto*  headername = header::get<header::HeaderType*>(input.header);

					mHistogram->Fill(header->payloadSize);

					//QcInfoLogger::GetInstance() << "header = "  <<  headername <<  AliceO2::InfoLogger::InfoLogger::endm;

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

			void ITSQCTask::endOfCycle()
			{
				QcInfoLogger::GetInstance() << "endOfCycle" << AliceO2::InfoLogger::InfoLogger::endm;
			}

			void ITSQCTask::endOfActivity(Activity& activity)
			{
				QcInfoLogger::GetInstance() << "endOfActivity" << AliceO2::InfoLogger::InfoLogger::endm;
			}

			void ITSQCTask::reset()
			{
				// clean all the monitor objects here

				QcInfoLogger::GetInstance() << "Resetting the histogram" << AliceO2::InfoLogger::InfoLogger::endm;
				mHistogram->Reset();
			}

		} // namespace itsqctask
	} // namespace quality_control_modules
} // namespace o2



