///
/// \file   SimpleDS.cxx
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

#include <TCanvas.h>
#include <TH1.h>
#include <TH2.h>

#include "QualityControl/FileFinish.h"
#include "QualityControl/QcInfoLogger.h"
#include "SimpleDS/SimpleDS.h"
#include "DetectorsBase/GeometryManager.h"
#include "ITSBase/GeometryTGeo.h"
#include "ITSMFTReconstruction/DigitPixelReader.h"
#include <algorithm>


using o2::itsmft::Digit;



using namespace std;
using namespace o2::itsmft;
using namespace o2::ITS;



namespace o2
{
	namespace quality_control_modules
	{
		namespace simpleds
		{

			SimpleDS::SimpleDS() : TaskInterface() { 

				o2::base::GeometryManager::loadGeometry();
				gStyle->SetPadRightMargin(0.15);
				gStyle->SetPadLeftMargin(0.15);



				gStyle->SetOptFit(0);
				gStyle->SetOptStat(0);

				ChipStave->GetXaxis()->SetTitle("Chip ID");
				ChipStave->GetYaxis()->SetTitle("Number of Hits");
				ChipStave->SetTitle("Number of Hits vs Chip ID for Stave 1 at Layer 1");


				for(int i = 0; i < NLayer; i++){

					NChipLay[i] = ChipBoundary[i + 1] - ChipBoundary[i];

					OccupancyPlot[i]	= new TH1D(Form("Layer%dOccupancy",i),Form("Layer%dOccupancy",i),NEventMax[i],0,NEventMax[i]); 
					OccupancyPlot[i]->GetXaxis()->SetTitle ("Occupancy");
					OccupancyPlot[i]->GetYaxis()->SetTitle ("Counts");
					OccupancyPlot[i]->GetYaxis()->SetTitleOffset(2.2);	
					OccupancyPlot[i]->SetTitle(Form("Occupancy Distribution for ITS Layer %d",i));
					//		OccupancyPlot[i]->SetStats(false);

					LayEtaPhi[i] = new TH2S(Form("Layer%dEtaPhi",i),Form("Layer%dEtaPhi",i),NEta,EtaMin,EtaMax,NPhi,PhiMin,PhiMax);
					LayEtaPhi[i]->GetXaxis()->SetTitle("#eta");
					LayEtaPhi[i]->GetYaxis()->SetTitle("#phi");
					LayEtaPhi[i]->GetZaxis()->SetTitle("Number of Hits");
					LayEtaPhi[i]->GetZaxis()->SetTitleOffset(1.4);
					LayEtaPhi[i]->GetYaxis()->SetTitleOffset(1.10);	
					LayEtaPhi[i]->SetTitle(Form("Number of Hits for Layer %d #eta and #phi Distribution",i));
					//		LayEtaPhi[i]->SetStats(false);


					NStaveChip[i] = NChipLay[i]/NStaves[i];
					NColStave[i] = NStaveChip[i] * NColHis;

					LayChipStave[i] = new TH2S(Form("Layer%dChipStave",i),Form("Layer%dChipStave",i),NStaveChip[i],0,NStaveChip[i],NStaves[i],0,NStaves[i]);
					LayChipStave[i]->GetXaxis()->SetTitle("Chip Number");
					LayChipStave[i]->GetYaxis()->SetTitle("Stave Number");
					LayChipStave[i]->GetZaxis()->SetTitle("Number of Hits");
					LayChipStave[i]->GetZaxis()->SetTitleOffset(1.4);
					LayChipStave[i]->GetYaxis()->SetTitleOffset(1.10);	
					LayChipStave[i]->SetTitle(Form("Number of Hits for Layer %d Chip Number and Stave Number Distribution",i));
					//					LayChipStave[i]->SetStats(false);


				}


				for(int i = 0; i < NError; i++){
					Errors[i] = 0;
					ErrorPre[i] = 0;
					ErrorPerFile[i] = 0;
				}


				for(int j = 0; j < 1; j++){
					for(int i = 0; i< NStaves[j]; i++){
						Lay1HIT[i] = new TH2S(Form("Layer%dStave%dHITMAP",j,i),Form("Layer%dStave%dHITMAP",j,i),NColHis*NStaveChip[j]/SizeReduce,0,NColHis*NStaveChip[j]/SizeReduce,NRowHis,0,NRowHis);
						//		Lay1HIT[i] = new TH2D(Form("HICMAPLay%dStave%d",j,i),Form("HICMAPLay%dStave%d",j,i),100,0,NColHis*NStaveChip[j],100,0,NRowHis);
						Lay1HIT[i]->GetXaxis()->SetTitle("Column");
						Lay1HIT[i]->GetYaxis()->SetTitle("Row");
						Lay1HIT[i]->GetYaxis()->SetTitleOffset(1.10);
						Lay1HIT[i]->GetZaxis()->SetTitleOffset(1.50);
						Lay1HIT[i]->SetTitle(Form("Hits Map on Layer %d Stave %d",j,i));
						//			Lay1HIT[i]->SetStats(false);

					}
				}

				ErrorPlots->GetXaxis()->SetTitle("Error ID");
				ErrorPlots->GetYaxis()->SetTitle("Counts");
				ErrorPlots->SetTitle("Error Checked During Decoding");
				ErrorPlots->SetMinimum(0);
				ErrorPlots->SetStats(false);
				ErrorPlots->SetFillColor(kRed);


				cout << "DONE 1" << endl;



				ErrorFile->GetXaxis()->SetTitle("File ID (data-link)");
				ErrorFile->GetYaxis()->SetTitle("Error ID");
				ErrorFile->GetZaxis()->SetTitle("Counts");
				ErrorFile->SetTitle("Error During Decoding vs File Name Statistics");
				ErrorFile->SetMinimum(0);
				ErrorFile->SetStats(false);

				for(int j = 0; j < 1; j++){
					for(int i = 0; i < NStaveChip[j]; i++){
						HITMAP[i]	= new TH2S(Form("Layer%dChip%dHITMAP",j,i),Form("Layer%dChip%dHITMAP",j,i),NColHis,0,NColHis,NRowHis,0,NRowHis);
						//		HITMAP[i]	= new TH2D(Form("HITMAP%dLay%d",i,j),Form("HIGMAP%dLay%d",i,j),100,0,NColHis,100,0,NRowHis);
						HITMAP[i]->GetXaxis()->SetTitle("Column");
						HITMAP[i]->GetYaxis()->SetTitle("Row");
						HITMAP[i]->GetYaxis()->SetTitleOffset(1.10);
						HITMAP[i]->GetZaxis()->SetTitleOffset(1.50);
						HITMAP[i]->SetTitle(Form("Hits on Pixel of Stave 1 for Chip Number % d on Layer %d",i,j));
						//	HITMAP[i]->SetStats(false);

					}
				}


				cout << "DONE 2" << endl;

				for(int j = 6; j < 7; j++){
					for(int i = 0; i < 18; i++){
						HITMAP6[i]	= new TH2S(Form("Layer%dStave%dHITMAP",j,i),Form("Layer%dStave%dHITMAP",j,i),NColHis*11,0,NColHis*11,NRowHis,0,NRowHis);
						//		HITMAP6[i]	= new TH2D(Form("HITMAP%dLay%d",i,j),Form("HIGMAP%dLay%d",i,j),100,0,NColHis*11,100,0,NRowHis);
						HITMAP6[i]->GetXaxis()->SetTitle("Column");
						HITMAP6[i]->GetYaxis()->SetTitle("Row");
						HITMAP6[i]->GetYaxis()->SetTitleOffset(1.10);
						HITMAP6[i]->GetZaxis()->SetTitleOffset(1.50);
						HITMAP6[i]->SetTitle(Form("Hits on Pixel of Stave 1 for Chip Sector Number % d on Layer %d",i,j));
						//		HITMAP6[i]->SetStats(false);
					}
				}
				cout << "DONE 3" << endl;

				HITMAP[6]->SetMaximum(2);	
				HITMAP[6]->SetMinimum(0);	
				cout << "Clear " << endl;
				FileNameInfo->GetXaxis()->SetTitle("InputFile");
				FileNameInfo->GetYaxis()->SetTitle("Total Files Proccessed");
				FileNameInfo->GetXaxis()->SetTitleOffset(1.10);
				//	FileNameInfo->SetStats(false);

			}

