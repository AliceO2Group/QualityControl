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

#include "ITSMFTReconstruction/DigitPixelReader.h"
#include "DetectorsBase/GeometryManager.h"
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

				gStyle->SetPadRightMargin(0.15);
				gStyle->SetPadLeftMargin(0.15);
				o2::Base::GeometryManager::loadGeometry ();


				gStyle->SetOptFit(0);
				gStyle->SetOptStat(0);

				ChipStave->GetXaxis()->SetTitle("Chip ID");
				ChipStave->GetYaxis()->SetTitle("Number of Hits");
				ChipStave->SetTitle("Number of Hits vs Chip ID for Stave 1 at Layer 1");


				for(int i = 0; i < NLayer; i++){

					NChipLay[i] = ChipBoundary[i + 1] - ChipBoundary[i];

					OccupancyPlot[i]	= new TH1D(Form("Occupancy%d",i),Form("Occupancy%d",i),NEventMax[i],0,NEventMax[i]); 
					OccupancyPlot[i]->GetXaxis()->SetTitle ("Occupancy");
					OccupancyPlot[i]->GetYaxis()->SetTitle ("Counts");
					OccupancyPlot[i]->GetYaxis()->SetTitleOffset(2.2);	
					OccupancyPlot[i]->SetTitle(Form("Occupancy Distribution for ITS Layer %d",i));

					LayEtaPhi[i] = new TH2D(Form("Lay1EtaPhiLay%d",i),Form("Lay1EtaPhiLay%d",i),NEta,EtaMin,EtaMax,NPhi,PhiMin,PhiMax);
					LayEtaPhi[i]->GetXaxis()->SetTitle("#eta");
					LayEtaPhi[i]->GetYaxis()->SetTitle("#phi");
					LayEtaPhi[i]->GetZaxis()->SetTitle("Number of Hits");
					LayEtaPhi[i]->GetZaxis()->SetTitleOffset(1.4);
					LayEtaPhi[i]->GetYaxis()->SetTitleOffset(1.10);	
					LayEtaPhi[i]->SetTitle(Form("Number of Hits for Layer %d #eta and #phi Distribution",i));


					NStaveChip[i] = NChipLay[i]/NStaves[i];
					NColStave[i] = NStaveChip[i] * NColHis;

					LayChipStave[i] = new TH2D(Form("LayChipStave%d",i),Form("LayChipStave%d",i),NStaveChip[i],0,NStaveChip[i],NStaves[i],0,NStaves[i]);
					LayChipStave[i]->GetXaxis()->SetTitle("Chip Number");
					LayChipStave[i]->GetYaxis()->SetTitle("Stave Number");
					LayChipStave[i]->GetZaxis()->SetTitle("Number of Hits");
					LayChipStave[i]->GetZaxis()->SetTitleOffset(1.4);
					LayChipStave[i]->GetYaxis()->SetTitleOffset(1.10);	
					LayChipStave[i]->SetTitle(Form("Number of Hits for Layer %d Chip Number and Stave Number Distribution",i));


				}


				for(int i = 0; i < NError; i++){
					Error[i] = 0;
				}

				for(int j = 0; j < 1; j++){
					for(int i = 0; i< NStaves[j]; i++){
						Lay1HIG[i] = new TH2D(Form("HICMAPLay%dStave%d",j,i),Form("HICMAPLay%dStave%d",j,i),100,0,NColHis*NStaveChip[i] ,100,0,NRowHis);
						Lay1HIG[i]->GetXaxis()->SetTitle("Column");
						Lay1HIG[i]->GetYaxis()->SetTitle("Row");
						Lay1HIG[i]->GetYaxis()->SetTitleOffset(1.10);
						Lay1HIG[i]->GetZaxis()->SetTitleOffset(1.50);
						Lay1HIG[i]->SetTitle(Form("Hits Map on Layer %d Stave %d",j,i));
					}
				}

				ErrorPlots->GetXaxis()->SetTitle("Error ID");
				ErrorPlots->GetYaxis()->SetTitle("Counts");
				ErrorPlots->SetTitle("Error Checked During Decoding");
				ErrorPlots->SetMinimum(0);

				for(int j = 0; j < 1; j++){
					for(int i = 0; i < NStaveChip[j]; i++){
						HIGMAP[i]	= new TH2D(Form("HIGMAP%dLay%d",i,j),Form("HIGMAP%dLay%d",i,j),100,0,NColHis,100,0,NRowHis);
						HIGMAP[i]->GetXaxis()->SetTitle("Column");
						HIGMAP[i]->GetYaxis()->SetTitle("Row");
						HIGMAP[i]->GetYaxis()->SetTitleOffset(1.10);
						HIGMAP[i]->GetZaxis()->SetTitleOffset(1.50);
						HIGMAP[i]->SetTitle(Form("Hits on Pixel of Stave 1 for Chip Number % d on Layer %d",i,j));
					}
				}

				for(int j = 6; j < 7; j++){
					for(int i = 0; i < 18; i++){
						HIGMAP6[i]	= new TH2D(Form("HIGMAP%dLay%d",i,j),Form("HIGMAP%dLay%d",i,j),100,0,NColHis*11,100,0,NRowHis);
						HIGMAP6[i]->GetXaxis()->SetTitle("Column");
						HIGMAP6[i]->GetYaxis()->SetTitle("Row");
						HIGMAP6[i]->GetYaxis()->SetTitleOffset(1.10);
						HIGMAP6[i]->GetZaxis()->SetTitleOffset(1.50);
						HIGMAP6[i]->SetTitle(Form("Hits on Pixel of Stave 1 for Chip Sector Number % d on Layer %d",i,j));
					}
				}

				HIGMAP[6]->SetMaximum(2);	
				HIGMAP[6]->SetMinimum(0);	
				cout << "Clear " << endl;


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
				cout << "START LOOPING BR	getObjectsManager()->startPublishingO" << endl;
				mReaderRaw.openInput (inpName);
				mReaderRaw.setPadding128(true);
				mReaderRaw.setVerbosity(0);


				process (mReaderRaw);



				/*
				   int IndexNow = 0;
				   while (mReaderMC->readNextEntry()) {
				   cout << "Now Working on Event = " << IndexNow << endl;
				   process(* mReader);
				   IndexNow = IndexNow + 1;
				   }
				   */

				TCanvas *c5 = new TCanvas("c5","c5",600,600);
				c5->cd();
				c5->SetLogz();
				ChipStave->SetMinimum(1);
				ChipStave->Draw("COLZ");
				c5->SaveAs("Stave1ChipStaveCheck.png");

				getObjectsManager()->startPublishing(ChipStave);

				TCanvas * c2 = new TCanvas("c2","c2",600,600);
				c2->cd();
				//TLegend* l = new TLegend(0.15,0.50,0.90,0.90);
				for(int i =0; i< NError;i++){
					cout << "i = " << i << "  Error Number = " << Error[i] << endl;
					ErrorPlots->SetBinContent(i+1,Error[i]);
					pt[i] = new TPaveText(0.20,0.80 -i*0.05,0.85,0.85-i*0.05,"NDC");
					pt[i]->SetTextSize(0.04);
					pt[i]->SetFillColor(0);
					pt[i]->SetTextAlign(12);
					pt[i]->AddText(ErrorType[i].Data());
					ErrorPlots->GetListOfFunctions()->Add(pt[i]);
				}

				ErrorMax = ErrorPlots->GetMaximum();
				ErrorPlots->SetMaximum(ErrorMax * 4.1+1000);
				ErrorPlots->Draw();
				gStyle->SetOptStat(0);
				c2->SaveAs("ErrorChecker.png");
				getObjectsManager()->startPublishing(ErrorPlots);

				
				TCanvas *c = new TCanvas ("c", "c", 600, 600);
				c->cd ();

				/*
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
				*/
				c->SetLogy();	
				for(int j = 0; j < NLayer; j++){ 
					OccupancyPlot[j]->SetMarkerStyle (22);
					OccupancyPlot[j]->SetMarkerSize (1.5);
					OccupancyPlot[j]->Draw ("ep");

					c->SaveAs(Form("OccupancyLay%d.png",j));

				}

				fout = new TFile("Hist.root","RECREATE");
				fout->cd();
				for(int j = 0; j < NLayer; j++){ 
					//ChipStave[j]->Write();
					OccupancyPlot[j]->Write();
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
						HIGMAP[i]->GetZaxis()->SetTitle("Number of Hits");
						HIGMAP[i]->Draw("COLZ");
						getObjectsManager()->startPublishing(HIGMAP[i]);
					}
					c1->SaveAs(Form("HIGMAPStave%d.png",j+1));
				}

				TCanvas *c6 = new TCanvas ("c6", "c6", 600, 600);

				for(int j = 0; j < 1; j++){
					c6->Divide(3,4);
					for(int i = 0; i < NStaves[j]; i++){
						c6->cd(i+1);
						Lay1HIG[i]->GetZaxis()->SetTitle("Number of Hits");
						Lay1HIG[i]->Draw("COLZ");
						getObjectsManager()->startPublishing(Lay1HIG[i]);
					}
					c6->SaveAs(Form("HIGMAPLay%d.png",j+1));
				}




				TCanvas *c3 = new TCanvas ("c3", "c3", 3600, 7200);


				for(int j = 6; j < 7; j++){
					c3->Divide(3,6);
					for(int i = 0; i < 18; i++){
						c3->cd(i+1);
						HIGMAP6[i]->GetZaxis()->SetTitle("Number of Hits");
						HIGMAP6[i]->Draw("COLZ");
						getObjectsManager()->startPublishing(HIGMAP6[i]);
					}
					c3->SaveAs(Form("HIGMAPStave%d.png",j+1));
				}



				for(int i = 0; i < NLayer; i++){


					getObjectsManager()->startPublishing(LayEtaPhi[i]);
					getObjectsManager()->startPublishing(LayChipStave[i]);
					getObjectsManager()->startPublishing(OccupancyPlot[i]);


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



				cout << "START MCHIPDATA" << endl;
				while ((mChipData = reader.getNextChipData (mChips)))
				{
					//      cout << "ChipID Before = " << ChipID << endl; 
					ChipID = mChipData->getChipID ();
					mReaderRaw.getMapping().getChipInfoSW( ChipID, chipInfo );
					const auto& statRU =  mReaderRaw.getRUDecodingStatSW( chipInfo.ru );
					//printf("ErrorCount: %d\n", (int)statRU.errorCounts[o2::ITSMFT::RUDecodingStat::ErrPageCounterDiscontinuity] );

					//Error[0] = Error[0]  + (int)statRU->errorCounts[o2::ITSMFT::RUDecodingStat::ErrGarbageAfterPayload];
					Error[0] = Error[0]  + (int)statRU->errorCounts[o2::ITSMFT::RUDecodingStat::ErrPageCounterDiscontinuity];
					Error[1] = Error[1]  + (int)statRU->errorCounts[o2::ITSMFT::RUDecodingStat::ErrRDHvsGBTHPageCnt];
					Error[2] = Error[2]  + (int)statRU->errorCounts[o2::ITSMFT::RUDecodingStat::ErrMissingGBTHeader];
					Error[3] = Error[3]  + (int)statRU->errorCounts[o2::ITSMFT::RUDecodingStat::ErrMissingGBTTrailer];
					Error[4] = Error[4]  + (int)statRU->errorCounts[o2::ITSMFT::RUDecodingStat::ErrNonZeroPageAfterStop];
					Error[5] = Error[5]  + (int)statRU->errorCounts[o2::ITSMFT::RUDecodingStat::ErrUnstoppedLanes];  
					Error[6] = Error[6]  + (int)statRU->errorCounts[o2::ITSMFT::RUDecodingStat::ErrDataForStoppedLane];
					Error[7] = Error[7]  + (int)statRU->errorCounts[o2::ITSMFT::RUDecodingStat::ErrNoDataForActiveLane];
					Error[8] = Error[8]  + (int)statRU->errorCounts[o2::ITSMFT::RUDecodingStat::ErrIBChipLaneMismatch];
					Error[9] = Error[9]  + (int)statRU->errorCounts[o2::ITSMFT::RUDecodingStat::ErrCableDataHeadWrong];

					//cout << "Error 5 = " << Error[5]  << endl;

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
						//			Occupancy[ChipID] = Occupancy[ChipID] + ActPix;
						OccupancyPlot[lay]->Fill(ActPix);
						ChipStave->Fill(ChipID, ActPix);
						LayEtaPhi[lay]->Fill(eta,phi,ActPix);
						LayChipStave[lay]->Fill(ChipNumber,sta,ActPix);
						if(sta == 0  && ChipID < NLay1){
							//cout << "ChipID in Stave 0 = " << ChipID << endl; 
							for(int ip = 0; ip < ActPix; ip++){
								const auto pix = mChipData->getData()[ip];
								row = pix.getRow();
								col = pix.getCol();
								if(row > 0 && col > 0) HIGMAP[ChipID]->Fill(col,row);
						
							}	
						}

						if(lay == 0){
							//cout << "ChipID in Stave 0 = " << ChipID << endl; 
							for(int ip = 0; ip < ActPix; ip++){
								const auto pix = mChipData->getData()[ip];
								row = pix.getRow();
								col = pix.getCol() + NColHis * ChipNumber;
								if(row > 0 && col > 0) Lay1HIG[sta]->Fill(col,row);
						
							}	
						}

						if(sta == 0 && lay == 6){
							//cout << "ChipID in Stave 0 = " << ChipID << endl; 
							for(int ip = 0; ip < ActPix; ip++){
								const auto pix = mChipData->getData()[ip];
								ChipIndex6 = ChipNumber/11; 
								int ChipLocal6 = ChipNumber - ChipIndex6 * 11;
								if(ChipLocal6 < 0 ) ChipLocal6 = ChipNumber - (ChipIndex6 -1) * 11;

								row = pix.getRow();
								col = pix.getCol() + ChipLocal6 * NColHis;

								//cout << "ChipNumber = " <<  ChipNumber << "   ChipIndex6 = " <<  ChipIndex6 << "   ChipLocal6 = " << ChipLocal6 << endl;
								if(row > 0 && col > 0) HIGMAP6[ChipIndex6]->Fill(col,row);
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

