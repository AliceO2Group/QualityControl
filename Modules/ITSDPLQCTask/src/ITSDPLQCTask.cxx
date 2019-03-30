///
/// \file   ITSDPLQCTask.cxx
/// \author Barthelemy von Haller
/// \author Piotr Konopka
///

#include <TCanvas.h>
#include <TH1.h>

#include "QualityControl/QcInfoLogger.h"
#include "ITSDPLQCTask/ITSDPLQCTask.h"

#include <sstream>

#include <TStopwatch.h>
#include "DataFormatsParameters/GRPObject.h"
#include "FairLogger.h"
#include "FairRunAna.h"
#include "FairFileSource.h"
#include "FairRuntimeDb.h"
#include "FairParRootFileIo.h"
#include "FairSystemInfo.h"
//#include "ITSQCWorkflow/HisAnalyzerSpec.h"
//#include "/data/zhaozhong/alice/O2/Detectors/ITSMFT/common/base/include/ITSMFTBase/GeometryTGeo.h"

//#include "ITSSimulation/DigitizerTask.h"
//#include "../include/ITSQCWorkflow/HisAnalyzerSpec.h"


#include "ITSMFTReconstruction/DigitPixelReader.h"
#include "DetectorsBase/GeometryManager.h"
//#include "ITSBase/GeometryTGeo.h"
#include <TCanvas.h>



using o2::ITSMFT::Digit;



using namespace std;
using namespace o2::ITSMFT;
using namespace o2::ITS;



namespace o2
{
	namespace quality_control_modules
	{
		namespace itsdplqctask
		{

			ITSDPLQCTask::ITSDPLQCTask() : TaskInterface(), mHistogram(nullptr) { 
				mHistogram = nullptr;

				gStyle->SetOptFit (0);
				gStyle->SetOptStat (0);
				o2::Base::GeometryManager::loadGeometry ();
				gStyle->SetPadRightMargin(0.10);
				for(int i = 0; i < NLayer; i++){

					NChipLay[i] = ChipBoundary[i + 1] - ChipBoundary[i];

					ChipStave[i] = new TH2D(Form("ChipStaveLayer%d",i),Form("ChipStaveLayer%d",i),NChipLay[i],ChipBoundary[i],ChipBoundary[i+1],NEventMax[i],0,NEventMax[i]);
					ChipStave[i]->GetXaxis ()->SetTitle ("Chip ID");
					ChipStave[i]->GetYaxis ()->SetTitle ("Number of Hits");
					ChipStave[i]->SetTitle (Form("Occupancy for ITS Layer %d",i));

					ChipProj[i]	= new TH1D(Form("ChipProj%d",i),Form("ChipProj%d",i),NChipLay[i],ChipBoundary[i],ChipBoundary[i+1]); 
					ChipProj[i]->GetXaxis ()->SetTitle ("Chip ID");
					ChipProj[i]->GetYaxis ()->SetTitle ("Average Number of Hits");
					ChipProj[i]->SetTitle (Form("Average Occupancy for ITS Layer %d",i));

					LayEtaPhi[i] = new TH2D(Form("Lay1EtaPhiLay%d",i),Form("Lay1EtaPhiLay%d",i),NEta,EtaMin,EtaMax,NPhi,PhiMin,PhiMax);
					LayEtaPhi[i]->GetXaxis()->SetTitle("#eta");
					LayEtaPhi[i]->GetYaxis()->SetTitle("#phi");
					LayEtaPhi[i]->GetZaxis()->SetTitle("Number of Hits");
					LayEtaPhi[i]->GetZaxis()->SetTitleOffset(0.07);
					LayEtaPhi[i]->SetTitle(Form("Number of Hits for Layer %d #eta and #phi Distribution",i));


					NStaveChip[i] = NChipLay[i]/NStaves[i];

					LayChipStave[i] = new TH2D(Form("LayChipStave%d",i),Form("LayChipStave%d",i),NStaveChip[i],0,NStaveChip[i],NStaves[i],0,NStaves[i]);
					LayChipStave[i]->GetXaxis()->SetTitle("Chip Number");
					LayChipStave[i]->GetYaxis()->SetTitle("Stave Number");
					LayChipStave[i]->GetZaxis()->SetTitle("Number of Hits");
					LayChipStave[i]->GetZaxis()->SetTitleOffset(0.0);
					LayChipStave[i]->SetTitle(Form("Number of Hits for Layer %d Chip Number and Stave Number Distribution",i));



				}


				for(int i = 0; i < NError; i++){
					Error[i] = 12 - i;
					ErrorPlots[i] = new TH1D(Form("ErrorPlot%d",i),Form("ErrorPlot%d",i),NError,0,NError);
					ErrorPlots[i]->GetXaxis()->SetTitle("Error Type");
					ErrorPlots[i]->GetYaxis()->SetTitle("Counts");
					ErrorPlots[i]->SetTitle("Error Checked During Decoding");
					ErrorPlots[i]->SetMaximum(35);
				}

				for(int j = 0; j < 1; j++){
					for(int i = 0; i < NStaveChip[j]; i++){
						HIGMAP[i]	= new TH2D(Form("HIGMAP%d%d",i,j),Form("HIGMAP%d%d",i,j),100,0,NColHis,100,0,NRowHis);
						HIGMAP[i]->GetXaxis()->SetTitle("Column");
						HIGMAP[i]->GetYaxis()->SetTitle("Row");
						HIGMAP[i]->SetTitle(Form("Hits on Pixel of Stave 1 for Chip Number % d",i));
					}
				}

				HIGMAP[6]->SetMaximum(2);	
				HIGMAP[6]->SetMinimum(0);	
				cout << "Clear " << endl;
				/*
				   for(int i = 0 ; i < 24120; i++){	
				   gm->getChipId (i, lay, sta, ssta, mod, chip);
				   gm->fillMatrixCache(o2::utils::bit2Mask(o2::TransformType::L2G));
				   const Point3D<float> loc(0., 0.,0.); 
				   auto glo = gm->getMatrixL2G(i)(loc);
				   eta = glo.eta();
				   phi = glo.phi();
				   cout << "CHip ID = " << i << "   Layer = " << lay  << "  Stave = " << sta << "  eta = " << eta << "   phi = " << phi << endl;

				   }
				   */

			}