			SimpleDS::~SimpleDS() {

			}

			void SimpleDS::initialize(o2::framework::InitContext& ctx)
			{
				QcInfoLogger::GetInstance() << "initialize SimpleDS" << AliceO2::InfoLogger::InfoLogger::endm;

				o2::ITS::GeometryTGeo * geom = o2::ITS::GeometryTGeo::Instance ();
				geom->fillMatrixCache (o2::utils::bit2Mask (o2::TransformType::L2G));	
				numOfChips = geom->getNumberOfChips ();
				cout << "numOfChips = " << numOfChips << endl;
				setNChips (numOfChips);
				cout << "START LOOPING BR getObjectsManager()->startPublishingO" << endl;



				cout << "START Inititing Publication " << endl;


				getObjectsManager()->startPublishing(FileNameInfo);


				ChipStave->SetMinimum(1);
				getObjectsManager()->startPublishing(ChipStave);

				for(int i =0; i< NError;i++){
					pt[i] = new TPaveText(0.20,0.80 -i*0.05,0.85,0.85-i*0.05,"NDC");
					pt[i]->SetTextSize(0.04);
					pt[i]->SetFillColor(0);
					pt[i]->SetTextAlign(12);
					pt[i]->AddText(ErrorType[i].Data());
					ErrorPlots->GetListOfFunctions()->Add(pt[i]);
				}



				//TLegend* l = new TLegend(0.15,0.50,0.90,0.90);
				ErrorMax = ErrorPlots->GetMaximum();

				//cout << "ErrorMax = " << ErrorMax << endl;

				//ErrorPlots->SetMaximum(ErrorMax * 4.1+1000);
				//	ErrorPlots->SetName(Form("%s-%s",ErrorPlots->GetName(),HisRunID.Data()));
				//	cout << "ErrorPlot Name = " << ErrorPlots->GetName() << endl;
				getObjectsManager()->startPublishing(ErrorPlots);
				getObjectsManager()->addMetadata(ErrorPlots->GetName(), "custom", "34");
				getObjectsManager()->startPublishing(ErrorFile);


				ptFileName = new TPaveText(0.20,0.40,0.85,0.50,"NDC");
				ptFileName->SetTextSize(0.04);
				ptFileName->SetFillColor(0);
				ptFileName->SetTextAlign(12);
				ptFileName->AddText("Current File Proccessing: ");

				ptNFile = new TPaveText(0.20,0.30,0.85,0.40,"NDC");
				ptNFile->SetTextSize(0.04);
				ptNFile->SetFillColor(0);
				ptNFile->SetTextAlign(12);
				ptNFile->AddText("File Processed: ");


				ptNEvent = new TPaveText(0.20,0.20,0.85,0.30,"NDC");
				ptNEvent->SetTextSize(0.04);
				ptNEvent->SetFillColor(0);
				ptNEvent->SetTextAlign(12);
				ptNEvent->AddText("Event Processed: ");


				bulbRed = new TPaveText(0.60,0.75,0.90,0.85,"NDC");
				bulbRed->SetTextSize(0.04);
				bulbRed->SetFillColor(0);
				bulbRed->SetTextAlign(12);
				bulbRed->SetTextColor(kRed);
				bulbRed->AddText("Red = QC Waiting");


				bulbYellow = new TPaveText(0.60,0.65,0.90,0.75,"NDC");
				bulbYellow->SetTextSize(0.04);
				bulbYellow->SetFillColor(0);
				bulbYellow->SetTextAlign(12);
				bulbYellow->SetTextColor(kYellow);
				bulbYellow->AddText("Yellow = QC Pausing");



				bulbGreen = new TPaveText(0.60,0.55,0.90,0.65,"NDC");
				bulbGreen->SetTextSize(0.04);
				bulbGreen->SetFillColor(0);
				bulbGreen->SetTextAlign(12);
				bulbGreen->SetTextColor(kGreen);
				bulbGreen->AddText("GREEN = QC Processing");





				InfoCanvas->SetTitle("QC Process Information Canvas");
				InfoCanvas->GetListOfFunctions()->Add(ptFileName);
				InfoCanvas->GetListOfFunctions()->Add(ptNFile);
				InfoCanvas->GetListOfFunctions()->Add(ptNEvent);
				InfoCanvas->GetListOfFunctions()->Add(bulb);
				InfoCanvas->GetListOfFunctions()->Add(bulbRed);	
				InfoCanvas->GetListOfFunctions()->Add(bulbYellow);
				InfoCanvas->GetListOfFunctions()->Add(bulbGreen);
				//		InfoCanvas->SetStats(false);


				getObjectsManager()->startPublishing(InfoCanvas);



				for(int j = 0; j < 1; j++){
					for(int i = 0; i < NStaveChip[j]; i++){
						HITMAP[i]->GetZaxis()->SetTitle("Number of Hits");
						HITMAP[i]->GetXaxis()->SetNdivisions(-32);
						HITMAP[i]->Draw("COLZ");
						ConfirmXAxis(HITMAP[i]);
						ReverseYAxis(HITMAP[i]);
						getObjectsManager()->startPublishing(HITMAP[i]);

					}
				}


				ReverseYAxis(HITMAP[0]);


				for(int j = 0; j < 1; j++){
					for(int i = 0; i < NStaves[j]; i++){

						Lay1HIT[i]->GetZaxis()->SetTitle("Number of Hits");
						Lay1HIT[i]->GetXaxis()->SetNdivisions(-32);
						Lay1HIT[i]->Draw("COLZ");
						ConfirmXAxis(Lay1HIT[i]);
						ReverseYAxis(Lay1HIT[i]);
						getObjectsManager()->startPublishing(Lay1HIT[i]);
					}
				}





				for(int j = 6; j < 7; j++){
					for(int i = 0; i < 18; i++){
						HITMAP6[i]->GetZaxis()->SetTitle("Number of Hits");
						HITMAP6[i]->GetXaxis()->SetNdivisions(-32);
						HITMAP6[i]->Draw("COLZ");
						ConfirmXAxis(HITMAP6[i]);
						ReverseYAxis(HITMAP6[i]);	
						getObjectsManager()->startPublishing(HITMAP6[i]);
					}
				}




				for(int i = 0; i < NLayer; i++){

					getObjectsManager()->startPublishing(LayEtaPhi[i]);
					getObjectsManager()->startPublishing(LayChipStave[i]);
					getObjectsManager()->startPublishing(OccupancyPlot[i]);

				}

				cout << "DONE Inititing Publication = " << endl;


				RunIDPre = 0;
				FileIDPre = 0;
				bulb->SetFillColor(kRed);
				TotalFileDone = 0;



			}

