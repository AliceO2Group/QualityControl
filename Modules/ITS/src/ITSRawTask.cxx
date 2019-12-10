///
/// \file   ITSRawTask.cxx
/// \author Zhaozhong Shi
/// \author Markus Keil
/// \author Mario Sitta
/// \brief  ITS QC task for raw data analysis
///

#include <algorithm>
#include <sstream>
#include <iostream>
#include <math.h>
#include <TStopwatch.h>
#include "DataFormatsParameters/GRPObject.h"
#include "FairLogger.h"
#include "FairRunAna.h"
#include "FairFileSource.h"
#include "FairRuntimeDb.h"
#include "FairParRootFileIo.h"
#include "FairSystemInfo.h"
#include "TPaletteAxis.h"

#include "QualityControl/QcInfoLogger.h"
#include "ITS/ITSRawTask.h"
#include "ITS/ITSTaskVariables.h"
#include "DetectorsBase/GeometryManager.h"
#include "ITSBase/GeometryTGeo.h"
#include "ITSMFTReconstruction/DigitPixelReader.h"


using o2::itsmft::Digit;

using namespace std;
using namespace o2::itsmft;
using namespace o2::its;

namespace o2 {
namespace quality_control_modules {
namespace itsrawtask {

ITSRawTask::ITSRawTask()
    :
    TaskInterface()
{

  o2::base::GeometryManager::loadGeometry();
  gStyle->SetPadRightMargin(0.15);
  gStyle->SetPadLeftMargin(0.15);
  m_objects.clear();
  m_publishedObjects.clear();
  enableLayers(); 

  createHistos();

  for (int i = 0; i < NError; i++) {
    Errors[i] = 0;
    ErrorPre[i] = 0;
    ErrorPerFile[i] = 0;
  }

  gStyle->SetOptFit(0);
  gStyle->SetOptStat(0);
}

ITSRawTask::~ITSRawTask()
{

}

void ITSRawTask::initialize(o2::framework::InitContext &ctx)
{
  QcInfoLogger::GetInstance() << "initialize ITSRawTask" << AliceO2::InfoLogger::InfoLogger::endm;

  o2::its::GeometryTGeo *geom = o2::its::GeometryTGeo::Instance();
  geom->fillMatrixCache(o2::utils::bit2Mask(o2::TransformType::L2G));
  int numOfChips = geom->getNumberOfChips();
  cout << "numOfChips = " << numOfChips << endl;
  setNChips(numOfChips);

  for (int i = 0; i < NError; i++) {
    pt[i] = new TPaveText(0.20, 0.80 - i * 0.05, 0.85, 0.85 - i * 0.05, "NDC");
    formatPaveText(pt[i], 0.04, gStyle->GetTextColor(), 12, ErrorType[i].Data());
    hErrorPlots->GetListOfFunctions()->Add(pt[i]);
  }

  ptFileName = new TPaveText(0.20, 0.40, 0.85, 0.50, "NDC");
  ptFileName->SetName("");
  formatPaveText(ptFileName, 0.04, gStyle->GetTextColor(), 12, "Current File Processing: ");

  ptNFile = new TPaveText(0.20, 0.30, 0.85, 0.40, "NDC");
  ptFileName->SetName("FileName");
  formatPaveText(ptNFile, 0.04, gStyle->GetTextColor(), 12, "File Processed: ");

  ptNEvent = new TPaveText(0.20, 0.20, 0.85, 0.30, "NDC");
  ptNEvent->SetName("NEvent");
  formatPaveText(ptNEvent, 0.04, gStyle->GetTextColor(), 12, "Event Processed: ");

  bulbRed = new TPaveText(0.60, 0.75, 0.90, 0.85, "NDC");
  bulbRed->SetName("BulbRed");
  formatPaveText(bulbRed, 0.04, kRed, 12, "Red = QC Waiting");

  bulbYellow = new TPaveText(0.60, 0.65, 0.90, 0.75, "NDC");
  bulbYellow->SetName("BulbYellow");
  formatPaveText(bulbYellow, 0.04, kYellow, 12, "Yellow = QC Pausing");

  bulbGreen = new TPaveText(0.60, 0.55, 0.90, 0.65, "NDC");
  bulbGreen->SetName("BulbGreen");
  formatPaveText(bulbGreen, 0.04, kGreen, 12, "Green= QC Processing");

  hInfoCanvas->SetTitle("QC Process Information Canvas");
  hInfoCanvas->GetListOfFunctions()->Add(ptFileName);
  hInfoCanvas->GetListOfFunctions()->Add(ptNFile);
  hInfoCanvas->GetListOfFunctions()->Add(ptNEvent);
  hInfoCanvas->GetListOfFunctions()->Add(bulb);
  hInfoCanvas->GetListOfFunctions()->Add(bulbRed);
  hInfoCanvas->GetListOfFunctions()->Add(bulbYellow);
  hInfoCanvas->GetListOfFunctions()->Add(bulbGreen);

  //		InfoCanvas->SetStats(false);

  publishHistos();

  cout << "DONE Inititing Publication = " << endl;

  bulb->SetFillColor(kRed);
  TotalFileDone = 0;
  TotalHisTime = 0;
  Counted = 0;
  Yellowed = 0;
}

void ITSRawTask::startOfActivity(Activity &activity)
{
  QcInfoLogger::GetInstance() << "startOfActivity" << AliceO2::InfoLogger::InfoLogger::endm;

}

void ITSRawTask::startOfCycle()
{
  QcInfoLogger::GetInstance() << "startOfCycle" << AliceO2::InfoLogger::InfoLogger::endm;
}

void ITSRawTask::monitorData(o2::framework::ProcessingContext &ctx)
{
  double eta, phi;
  int lay, sta, ssta, mod, chip;
  UShort_t col, row, ChipID;
  std::chrono::time_point<std::chrono::high_resolution_clock> start;
  std::chrono::time_point<std::chrono::high_resolution_clock> startLoop;
  std::chrono::time_point<std::chrono::high_resolution_clock> end;
  int difference;

  start = std::chrono::high_resolution_clock::now();

  ofstream timefout("HisTimeGlobal.dat", ios::app);

  ofstream timefout2("HisTimeLoop.dat", ios::app);

  QcInfoLogger::GetInstance() << "BEEN HERE BRO" << AliceO2::InfoLogger::InfoLogger::endm;

  int FileID = ctx.inputs().get<int>("File");
  int EPID = ctx.inputs().get<int>("EP");
  getProcessStatus(ctx.inputs().get<int>("Finish"), FileFinish);
  updateFile(ctx.inputs().get<int>("Run"), ctx.inputs().get<int>("EP"), FileID);

  //Will Fix Later//

  int ResetDecision = ctx.inputs().get<int>("in");
  QcInfoLogger::GetInstance() << "Reset Histogram Decision = " << ResetDecision
      << AliceO2::InfoLogger::InfoLogger::endm;
  if (ResetDecision == 1)
    reset();

  auto digits = ctx.inputs().get<const std::vector<o2::itsmft::Digit>>("digits");
  LOG(INFO) << "Digit Size Getting For This TimeFrame (Event) = " << digits.size();

  Errors = ctx.inputs().get<const std::array<unsigned int, NError>>("Error");

  for (int i = 0; i < NError; i++) {
    ErrorPerFile[i] = Errors[i] - ErrorPre[i];
  }

  for (int i = 0; i < NError; i++) {
    QcInfoLogger::GetInstance() << " i = " << i << "   Error = " << Errors[i] << "   ErrorPre = " << ErrorPre[i]
        << "   ErrorPerFile = " << ErrorPerFile[i] << AliceO2::InfoLogger::InfoLogger::endm;
    hErrorPlots->SetBinContent(i + 1, Errors[i]);
    hErrorFile->SetBinContent((FileID + 1 + (EPID-4)*12), i + 1, ErrorPerFile[i]);
  }

  if (FileFinish == 1) {
    for (int i = 0; i < NError; i++) {
      ErrorPre[i] = Errors[i];
    }
  }

  difference = std::chrono::duration_cast < std::chrono::milliseconds > (end - start).count();
  //	QcInfoLogger::GetInstance() << "Before Loop = " << difference/1000.0 << "s" <<  AliceO2::InfoLogger::InfoLogger::endm;
  timefout << "Before Loop  = " << difference / 1000.0 << "s" << std::endl;

  for (auto &&pixeldata : digits) {
    startLoop = std::chrono::high_resolution_clock::now();

    ChipID = pixeldata.getChipIndex();
    col = pixeldata.getColumn();
    row = pixeldata.getRow();
    NEvent = pixeldata.getROFrame();
    //cout << "Event Compare: " << NEvent << ", " << NEventPre << endl;

    if (NEvent % occUpdateFrequency == 0 && NEvent > 0 && NEvent != NEventPre) {
      updateOccupancyPlots (NEventPre);
     // cout << "Carried out, " << NEventPre << endl;
    }
  

 // cout << "NFrame = " << NEvent  << endl;
    if (Counted < TotalCounted) {
      end = std::chrono::high_resolution_clock::now();
      difference = std::chrono::duration_cast < std::chrono::nanoseconds > (end - startLoop).count();
      //	QcInfoLogger::GetInstance() << "Before Geo = " << difference << "ns" <<  AliceO2::InfoLogger::InfoLogger::endm;
      timefout2 << "Getting Value Time  = " << difference << "ns" << std::endl;
    }

    if (NEvent % 1000000 == 0 && NEvent > 0)
      cout << "ChipID = " << ChipID << "  col = " << col << "  row = " << row << "  NEvent = " << NEvent << endl;

    // wouldnt this update this update the text for every digit in events 1000, 2000 ... ?
    if (NEvent % 1000 == 0 || NEventPre != NEvent) {
      ptNEvent->Clear();
      ptNEvent->AddText(Form("Event Being Processed: %d", NEvent));
    }

    if (Counted < TotalCounted) {
      end = std::chrono::high_resolution_clock::now();
      difference = std::chrono::duration_cast < std::chrono::nanoseconds > (end - startLoop).count();
      //	QcInfoLogger::GetInstance() << "Before Geo = " << difference << "ns" <<  AliceO2::InfoLogger::InfoLogger::endm;
      timefout2 << "Before Geo  = " << difference << "ns" << std::endl;
    }

    gm->getChipId(ChipID, lay, sta, ssta, mod, chip);
   // cout << "getID : " << ChipID << ", " << lay << ", " << sta << ", " << ssta << ", " << mod << ", " << chip << endl;
    gm->fillMatrixCache(o2::utils::bit2Mask(o2::TransformType::L2G));
    const Point3D<float> loc(0., 0., 0.);
    auto glo = gm->getMatrixL2G(ChipID)(loc);

    if (!layerEnable[lay]) continue;

    if (Counted < TotalCounted) {
      end = std::chrono::high_resolution_clock::now();
      difference = std::chrono::duration_cast < std::chrono::nanoseconds > (end - startLoop).count();
      //	QcInfoLogger::GetInstance() << "After Geo = " << difference << "ns" <<  AliceO2::InfoLogger::InfoLogger::endm;
      timefout2 << "After Geo =  " << difference << "ns" << std::endl;
    }

    int hicCol, hicRow;
    // Todo: check if chipID is really chip ID
    getHicCoordinates(lay, chip, col, row, hicRow, hicCol);
  //  if (lay == 0 && sta == 0 && ssta == 0 && mod == 0 && chip == 0) cout << ChipID << ", " << col << ", " << row << endl;
    hHicHitmap[lay][sta][mod]->Fill(hicCol, hicRow);
    if (lay > NLayerIB && chip > 6) {
      // OB HICs: take into account that chip IDs are 0 .. 6, 8 .. 14
      hChipHitmap[lay][sta][mod][chip-1]->Fill(col, row);
    }
    else {
      hChipHitmap[lay][sta][mod][chip]->Fill(col, row);
     // hIBHitmap[lay]->Fill(hicCol, row+(sta*NRowHis));
    }

    if (Counted < TotalCounted) {
      end = std::chrono::high_resolution_clock::now();
      difference = std::chrono::duration_cast < std::chrono::nanoseconds > (end - startLoop).count();
      //	QcInfoLogger::GetInstance() << "After Geo = " << difference << "ns" <<  AliceO2::InfoLogger::InfoLogger::endm;
      timefout2 << "Fill HitMaps =  " << difference << "ns" << std::endl;
    }

    if (Counted < TotalCounted) {
      end = std::chrono::high_resolution_clock::now();
      difference = std::chrono::duration_cast < std::chrono::nanoseconds > (end - startLoop).count();
      //	QcInfoLogger::GetInstance() << "After Geo = " << difference << "ns" <<  AliceO2::InfoLogger::InfoLogger::endm;
      timefout2 << "Before glo etaphi =  " << difference << "ns" << std::endl;
    }

    eta = glo.eta();
    phi = glo.phi();
    hEtaPhiHitmap[lay]->Fill(eta, phi);

    if (Counted < TotalCounted) {
      end = std::chrono::high_resolution_clock::now();
      difference = std::chrono::duration_cast < std::chrono::nanoseconds > (end - startLoop).count();
      //	QcInfoLogger::GetInstance() << "After Geo = " << difference << "ns" <<  AliceO2::InfoLogger::InfoLogger::endm;
      timefout2 << "After glo etaphi =  " << difference << "ns" << std::endl;
      Counted = Counted + 1;
    }

    NEventPre = NEvent;

  } // end digits loop
 
  if(NEventPre > 0) updateOccupancyPlots(NEventPre);
  //cout << "EndUpdateOcc " << NEventPre <<endl;
  end = std::chrono::high_resolution_clock::now();
  difference = std::chrono::duration_cast < std::chrono::milliseconds > (end - start).count();
  QcInfoLogger::GetInstance() << "Time After Loop = " << difference / 1000.0 << "s"
      << AliceO2::InfoLogger::InfoLogger::endm;
  timefout << "Time After Loop = " << difference / 1000.0 << "s" << std::endl;

  cout << "NEventDone = " << NEvent << endl;
  cout <<  "Test  " << endl;

  digits.clear();

  end = std::chrono::high_resolution_clock::now();
  difference = std::chrono::duration_cast < std::chrono::milliseconds > (end - start).count();
  TotalHisTime = TotalHisTime + difference;
  QcInfoLogger::GetInstance() << "Time in Histogram = " << difference / 1000.0 << "s"
      << AliceO2::InfoLogger::InfoLogger::endm;
  timefout << "Time in Histogram = " << difference / 1000.0 << "s" << std::endl;

  if (NEvent == 0 && ChipID == 0 && row == 0 && col == 0 && Yellowed == 0) {
    bulb->SetFillColor(kYellow);
    Yellowed = 1;
  }

}

void ITSRawTask::addObject(TObject* aObject, bool published)
{
  if (!aObject) {
    std::cout << "ERROR: trying to add non-existent object" << std::endl;
    return;
  }
  m_objects.push_back(aObject);
  if (published) {
    m_publishedObjects.push_back(aObject);
  }
}

void ITSRawTask::createHistos()
{
  createGlobalHistos();

  for (int iLayer = 0; iLayer < NLayer; iLayer++) {
    if (!layerEnable[iLayer]) continue;
    createLayerHistos(iLayer);
  }
}

void ITSRawTask::createGlobalHistos()
{
  hErrorPlots = new TH1D("General/ErrorPlots", "Decoding Errors", NError, 0.5, NError + 0.5);
  formatAxes(hErrorPlots, "Error ID", "Counts");
  hErrorPlots->SetMinimum(0);
  hErrorPlots->SetFillColor(kRed);

  hFileNameInfo = new TH1D("General/FileNameInfo", "FileNameInfo", 5, 0, 1);
  formatAxes(hFileNameInfo, "InputFile", "Total Files Processed", 1.1);

  hErrorFile = new TH2D("General/ErrorFile", "Decoding Errors vs File ID", NFiles + 1, -0.5, NFiles + 0.5, NError, 0.5, NError + 0.5);
  formatAxes(hErrorFile, "File ID (data-link)", "Error ID");
  formatStatistics(hErrorFile);
  //format2DZaxis(hErrorFile);
  hErrorFile->GetZaxis()->SetTitle("Counts");
  hErrorFile->SetMinimum(0);

  hInfoCanvas = new TH1D("General/InfoCanvas", "InfoCanvas", 3, -0.5, 2.5);
  bulb = new TEllipse(0.2, 0.75, 0.30, 0.20);

  addObject(hErrorPlots);
  addObject(hFileNameInfo);
  addObject(hErrorFile);
  addObject(hInfoCanvas);
}

void ITSRawTask::createLayerHistos(int aLayer)
{
  createEtaPhiHitmap(aLayer);
  createChipStaveOcc(aLayer);

  // 1d- occupancy histogram of the full layer, x-axis units = log (occupancy)
  hOccupancyPlot[aLayer] = new TH1D(Form("Occupancy/Layer%dOccupancy", aLayer),
				    Form("ITS Layer %d Occupancy Distribution", aLayer), 300, -15, 0);
  formatAxes(hOccupancyPlot[aLayer], "Occupancy", "Counts", 1., 2.2);
  addObject(hOccupancyPlot[aLayer]);
/*
//HITMAPS of the full inner layers
  int maxX, maxY, nBinsX, nBinsY;


  maxX = 9 * NColHis;
  maxY = NStaves[aLayer] * NRowHis;
  nBinsX = maxX / SizeReduce;
  nBinsY = maxY / SizeReduce;
  if(aLayer < NLayerIB){
  hIBHitmap[aLayer] = new TH2I(Form("Occupancy/Layer%dHITMAP", aLayer),
                               Form("Hits on Layer %d", aLayer), nBinsX, 0, maxX, nBinsY, 0,  maxY);
  addObject(hIBHitmap[aLayer]);
  }
*/
  // HITMAPS per HIC, binning in groups of SizeReduce * SizeReduce pixels
  // chipHitmap: fine binning, one hitmap per chip, but not to be saved to CCDB (only for determination of noisy pixels)
  for (int iStave = 0; iStave < NStaves[aLayer]; iStave ++) {
    createStaveHistos(aLayer, iStave);
  }
}

// hChipStaveOccupancy: Occupancy histograms for complete layer
// y-axis: number of stave
// x-axis: number of chip (IB) or number of HIC (OB)
void ITSRawTask::createChipStaveOcc(int aLayer)
{
  int nBinsX;
  if (aLayer < NLayerIB) {
    nBinsX = nChipsPerHic[aLayer];
    hChipStaveOccupancy[aLayer] = new TH2D(Form("Occupancy/Layer%d/Layer%dChipStave", aLayer, aLayer),
        Form("ITS Layer%d, Occupancy vs Chip and Stave", aLayer), nBinsX, -.5 , nBinsX-.5 , NStaves[aLayer], -.5, NStaves[aLayer]-.5);
    formatAxes(hChipStaveOccupancy[aLayer], "Chip Number", "Stave Number", 1., 1.1);
    formatStatistics(hChipStaveOccupancy[aLayer]);
   // format2DZaxis(hChipStaveOccupancy[aLayer]);
  }
  else {
    nBinsX = nHicPerStave[aLayer];
    hChipStaveOccupancy[aLayer] = new TH2D(Form("Occupancy/Layer%d/Layer%dHicStave", aLayer, aLayer),
        Form("ITS Layer%d, Occupancy vs Hic and Stave", aLayer), nBinsX, -.5 , nBinsX-.5 , NStaves[aLayer], -.5, NStaves[aLayer]-.5);
    formatAxes(hChipStaveOccupancy[aLayer], "Hic Number", "Stave Number", 1., 1.1);
    formatStatistics(hChipStaveOccupancy[aLayer]);
   // format2DZaxis(hChipStaveOccupancy[aLayer]);
  }

  hChipStaveOccupancy[aLayer]->GetZaxis()->SetTitle("Number of Hits");
  hChipStaveOccupancy[aLayer]->GetZaxis()->SetTitleOffset(1.4);
  addObject(hChipStaveOccupancy[aLayer]);
}

// hEtaPhiHitmap: eta-phi hitmaps for complete layers, binning 100 x 100 pixels
// using eta coverage of TDR, assuming phi runs from 0 ... 2*Pi
void ITSRawTask::createEtaPhiHitmap(int aLayer)
{
  int NEta, NPhi;
  if (aLayer < NLayerIB) {
    NEta = 9 * 10;
    NPhi = NStaves[aLayer] * 5;
  }
  else {
    NEta = nHicPerStave[aLayer] * 70;
    NPhi = NStaves[aLayer] * 10;
  }
  hEtaPhiHitmap[aLayer] = new TH2I(Form("Occupancy/Layer%d/Layer%dEtaPhi", aLayer, aLayer),
      Form("ITS Layer%d, Hits vs Eta and Phi", aLayer), NEta, (-1)*etaCoverage[aLayer], etaCoverage[aLayer], NPhi, PhiMin, PhiMax);
  formatAxes(hEtaPhiHitmap[aLayer], "#eta", "#phi", 1., 1.1);
  hEtaPhiHitmap[aLayer]->GetZaxis()->SetTitle("Number of Hits");
  hEtaPhiHitmap[aLayer]->GetZaxis()->SetTitleOffset(1.4);
  addObject(hEtaPhiHitmap[aLayer]);
}

void ITSRawTask::createStaveHistos(int aLayer, int aStave)
{
  for (int iHic = 0; iHic < nHicPerStave[aLayer]; iHic++) {
     createHicHistos(aLayer, aStave, iHic);
  }
}


void ITSRawTask::createHicHistos(int aLayer, int aStave, int aHic)
{
  TString Name, Title;
  int nBinsX, nBinsY, maxX, maxY, nChips;

  if (aLayer < NLayerIB) {
    Name = Form("Occupancy/Layer%d/Stave%d/Layer%dStave%dHITMAP", aLayer, aStave, aLayer, aStave);
    Title = Form("Hits on Layer %d, Stave %d", aLayer, aStave);
    maxX = 9 * NColHis;
    maxY = NRowHis;
nChips = 9;
  }
  else {
    Name = Form("Occupancy/Layer%d/Stave%d/HIC%d/Layer%dStave%dHIC%dHITMAP", aLayer, aStave, aHic, aLayer, aStave, aHic);
Title = Form("Hits on Layer %d, Stave %d, Hic %d", aLayer, aStave, aHic);
    maxX = 7 * NColHis;
    maxY = 2 * NRowHis;
    nChips = 14;
  }
  nBinsX = maxX / SizeReduce;
  nBinsY = maxY / SizeReduce;
  hHicHitmap[aLayer][aStave][aHic] = new TH2I(Name, Title, nBinsX, 0, maxX, nBinsY, 0,  maxY);
  formatAxes(hHicHitmap[aLayer][aStave][aHic], "Column", "Row", 1., 1.1);
  // formatting, moved here from initialize
  hHicHitmap[aLayer][aStave][aHic]->GetZaxis()->SetTitleOffset(1.50);
  hHicHitmap[aLayer][aStave][aHic]->GetZaxis()->SetTitle("Number of Hits");
  hHicHitmap[aLayer][aStave][aHic]->GetXaxis()->SetNdivisions(-32);
  hHicHitmap[aLayer][aStave][aHic]->Draw("COLZ"); // should this really be drawn here?
  addObject(hHicHitmap[aLayer][aStave][aHic]);

  for (int iChip = 0; iChip < nChips; iChip ++) {
    hChipHitmap[aLayer][aStave][aHic][iChip] = new TH2I(Form("chipHitmapL%dS%dH%dC%d", aLayer, aStave, aHic, iChip),
    Form("chipHitmapL%dS%dH%dC%d", aLayer, aStave, aHic, iChip),  1024, -.5, 1023.5, 512, -.5, 511.5);
    addObject(hChipHitmap[aLayer][aStave][aHic][iChip], false);
  }
}

// To be checked:
// - something like this should exist in the official geometry already
// - is aChip really the chipID (i.e. 0..6, 8.. 14 in case of OB HICs)?
void ITSRawTask::getHicCoordinates (int aLayer, int aChip, int aCol, int aRow, int& aHicRow, int& aHicCol)
{
  aChip &= 0xf;
  if (aLayer < NLayerIB) {
    aHicCol = aChip * NCols + aCol;
    aHicRow = aRow;
  }
  else { // OB Hic: chip row 0 at center of HIC
    if (aChip < 7) {
      aHicCol = aChip * NCols + aCol;
      aHicRow = NRows - aRow - 1;
    }
    else {
      aHicRow = NRows + aRow;
      aHicCol = 7 * NCols - ((aChip - 8) * NCols + aCol);
    }
  }
}


void ITSRawTask::formatStatistics(TH2 *h){

  h->SetStats(0);


}
/*
void ITSRawTask::format2DZaxis(TH2 *h){

  TPaletteAxis *palette = (TPaletteAxis*)h->GetListOfFunctions()->FindObject("palette");
  palette->SetLabelSize(0.5);
 // TPad *pad = ()

}
*/
void ITSRawTask::formatAxes(TH1 *h, const char* xTitle, const char* yTitle, float xOffset, float yOffset)
{
  h->GetXaxis()->SetTitle(xTitle);
  h->GetYaxis()->SetTitle(yTitle);
  h->GetXaxis()->SetTitleOffset(xOffset);
  h->GetYaxis()->SetTitleOffset(yOffset);
}

void ITSRawTask::formatPaveText(TPaveText *aPT, float aTextSize, Color_t aTextColor, short aTextAlign, const char *aText)
{
  aPT->SetTextSize(aTextSize);
  aPT->SetTextAlign(aTextAlign);
  aPT->SetFillColor(0);
  aPT->SetTextColor(aTextColor);
  aPT->AddText(aText);
}

void ITSRawTask::ConfirmXAxis(TH1 *h)
{
  // Remove the current axis
  h->GetXaxis()->SetLabelOffset(999);
  h->GetXaxis()->SetTickLength(0);
  // Redraw the new axis
  gPad->Update();
  int XTicks = (h->GetXaxis()->GetXmax() - h->GetXaxis()->GetXmin()) / DivisionStep;

  TGaxis *newaxis = new TGaxis(gPad->GetUxmin(), gPad->GetUymin(), gPad->GetUxmax(), gPad->GetUymin(),
      h->GetXaxis()->GetXmin(), h->GetXaxis()->GetXmax(), XTicks, "N");
  newaxis->SetLabelOffset(0.0);
  newaxis->Draw();
  h->GetListOfFunctions()->Add(newaxis);
}

void ITSRawTask::ReverseYAxis(TH1 *h)
{
  // Remove the current axis
  h->GetYaxis()->SetLabelOffset(999);
  h->GetYaxis()->SetTickLength(0);

  // Redraw the new axis
  gPad->Update();

  int YTicks = (h->GetYaxis()->GetXmax() - h->GetYaxis()->GetXmin()) / DivisionStep;
  TGaxis *newaxis = new TGaxis(gPad->GetUxmin(), gPad->GetUymax(), gPad->GetUxmin() - 0.001, gPad->GetUymin(),
      h->GetYaxis()->GetXmin(), h->GetYaxis()->GetXmax(), YTicks, "N");

  newaxis->SetLabelOffset(0);
  newaxis->Draw();
  h->GetListOfFunctions()->Add(newaxis);

}

void ITSRawTask::publishHistos()
{
  for (unsigned int iObj = 0; iObj < m_publishedObjects.size(); iObj++) {
    getObjectsManager()->startPublishing(m_publishedObjects.at(iObj));
  }
}

void ITSRawTask::addMetadata(int runID, int EpID, int fileID)
{
  for (unsigned int iObj = 0; iObj < m_publishedObjects.size(); iObj++) {
    getObjectsManager()->addMetadata(m_publishedObjects.at(iObj)->GetName(), "Run", Form("%d", runID));
    getObjectsManager()->addMetadata(m_publishedObjects.at(iObj)->GetName(), "EP", Form("%d", EpID));
    getObjectsManager()->addMetadata(m_publishedObjects.at(iObj)->GetName(), "File", Form("%d", fileID));
  }
}

void ITSRawTask::getProcessStatus(int aInfoFile, int& aFileFinish)
{
  aFileFinish = aInfoFile % 10;
  //cout<<"aInfoFile = "<<aInfoFile<<endl;
  FileRest = (aInfoFile - aFileFinish) / 10;

  QcInfoLogger::GetInstance() << "FileFinish = " << aFileFinish << AliceO2::InfoLogger::InfoLogger::endm;
  QcInfoLogger::GetInstance() << "FileRest = " << FileRest << AliceO2::InfoLogger::InfoLogger::endm;

  if (aFileFinish == 0)
    bulb->SetFillColor(kGreen);
  if (aFileFinish == 1 && FileRest > 1)
    bulb->SetFillColor(kYellow);
  if (aFileFinish == 1 && FileRest == 1)
    bulb->SetFillColor(kRed);
}

void ITSRawTask::updateFile(int aRunID, int aEpID, int aFileID)
{

  static int RunIDPre=0, FileIDPre=0;
  if (RunIDPre != aRunID || FileIDPre != aFileID) {
    TString FileName = Form("infiles/run000%d/data-ep%d-link%d", aRunID, aEpID, aFileID);
    QcInfoLogger::GetInstance() << "For the Moment: RunID = " << aRunID << "  FileID = " << aFileID
        << AliceO2::InfoLogger::InfoLogger::endm;
    hFileNameInfo->Fill(0.5);
    hFileNameInfo->SetTitle(Form("Current File Name: %s", FileName.Data()));
    TotalFileDone = TotalFileDone + 1;
    //hInfoCanvas->SetBinContent(1,FileID);
    //hInfoCanvas->SetBinContent(2,TotalFileDone);
    ptFileName->Clear();
    ptNFile->Clear();
    ptFileName->AddText(Form("File Being Proccessed: %s", FileName.Data()));
    ptNFile->AddText(Form("File Processed: %d ", TotalFileDone));

    addMetadata(aRunID, aEpID, aFileID);
  }
  RunIDPre = aRunID;
  FileIDPre = aFileID;
}

void ITSRawTask::endOfCycle()
{
  QcInfoLogger::GetInstance() << "endOfCycle" << AliceO2::InfoLogger::InfoLogger::endm;

}

void ITSRawTask::endOfActivity(Activity &activity)
{
  QcInfoLogger::GetInstance() << "endOfActivity" << AliceO2::InfoLogger::InfoLogger::endm;
}

void ITSRawTask::reset()
{
  // clean all the monitor objects here
  QcInfoLogger::GetInstance() << "Resetting the histogram" << AliceO2::InfoLogger::InfoLogger::endm;

  TotalFileDone = 0;
  ptNFile->Clear();
  ptNFile->AddText(Form("File Processed: %d ", TotalFileDone));
  Yellowed = 0;

  resetHitmaps();
  resetOccupancyPlots();

  QcInfoLogger::GetInstance() << "DONE the histogram Resetting" << AliceO2::InfoLogger::InfoLogger::endm;
}

// reset method for all plots that are supposed to be reset once
void ITSRawTask::resetHitmaps()
{
  hErrorPlots->Reset();
  hErrorFile->Reset();

// for (int iLayer = 0; iLayer < NLayerIB; iLayer ++) {
 
   // if (!layerEnable[iLayer]) continue;
   // hIBHitmap[iLayer]->Reset();
  
 //}


  for (int iLayer = 0; iLayer < NLayer; iLayer ++) {
    if (!layerEnable[iLayer]) continue;
    hEtaPhiHitmap[iLayer]->Reset();
   // hIBHitmap[iLayer]->Reset();
    for (int iStave = 0; iStave < NStaves[iLayer]; iStave ++) {
      for (int iHic = 0; iHic < nHicPerStave[iLayer]; iHic++) {
        hHicHitmap[iLayer][iStave][iHic]->Reset();
        for (int iChip = 0; iChip < nChipsPerHic[iLayer]; iChip ++) {
          hChipHitmap[iLayer][iStave][iHic][iChip]->Reset();
        }
      }
    }
  }
}

// reset method for all histos that are to be reset regularly
// (occupancy plots when recalculating / updating the occupancies)
void ITSRawTask::resetOccupancyPlots()
{
  for (int iLayer = 0; iLayer < NLayer; iLayer++) {
    if (!layerEnable[iLayer]) continue;
    hOccupancyPlot[iLayer]->Reset();
    hChipStaveOccupancy[iLayer]->Reset();
  }
}

void ITSRawTask::updateOccupancyPlots(int nEvents)
{
  double pixelOccupancy, chipOccupancy;

  resetOccupancyPlots();

  for (int iLayer = 0; iLayer < NLayer; iLayer ++) {
    if (!layerEnable[iLayer]) continue;
    //hEtaPhiHitmap[iLayer]->Reset();
    for (int iStave = 0; iStave < NStaves[iLayer]; iStave ++) {
      for (int iHic = 0; iHic < nHicPerStave[iLayer]; iHic++) {
        for (int iChip = 0; iChip < nChipsPerHic[iLayer]; iChip ++) {
          chipOccupancy = hChipHitmap[iLayer][iStave][iHic][iChip]->Integral();
          chipOccupancy = chipOccupancy/((double)nEvents * (double)NPixels);
         // cout << "chipOcc = " << hChipHitmap[iLayer][iStave][iHic][iChip]->Integral() << ", " << iLayer << ", " << iStave << ", " << iHic << ", " << iChip << endl;
          if (iLayer < NLayerIB) {
            hChipStaveOccupancy[iLayer]->Fill(iChip, iStave, chipOccupancy);
          } else {
            hChipStaveOccupancy[iLayer]->Fill(iHic, iStave, chipOccupancy / nChipsPerHic[iLayer]);
          }
          for (int iCol = 0; iCol < NCols; iCol++){
            for (int iRow = 0; iRow < NRows; iRow ++) {
              pixelOccupancy = hChipHitmap[iLayer][iStave][iHic][iChip]->GetBinContent(iCol + 1, iRow + 1);
              if (pixelOccupancy > 0) {
                pixelOccupancy /= (double)nEvents;
                hOccupancyPlot[iLayer]->Fill(log10(pixelOccupancy));
              }
            }
          }
        }
      }
    }
  }
}

 

void ITSRawTask::enableLayers(){

  string mRunType, mEventPerPush, mTrackError, mWorkDir, enable[7];
  std::ifstream RunFileType("Config/RunType.dat");
  RunFileType >> mRunType;

  if (mRunType == "FakeHitRate") {
    std::ifstream EventPush("Config/ConfigFakeRate.dat");
    EventPush >> mEventPerPush >> mTrackError >> mWorkDir >> enable[0] >>enable[1] >>enable[2] >>enable[3] >>enable[4] >>enable[5] >>enable[6];
  }
  if (mRunType == "ThresholdScan") {
    std::ifstream EventPush("Config/ConfigThreshold.dat");
    EventPush >> mEventPerPush >> mTrackError >> mWorkDir >> enable[0] >>enable[1] >>enable[2] >>enable[3] >>enable[4] >>enable[5] >>enable[6];   
  }
  for (int i=0; i<7; i++) layerEnable[i] = stoi(enable[i]);
  //cout << "LayerEnable = " << " layer0 = " << layerEnable[0] << " layer5 = " << layerEnable[5] <<  endl;
}


} // namespace simpleds
} // namespace quality_control_modules
} // namespace o2