			ITSDPLQCTask::~ITSDPLQCTask() {
				if (mHistogram) {
					delete mHistogram;
				}
			}

			void ITSDPLQCTask::initialize(o2::framework::InitContext& ctx)
			{
				QcInfoLogger::GetInstance() << "initialize ITSDPLQCTask" << AliceO2::InfoLogger::InfoLogger::endm;

				bool mRawDataMode = 1;
				if (mRawDataMode)
				{
					//mReaderRaw = std::make_unique<o2::ITSMFT::RawPixelReader<o2::ITSMFT::ChipMappingITS>>();

					//mReader = mReaderRaw.get();
					int a = 1;
				}
				else
				{				// clusterizer of digits needs input from the FairRootManager (at the moment)
					mReaderMC = std::make_unique < o2::ITSMFT::DigitPixelReader > ();
					mReader = mReaderMC.get ();
				}

				LOG (INFO) << "It WORK, PASS 1";

				o2::ITS::GeometryTGeo * geom = o2::ITS::GeometryTGeo::Instance ();
				geom->fillMatrixCache (o2::utils::bit2Mask (o2::TransformType::L2G));	
				const Int_t numOfChips = geom->getNumberOfChips ();
				cout << "numOfChips = " << numOfChips << endl;
				setNChips (numOfChips);
				cout << "START LOOPING BRO" << endl;
				mReaderRaw.openInput (inpName);
				mReaderRaw.setPadding128(true);
				mReaderRaw.setVerbosity(0);

				/*
				   const auto* ruInfo = mReaderRaw.getRUDecodeData().ruInfo;
				   if (ruInfo) {
				   const auto& statRU =  mReaderRaw.getRUDecodingStatSW( ruInfo.idSW ); 
				   printf("ErrorCount: %d\n", (int)statRU.errorCounts[o2::ITSMFT::RUDecodingStat::ErrPageCounterDiscontinuity] );
				   }
				   */
				process (mReaderRaw);



				/*
				   int IndexNow = 0;
				   while (mReaderMC->readNextEntry()) {
				   cout << "Now Working on Event = " << IndexNow << endl;
				   process(* mReader);
				   IndexNow = IndexNow + 1;
				   }
				   */
				TCanvas * c2 = new TCanvas("c2","c2",600,600);
				c2->cd();
				TLegend* l = new TLegend(0.15,0.50,0.90,0.90);
				for(int i =0; i< NError;i++){
					cout << "i = " << i << "  Error Number = " << Error[i] << endl;
					ErrorPlots[i]->SetBinContent(i+1,Error[i]);
					ErrorPlots[i]->SetLineColor(i+1);
					ErrorPlots[i]->SetFillColor(i+1);
					if(i == 0) 	{
						ErrorPlots[i]->Draw();
					}
					if(i > 0) 	ErrorPlots[i]->Draw("SAME");
					l->AddEntry(ErrorPlots[i],ErrorType[i].Data());	
					//getObjectsManager()->startPublishing(ErrorPlots[i]);	
				}
				l->Draw("SAME");
				c2->SaveAs("ErrorChecker.png");


				TCanvas *c = new TCanvas ("c", "c", 600, 600);
				c->cd ();

				for(int j = 0; j < NLayer; j++){ 
					ChipStave[j]->SetMarkerSize(1.2);
					ChipStave[j]->SetMarkerStyle(24);
					ChipStave[j]->Draw();
					cout << "j = " << j << "   Number of ChipStave = " << ChipStave[j]->GetEntries() << endl;
					c->SaveAs (Form("Occupancy%d.png",j));
				}
				cout << "Plot Draw" << endl;

		
				for(int j = 0; j < NLayer; j++){ 
					for (int i = ChipBoundary[j]; i < ChipBoundary[j+1]; i++)
					{
						TH1D *Proj = new TH1D ("Proj", "CProj", NEventMax[j], 0, NEventMax[j]);

						int XBin = ChipStave[j]->GetXaxis ()->FindBin (i);
						ChipStave[j]->ProjectionY ("Proj", XBin, XBin);
						//			cout << "Mean = " << Proj->GetMean () << endl;
						//			cout << "RMS = " << Proj->GetRMS () << endl;
						ChipProj[j]->Fill (i, Proj->GetMean());
						ChipProj[j]->SetBinError (XBin, Proj->GetRMS () / Proj->Integral ());
					}
					ChipProj[j]->SetMarkerStyle (22);
					ChipProj[j]->SetMarkerSize (1.5);
					ChipProj[j]->Draw ("p");
					c->SaveAs(Form("OccupancyProj%d.png",j));
				}
				fout = new TFile("Hist.root","RECREATE");
				fout->cd();
				for(int j = 0; j < NLayer; j++){ 
					ChipStave[j]->Write();
					ChipProj[j]->Write();
				}
				fout->Close();

				for(int j = 0; j < NLayer; j++){ 
					LayEtaPhi[j]->Draw("COLZ");
					cout << "Eta Phi Total = " << 	LayEtaPhi[j]->Integral() << endl;
					c->SaveAs(Form("EtaPhiLay%d.png",j));
				}

				for(int j = 0; j < NLayer; j++){ 
					LayChipStave[j]->Draw("COLZ");
					cout << "LayChipStave Total = " << 	LayChipStave[j]->Integral() << endl;
					c->SaveAs(Form("LayChipStave%d.png",j));
				}

				TCanvas *c1 = new TCanvas ("c1", "c1", 600, 600);

				for(int j = 0; j < 1; j++){
					c1->Divide(3,3);
					for(int i = 0; i < NStaveChip[j]; i++){
						c1->cd(i+1);
						HIGMAP[i]->Draw("COLZ");
						getObjectsManager()->startPublishing(HIGMAP[i]);
					}
					c1->SaveAs(Form("HIGMAPStave%d.png",j+1));
				}

	


				for(int i = 2; i < 3; i++){

					getObjectsManager()->startPublishing(ChipProj[i]);
					getObjectsManager()->startPublishing(ChipStave[i]);	
					getObjectsManager()->startPublishing(LayEtaPhi[i]);
					getObjectsManager()->startPublishing(LayChipStave[i]);


				}

				mHistogram = new TH1F("example", "example", 20, 0, 30000);
				//	getObjectsManager()->addCheck(mHistogram, "checkFromITSDPLQCTask", "o2::quality_control_modules::itsdplqctask::ITSDPLQCTaskCheck",	"QcITSDPLQCTask");
				//getObjectsManager()->addCheck(ChipStave, "checkFromITSDPLQCTask", "o2::quality_control_modules::itsdplqctask::ITSDPLQCTaskCheck",	"QcITSDPLQCTask");

			}