			void SimpleDS::startOfActivity(Activity& activity)
			{
				QcInfoLogger::GetInstance() << "startOfActivity" << AliceO2::InfoLogger::InfoLogger::endm;

			}

			void SimpleDS::startOfCycle()
			{
				QcInfoLogger::GetInstance() << "startOfCycle" << AliceO2::InfoLogger::InfoLogger::endm;
			}

			void SimpleDS::monitorData(o2::framework::ProcessingContext& ctx)
			{




				QcInfoLogger::GetInstance() << "BEEN HERE BRO" << AliceO2::InfoLogger::InfoLogger::endm;

				int InfoFile = ctx.inputs().get<int>("Finish");


				FileFinish = InfoFile % 10;
				FileRest = (InfoFile - FileFinish)/10;

				QcInfoLogger::GetInstance() << "FileFinish = " <<  FileFinish << AliceO2::InfoLogger::InfoLogger::endm;
				QcInfoLogger::GetInstance() << "FileRest = " <<  FileRest << AliceO2::InfoLogger::InfoLogger::endm;

				//		if(FileFinish == 0) bulb->SetFillColor(kGreen);
				//		if(FileFinish == 1) bulb->SetFillColor(kRed);


				if(FileFinish == 0) bulb->SetFillColor(kGreen);
				if(FileFinish == 1 && FileRest > 1) bulb->SetFillColor(kYellow);
				if(FileFinish == 1 && FileRest == 1) bulb->SetFillColor(kRed);


				//For The Moment//

				int RunID = ctx.inputs().get<int>("Run");
				int FileID = ctx.inputs().get<int>("File");
				//QcInfoLogger::GetInstance() << "RunID IN QC = "  << runID;


				TString RunName = Form("Run%d",RunID);
				TString FileName = Form("infiles/run000%d/data-link%d",RunID,FileID);

				if(RunIDPre != RunID || FileIDPre != FileID){
					QcInfoLogger::GetInstance() << "For the Moment: RunID = "  << RunID << "  FileID = " << FileID << AliceO2::InfoLogger::InfoLogger::endm;
					FileNameInfo->Fill(0.5);
					FileNameInfo->SetTitle(Form("Current File Name: %s",FileName.Data()));
					TotalFileDone = TotalFileDone + 1;
					//InfoCanvas->SetBinContent(1,FileID);
					//InfoCanvas->SetBinContent(2,TotalFileDone);
					ptFileName->Clear();
					ptNFile->Clear();
					ptFileName->AddText(Form("File Being Proccessed: %s",FileName.Data()));
					ptNFile->AddText(Form("File Processed: %d ",TotalFileDone));
				}
				RunIDPre = RunID;
				FileIDPre = FileID;

				//Will Fix Later//


				int ResetDecision = ctx.inputs().get<int>("in");
				QcInfoLogger::GetInstance() << "Reset Histogram Decision = " << ResetDecision << AliceO2::InfoLogger::InfoLogger::endm;
				if(ResetDecision == 1) reset();

				Errors = ctx.inputs().get<const std::array<unsigned int,NError>>("Error");


				for(int i = 0; i < NError; i++){ 
					ErrorPerFile[i] = Errors[i] - ErrorPre[i];
				}

				for(int i = 0; i < NError; i++){
					QcInfoLogger::GetInstance() << " i = " << i << "   Error = "	 << Errors[i]  <<  "   ErrorPre = "	  << ErrorPre[i] <<  "   ErrorPerFile = "	  << ErrorPerFile[i]  <<  AliceO2::InfoLogger::InfoLogger::endm;
					ErrorPlots->SetBinContent(i+1,Errors[i]);
					ErrorFile->SetBinContent(FileID+1,i+1,ErrorPerFile[i]);	
				}


				if(FileFinish == 1){ 
					for(int i = 0; i < NError; i++){ 
						ErrorPre[i] = Errors[i];
					}
				}




				auto digits = ctx.inputs().get<const std::vector<o2::itsmft::Digit>>("digits");
				LOG(INFO) << "Digit Size Getting For This TimeFrame (Event) = " <<  digits.size();


				for (auto&& pixeldata : digits) {



					ChipID = pixeldata.getChipIndex();
					col = pixeldata.getColumn();
					row = pixeldata.getRow();
					NEvent = pixeldata.getROFrame();



					if (NEvent%1000000==0 && NEvent > 0) cout << "ChipID = " << ChipID << "  col = " << col << "  row = " << row << "  NEvent = " << NEvent << endl;
					//InfoCanvas->SetBinContent(3,NEvent);


					if (NEvent%1000==0 || NEventPre != NEvent){
						ptNEvent->Clear();
						ptNEvent->AddText(Form("Event Being Processed: %d",NEvent));
					}

					gm->getChipId (ChipID, lay, sta, ssta, mod, chip);
					gm->fillMatrixCache(o2::utils::bit2Mask(o2::TransformType::L2G));
					const Point3D<float> loc(0., 0.,0.); 
					auto glo = gm->getMatrixL2G(ChipID)(loc);



					if(ChipID != ChipIDPre){
						OccupancyPlot[lay]->Fill(OccupancyCounter);
						OccupancyCounter = 0;
					}
					OccupancyCounter  = OccupancyCounter + 1;


					if (lay < 7)
					{
						//cout << "lay = " <<  lay << endl;
						//cout << "ChipID = " << ChipID << endl;

						//Layer Occupancy Plot//


						int ChipNumber = (ChipID - ChipBoundary[lay])- sta*	NStaveChip[lay];

						LayEtaPhi[lay]->Fill(eta,phi);
						LayChipStave[lay]->Fill(ChipNumber,sta);


						if(sta == 0  && ChipID < NLay1){
							if(row > 0 && col > 0) HITMAP[ChipID]->Fill(col,row);
						}



						eta = glo.eta();
						phi = glo.phi();
						if(lay == 0){
							rowCS = row;
							colCS = col + NColHis * ChipNumber;
							//	cout << "ChipID in Stave 0 = " << ChipID << "  colCS = " << colCS <<  "  ChipNumber = " << ChipNumber<< endl; 
							if(row > 0 && col > 0) Lay1HIT[sta]->Fill(colCS,rowCS);
						}

						if(sta == 0 && lay == 6){
							ChipIndex6 = ChipNumber/11; 
							int ChipLocal6 = ChipNumber - ChipIndex6 * 11;
							if(ChipLocal6 < 0 ) ChipLocal6 = ChipNumber - (ChipIndex6 -1) * 11;
							rowLay6 = row;
							colLay6 = col + ChipLocal6 * NColHis;
							if(row > 0 && col > 0) HITMAP6[ChipIndex6]->Fill(colLay6,rowLay6);	
						}

					}

					NEventPre = NEvent;
					ChipIDPre = ChipID;
				}
				/*

				   for(int i = 0; i < NEvent; i++){
				   for(int j = 0; j < numOfChips; i++){
				   gm->getChipId (j, lay, sta, ssta, mod, chip);
				   if(Frequency[i][j] > 0) OccupancyPlot[lay]->Fill(Frequency[i][j]);
				   }	

				   }
				   */

				digits.clear();


				//	o2::ITSMFT::Digit digit = ctx.inputs().get<o2::ITSMFT::Digit>("digits");
				//	LOG(INFO) << "Chip ID Getting " << digit.getChipIndex() << " Row = " << digit.getRow() << "   Column = " << digit.getColumn();

				/*	
					ChipID = digit.getChipIndex();
					col = digit.getColumn();
					row = digit.getRow();

					gm->getChipId (ChipID, lay, sta, ssta, mod, chip);
					gm->fillMatrixCache(o2::utils::bit2Mask(o2::TransformType::L2G));
					Occupancy[ChipID] = Occupancy[ChipID] + 1;

				//			LOG(INFO) << "ChipID = " << ChipID << "  row = " << row << "  Column = " << col << "   OCCCUPANCY = " << Occupancy[ChipID];


				if(TotalDigits%1000==0) 	LOG(INFO) << "TotalDigits = " << TotalDigits  << "   ChipID = " << ChipID;

				int ChipNumber = (ChipID - ChipBoundary[lay])- sta*	NStaveChip[lay];
				if(sta == 0  && ChipID < NLay1){
				HIGMAP[ChipID]->Fill(col,row);
				}

				if(lay == 0){
				col = col + NColHis * ChipNumber;
				Lay1HIG[sta]->Fill(col,row);
				}

				if(sta == 0 && lay == 6){
				ChipIndex6 = ChipNumber/11; 
				int ChipLocal6 = ChipNumber - ChipIndex6 * 11;
				if(ChipLocal6 < 0 ) ChipLocal6 = ChipNumber - (ChipIndex6 -1) * 11;
				col = col + + ChipLocal6 * NColHis;
				HIGMAP6[ChipIndex6]->Fill(col,row);
				}



				for(int i = 0; i < ChipBoundary[7]; i++){
				gm->getChipId (i, lay, sta, ssta, mod, chip);

				const Point3D<float> loc(0., 0.,0.); 
				auto glo = gm->getMatrixL2G(ChipID)(loc);

				int ChipNumber = (i - ChipBoundary[lay])- sta*	NStaveChip[lay];

				eta = glo.eta();
				phi = glo.phi();

				OccupancyPlot[lay]->Fill(Occupancy[ChipID]);
				LayEtaPhi[lay]->Fill(eta,phi,Occupancy[ChipID]);
				LayChipStave[lay]->Fill(ChipNumber,sta,Occupancy[ChipID]);
				}

				TFile * foutLayCheck = new TFile("LayCheck.root","RECREATE");

				foutLayCheck->cd();
				for(int j = 0; j < 1; j++){
				for(int i = 0; i< NStaves[j]; i++){
				Lay1HIT[i]->Write();
				}
				}

				foutLayCheck->Close();
				*/

			}

			void SimpleDS::ConfirmXAxis(TH1 *h)
			{
				// Remove the current axis
				h->GetXaxis()->SetLabelOffset(999);
				h->GetXaxis()->SetTickLength(0);
				// Redraw the new axis
				gPad->Update();
				XTicks = (h->GetXaxis()->GetXmax()-h->GetXaxis()->GetXmin())/DivisionStep;

				TGaxis *newaxis = new TGaxis(gPad->GetUxmin(),
						gPad->GetUymin(),
						gPad->GetUxmax(),
						gPad->GetUymin(),
						h->GetXaxis()->GetXmin(),
						h->GetXaxis()->GetXmax(),
						XTicks,"N");
				newaxis->SetLabelOffset(0.0);
				newaxis->Draw();
				h->GetListOfFunctions()->Add(newaxis);	
			}
			void SimpleDS::ReverseYAxis(TH1 *h)
			{
				// Remove the current axis
				h->GetYaxis()->SetLabelOffset(999);
				h->GetYaxis()->SetTickLength(0);

				// Redraw the new axis
				gPad->Update();

				YTicks = (h->GetYaxis()->GetXmax()-h->GetYaxis()->GetXmin())/DivisionStep;
				TGaxis *newaxis = new TGaxis(gPad->GetUxmin(),
						gPad->GetUymax(),
						gPad->GetUxmin()-0.001,
						gPad->GetUymin(),
						h->GetYaxis()->GetXmin(),
						h->GetYaxis()->GetXmax(),
						YTicks,"N");

				newaxis->SetLabelOffset(0);
				newaxis->Draw();
				h->GetListOfFunctions()->Add(newaxis);

			}