			void ITSDPLQCTask::startOfActivity(Activity& activity)
			{
				QcInfoLogger::GetInstance() << "startOfActivity" << AliceO2::InfoLogger::InfoLogger::endm;
				mHistogram->Reset();
			}

			void ITSDPLQCTask::startOfCycle()
			{
				QcInfoLogger::GetInstance() << "startOfCycle" << AliceO2::InfoLogger::InfoLogger::endm;
			}

			void ITSDPLQCTask::monitorData(o2::framework::ProcessingContext& ctx)
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
				   QcInfoLogger::GetInstance() << "BEEN HERE BRO" << AliceO2::InfoLogger::InfoLogger::endm;

				   auto digits = ctx.inputs().get<const std::vector<o2::ITSMFT::Digit>>("digits");
				   QcInfoLogger::GetInstance() << "PASS DIGIT OK" << AliceO2::InfoLogger::InfoLogger::endm;
				   LOG(INFO) << "ITSClusterer pulled " << digits.size() << " digits, ";
				   o2::ITSMFT::DigitPixelReader reader;
				   */
				/*
				   for (auto&& input : ctx.inputs()) {
				// get message header
				const auto* header = header::get<header::DataHeader*>(input.header);
				// get payload of a specific input, which is a char array.
				//    const char* payload = input.payload;

				// for the sake of an example, let's fill the histogram with payload sizes
				mHistogram->Fill(header->payloadSize);
				QcInfoLogger::GetInstance()  << "PayLoadSize = " << header->payloadSize << AliceO2::InfoLogger::InfoLogger::endm;

				}
				*/