			void SimpleDS::endOfCycle()
			{
				QcInfoLogger::GetInstance() << "endOfCycle" << AliceO2::InfoLogger::InfoLogger::endm;

			}

			void SimpleDS::endOfActivity(Activity& activity)
			{
				QcInfoLogger::GetInstance() << "endOfActivity" << AliceO2::InfoLogger::InfoLogger::endm;
			}

			void SimpleDS::reset()
			{
				// clean all the monitor objects here
				QcInfoLogger::GetInstance() << "Resetting the histogram" << AliceO2::InfoLogger::InfoLogger::endm;
				ChipStave->Reset();
				for(int i = 0; i < NLayer; i++){
					OccupancyPlot[i]->Reset();
					LayEtaPhi[i]->Reset();
					LayChipStave[i]->Reset();
				}

				for(int j = 0; j < 1; j++){
					for(int i = 0; i< NStaves[j]; i++){
						Lay1HIT[i]->Reset();
					}
				}

				for(int j = 0; j < 1; j++){
					for(int i = 0; i < NStaveChip[j]; i++){
						HITMAP[i]->Reset();
					}
				}

				for(int j = 6; j < 7; j++){
					for(int i = 0; i < 18; i++){
						HITMAP6[i]->Reset();
					}
				}
				ErrorPlots->Reset();
				NEventInRun = 0;
				ErrorFile->Reset();	
				TotalFileDone = 0;

				QcInfoLogger::GetInstance() << "DONE the histogram Resetting" << AliceO2::InfoLogger::InfoLogger::endm;

			}

		} // namespace simpleds
	} // namespace quality_control_modules
} // namespace o2