				// 2. Using get("<binding>")

				// get the payload of a specific input, which is a char array. "random" is the binding specified in the config file.
				//   auto payload = ctx.inputs().get("random").payload;

				// get payload of a specific input, which is a structure array:
				//  const auto* header = header::get<header::DataHeader*>(ctx.inputs().get("random").header);
				//  struct s {int a; double b;};
				//  auto array = ctx.inputs().get<s*>("random");
				//  for (int j = 0; j < header->payloadSize / sizeof(s); ++j) LayChipStave{
				//    int i = array.get()[j].a;
				//  }

				// get payload of a specific input, which is a root object
				//   auto h = ctx.inputs().get<TH1F*>("histos");
				//   Double_t stats[4];
				//   h->GetStats(stats);
				//   auto s = ctx.inputs().get<TObjString*>("string");
				//   LOG(INFO) << "String is " << s->GetString().Data();
			}


			void ITSDPLQCTask::process (PixelReader & reader){


				cout << "START PROCESSING" << endl;




				for (int i = 0; i < 24120; i++)
				{
					Occupancy[i] = 0;
				}
				cout << "START MCHIPDATA" << endl;
				while ((mChipData = reader.getNextChipData (mChips)))
				{
					//      cout << "ChipID Before = " << ChipID << endl; 
					ChipID = mChipData->getChipID ();
					mReaderRaw.getMapping().getChipInfoSW( ChipID, chipInfo );
					const auto& statRU =  mReaderRaw.getRUDecodingStatSW( chipInfo.ru );
					//printf("ErrorCount: %d\n", (int)statRU.errorCounts[o2::ITSMFT::RUDecodingStat::ErrPageCounterDiscontinuity] );

					Error[0] = Error[0]  + (int)statRU.errorCounts[o2::ITSMFT::RUDecodingStat::ErrGarbageAfterPayload];
					Error[1] = Error[1]  + (int)statRU.errorCounts[o2::ITSMFT::RUDecodingStat::ErrPageCounterDiscontinuity];
					Error[2] = Error[2]  + (int)statRU.errorCounts[o2::ITSMFT::RUDecodingStat::ErrRDHvsGBTHPageCnt];
					Error[3] = Error[3]  + (int)statRU.errorCounts[o2::ITSMFT::RUDecodingStat::ErrMissingGBTHeader];
					Error[4] = Error[4]  + (int)statRU.errorCounts[o2::ITSMFT::RUDecodingStat::ErrMissingGBTTrailer];
					Error[5] = Error[5]  + (int)statRU.errorCounts[o2::ITSMFT::RUDecodingStat::ErrNonZeroPageAfterStop];
					Error[6] = Error[6]  + (int)statRU.errorCounts[o2::ITSMFT::RUDecodingStat::ErrUnstoppedLanes];
					Error[7] = Error[7]  + (int)statRU.errorCounts[o2::ITSMFT::RUDecodingStat::ErrDataForStoppedLane];
					Error[8] = Error[8]  + (int)statRU.errorCounts[o2::ITSMFT::RUDecodingStat::ErrNoDataForActiveLane];
					Error[9] = Error[9]  + (int)statRU.errorCounts[o2::ITSMFT::RUDecodingStat::ErrIBChipLaneMismatch];
					Error[10] = Error[10]  + (int)statRU.errorCounts[o2::ITSMFT::RUDecodingStat::ErrCableDataHeadWrong];



					gm->getChipId (ChipID, lay, sta, ssta, mod, chip);
					gm->fillMatrixCache(o2::utils::bit2Mask(o2::TransformType::L2G));
					const Point3D<float> loc(0., 0.,0.); 
					auto glo = gm->getMatrixL2G(ChipID)(loc);

					if (lay < 7)
					{
						//cout << "lay = " <<  lay << endl;
						//cout << "ChipID = " << ChipID << endl;

						int ChipNumber = (ChipID - ChipBoundary[lay])- sta*	NStaveChip[lay];
						ActPix = mChipData->getData().size();

						//cout << "ChipNumber = " << ChipNumber << endl;
						eta = glo.eta();
						phi = glo.phi();
						Occupancy[ChipID] = Occupancy[ChipID] + ActPix;
						ChipStave[lay]->Fill(ChipID, ActPix);

						LayEtaPhi[lay]->Fill(eta,phi,ActPix);
						LayChipStave[lay]->Fill(ChipNumber,sta,ActPix);
						if(sta == 0 && ChipID < NLay1){
							//cout << "ChipID in Stave 0 = " << ChipID << endl; 
							for(int ip = 0; ip < ActPix; ip++){
								const auto pix = mChipData->getData()[ip];
								UShort_t row = pix.getRow();
								UShort_t col = pix.getCol();
								if(row > 0 && col > 0) HIGMAP[ChipID]->Fill(col,row);
							}
						}

					}
				}
				cout << "Start Filling" << endl;

				/*
				for(int j = 0; j < NLayer; j++){ 
					for (int i = ChipBoundary[j]; i < ChipBoundary[j+1]; i++)
					{
						//int XBin = ChipStave[j]->GetXaxis()->FindBin(i);
						//AveOcc = Occupancy[i]/NPixels;
						cout << "i = " << i << "   Occupancy = " << Occupancy[i] << endl;
											ChipStave[lay]->Fill(i, Occupancy[i]);

					}
				}
				*/
			}
		

			void ITSDPLQCTask::endOfCycle()
			{
				QcInfoLogger::GetInstance() << "endOfCycle" << AliceO2::InfoLogger::InfoLogger::endm;
			}

			void ITSDPLQCTask::endOfActivity(Activity& activity)
			{
				QcInfoLogger::GetInstance() << "endOfActivity" << AliceO2::InfoLogger::InfoLogger::endm;
			}

			void ITSDPLQCTask::reset()
			{
				// clean all the monitor objects here

				QcInfoLogger::GetInstance() << "Resetting the histogram" << AliceO2::InfoLogger::InfoLogger::endm;
				mHistogram->Reset();
			}

		} // namespace itsdplqctask
	} // namespace quality_control_modules
} // namespace o2

