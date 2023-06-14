
#include <TCanvas.h>
#include <TH1.h>
#include <TH2.h>
#include <TLine.h>
#include <TH1F.h>
#include <TH2F.h>
#include <TProfile.h>
#include <TProfile2D.h>
#include <TStopwatch.h>
#include <THashList.h>

#include "DataFormatsTRD/Constants.h"
#include "DataFormatsTRD/Digit.h"
#include "DataFormatsTRD/Tracklet64.h"
#include "DataFormatsTRD/TriggerRecord.h"
#include "QualityControl/ObjectsManager.h"
#include "QualityControl/TaskInterface.h"
#include "QualityControl/QcInfoLogger.h"
#include "TRD/DigitsTask.h"
#include <Framework/InputRecord.h>
#include <Framework/InputRecordWalker.h>
#include <gsl/span>
#include <map>
#include <tuple>

#include "CCDB/BasicCCDBManager.h"
namespace o2::quality_control_modules::trd
{
DigitsTask::~DigitsTask()
{
}

void DigitsTask::retrieveCCDBSettings()
{
  if (auto param = mCustomParameters.find("ccdbtimestamp"); param != mCustomParameters.end()) {
    mTimestamp = std::stol(mCustomParameters["ccdbtimestamp"]);
    ILOG(Debug, Support) << "configure() : using ccdbtimestamp = " << mTimestamp << ENDM;
  } else {
    mTimestamp = o2::ccdb::getCurrentTimestamp();
    ILOG(Debug, Support) << "configure() : using default timestam of now = " << mTimestamp << ENDM;
  }
  auto& mgr = o2::ccdb::BasicCCDBManager::instance();
  mgr.setTimestamp(mTimestamp);
  mNoiseMap = mgr.get<o2::trd::NoiseStatusMCM>("/TRD/Calib/NoiseMapMCM");
  if (mNoiseMap == nullptr) {
    ILOG(Info, Support) << "mNoiseMap is null, no noisy mcm reduction" << ENDM;
  }
  mChamberStatus = mgr.get<o2::trd::HalfChamberStatusQC>("/TRD/Calib/HalfChamberStatusQC");
  if (mChamberStatus == nullptr) {
    ILOG(Info, Support) << "mChamberStatus is null, no chamber status to display" << ENDM;
  }
}

void DigitsTask::buildChamberIgnoreBP()
{
  mChambersToIgnoreBP.reset();
  // Vector of string to save tokens
  std::vector<std::string> tokens;
  // stringstream class check1
  std::stringstream check1(mChambersToIgnore);
  std::string intermediate;
  // Tokenizing w.r.t. space ','
  while (getline(check1, intermediate, ',')) {
    tokens.push_back(intermediate);
  }
  // Printing the token vector
  for (auto& token : tokens) {
    // token now holds something like 16_3_0
    std::vector<std::string> parts;
    std::stringstream indexcheck(token);
    std::string tokenpart;
    // Tokenizing w.r.t. space ','
    while (getline(indexcheck, tokenpart, '_')) {
      parts.push_back(tokenpart);
    }
    // now flip the bit related to the sector:stack:layer stored in parts.
    int sector = std::stoi(parts[0]);
    int stack = std::stoi(parts[1]);
    int layer = std::stoi(parts[2]);
    mChambersToIgnoreBP.set(sector * o2::trd::constants::NSTACK * o2::trd::constants::NLAYER + stack * o2::trd::constants::NLAYER + layer);
  }
}

void DigitsTask::drawTrdLayersGrid(TH2F* hist)
{
  TLine* line;
  for (int i = 0; i < 5; ++i) {
    switch (i) {
      case 0:
        line = new TLine(15.5, 0, 15.5, 2592);
        hist->GetListOfFunctions()->Add(line);
        line->SetLineStyle(kDashed);
        line->SetLineColor(kBlack);
        break;
      case 1:
        line = new TLine(31.5, 0, 31.5, 2592);
        hist->GetListOfFunctions()->Add(line);
        line->SetLineStyle(kDashed);
        line->SetLineColor(kBlack);
        break;
      case 2:
        line = new TLine(43.5, 0, 43.5, 2592);
        hist->GetListOfFunctions()->Add(line);
        line->SetLineStyle(kDashed);
        line->SetLineColor(kBlack);
        break;
      case 3:
        line = new TLine(59.5, 0, 59.5, 2592);
        hist->GetListOfFunctions()->Add(line);
        line->SetLineStyle(kDashed);
        line->SetLineColor(kBlack);
        break;
    }
  }
  for (int iSec = 1; iSec < 18; ++iSec) {
    float yPos = iSec * 144 - 0.5;
    line = new TLine(0, yPos, 76, yPos);
    line->SetLineStyle(kDashed);
    line->SetLineColor(kBlack);
    hist->GetListOfFunctions()->Add(line);
  }

  ILOG(Info, Support) << "Layer Grid redrawn in check for : " << hist->GetName() << ENDM;
  ILOG(Info, Support) << "Layer Grid redrawn in hist has function count of :  " << hist->GetListOfFunctions()->GetSize() << ENDM;
}

void DigitsTask::drawLinesMCM(TH2F* histo)
{
  TLine* l;
  Int_t nPos[o2::trd::constants::NSTACK - 1] = { 16, 32, 44, 60 };

  for (Int_t iStack = 0; iStack < o2::trd::constants::NSTACK - 1; ++iStack) {
    l = new TLine(nPos[iStack] - 0.5, -0.5, nPos[iStack] - 0.5, 47.5);
    l->SetLineStyle(2);
    histo->GetListOfFunctions()->Add(l);
  }

  for (Int_t iLayer = 0; iLayer < o2::trd::constants::NLAYER; ++iLayer) {
    l = new TLine(0.5, iLayer * 8 - 0.5, 75.5, iLayer * 8 - 0.5);
    l->SetLineStyle(2);
    histo->GetListOfFunctions()->Add(l);
  }
}

void DigitsTask::drawLinesOnPulseHeight(TH1F* h)
{
  TLine* lmin = new TLine(mPulseHeightPeakRegion.first, 0, mPulseHeightPeakRegion.first, 1e9);
  TLine* lmax = new TLine(mPulseHeightPeakRegion.second, 0, mPulseHeightPeakRegion.second, 1e9);
  lmin->SetLineStyle(2);
  lmax->SetLineStyle(2);
  lmin->SetLineColor(kRed);
  lmax->SetLineColor(kRed);
  h->GetListOfFunctions()->Add(lmin);
  h->GetListOfFunctions()->Add(lmax);
}

void DigitsTask::drawHashOnLayers(int layer, int hcid, int col, int rowstart, int rowend)
{
  // instead of using overlays, draw a simple box in red with a cross on it.
  std::pair<float, float> topright, bottomleft; // coordinates of box
  TLine* boxlines[8];
  int det = hcid / 2;
  int side = hcid % 2;
  int sec = hcid / 60;
  bottomleft.first = rowstart - 0.5;
  bottomleft.second = (sec * 2 + side) * 72;
  topright.first = rowend - 0.5;
  topright.second = (sec * 2 + side) * 72 + 72;

  // LOG(info) << "Box for layer : " << layer << " hcid : " << hcid << ": " << bottomleft.first << ":" << bottomleft.second << "(" << bottomleft.second/144 << ") -- " << topright.first << ":" << topright.second << "(" << topright.second/144 << ")";
  boxlines[0] = new TLine(bottomleft.first, bottomleft.second, topright.first, bottomleft.second);                                                                                     // bottom
  boxlines[1] = new TLine(bottomleft.first, topright.second, topright.first, topright.second);                                                                                         // top
  boxlines[2] = new TLine(bottomleft.first, bottomleft.second, bottomleft.first, topright.second);                                                                                     // left
  boxlines[3] = new TLine(topright.first, bottomleft.second, topright.first, topright.second);                                                                                         // right
  boxlines[4] = new TLine(bottomleft.first, topright.second - (topright.second - bottomleft.second) / 2, topright.first, topright.second - (topright.second - bottomleft.second) / 2); // horizontal middle
  boxlines[5] = new TLine(topright.first, bottomleft.second, bottomleft.first, topright.second);                                                                                       // backslash
  boxlines[6] = new TLine(bottomleft.first, bottomleft.second, topright.first, topright.second);                                                                                       // forwardslash
  boxlines[7] = new TLine(bottomleft.first + (topright.first - bottomleft.first) / 2, bottomleft.second, bottomleft.first + (topright.first - bottomleft.first) / 2, topright.second); // vertical middle
  for (int line = 0; line < 8; ++line) {
    boxlines[line]->SetLineColor(kBlack);
    mLayers[layer]->GetListOfFunctions()->Add(boxlines[line]);
  }
}

void DigitsTask::fillLinesOnHistsPerLayer(int iLayer)
{
  std::bitset<1080> hciddone;
  hciddone.reset();
  if (mChamberStatus == nullptr) {
    // protect for if the chamber status is not pulled from the ccdb for what ever reason.
    ILOG(Info, Support) << "No half chamber status object to be able to fill the mask" << ENDM;
  } else {
    for (int iSec = 0; iSec < 18; ++iSec) {
      for (int iStack = 0; iStack < 5; ++iStack) {
        int rowMax = (iStack == 2) ? 12 : 16;
        for (int side = 0; side < 2; ++side) {
          int det = iSec * 30 + iStack * 6 + iLayer;
          int hcid = (side == 0) ? det * 2 : det * 2 + 1;
          int rowstart = iStack < 3 ? iStack * 16 : 44 + (iStack - 3) * 16;                 // pad row within whole sector
          int rowend = iStack < 3 ? rowMax + iStack * 16 : rowMax + 44 + (iStack - 3) * 16; // pad row within whole sector
          if (mChamberStatus->isMasked(hcid) && (!hciddone.test(hcid))) {
            drawHashOnLayers(iLayer, hcid, 0, rowstart, rowend);
            hciddone.set(hcid); // to avoid mutliple instances of adding lines
          }
        }
      }
    }
  }
}

void DigitsTask::buildHistograms()
{
  mDigitHCID.reset(new TH1F("digithcid", "Digit distribution over Halfchambers", 1080, 0, 1080));
  getObjectsManager()->startPublishing(mDigitHCID.get());
  mDigitsPerEvent.reset(new TH1F("digitsperevent", "Digits per Event", 10000, 0, 10000));
  getObjectsManager()->startPublishing(mDigitsPerEvent.get());
  mEventswDigitsPerTimeFrame.reset(new TH1F("eventswithdigitspertimeframe", "Number of Events with Digits per Time Frame", 100, 0, 100));
  getObjectsManager()->startPublishing(mEventswDigitsPerTimeFrame.get());

  mDigitsSizevsTrackletSize.reset(new TH2F("digitsvstracklets", "Tracklets Count vs Digits Count per event; Number of Tracklets;Number Of Digits", 2500, 0, 2500, 2500, 0, 2500));
  getObjectsManager()->startPublishing(mDigitsSizevsTrackletSize.get());

  mClsAmpCh.reset(new TH1F("Cluster/ClsAmpCh", "Reconstructed mean amplitude;Amplitude (ADC);# chambers", 100, 25, 125));
  mClsAmpCh->GetXaxis()->SetTitle("Amplitude (ADC)");
  mClsAmpCh->GetYaxis()->SetTitle("# chambers");
  getObjectsManager()->startPublishing(mClsAmpCh.get());

  mClsNTb.reset(new TH1F("Cluster/ClsNTb", "ClsNTb", 30, -0.5, 29.5));
  mClsNTb->SetTitle("Clusters per time bin;Number of clusters;Timebin");
  getObjectsManager()->startPublishing(mClsNTb.get());

  mClsChargeFirst.reset(new TH2F("Cluster/ClsChargeFirst", "ClsChargeFirst;Timebin of max Signal;Timebin", 100, 0, 1000, 101, -0.2, 0.2));
  getObjectsManager()->startPublishing(mClsChargeFirst.get());
  getObjectsManager()->setDefaultDrawOptions(mClsChargeFirst->GetName(), "COLZ");

  mClsChargeTb.reset(new TH1F("Cluster/ClsChargeTb", "ClsChargeTb;", 30, -0.5, 29.5));
  getObjectsManager()->startPublishing(mClsChargeTb.get());

  mClsChargeTbCycle.reset(new TH1F("Cluster/ClsChargeTbCycle", "ClsChargeTb cleared per cycle;Cluster charge; Timebin", 30, -0.5, 29.5));
  getObjectsManager()->startPublishing(mClsChargeTbCycle.get());

  mClsAmp.reset(new TH1F("Cluster/ClsAmp", "Amplitude of clusters;Amplitdu(ADC);Counts", 200, -0.5, 1999.5));
  getObjectsManager()->startPublishing(mClsAmp.get());

  mClsAmpDrift.reset(new TH1F("Cluster/ClsAmpDrift", "Amplitude of clusters in drift region;Amplitdu(ADC);Counts", 500, -0.5, 999.5));
  getObjectsManager()->startPublishing(mClsAmpDrift.get());

  for (Int_t layer = 0; layer < o2::trd::constants::NLAYER; ++layer) {
    std::string label = fmt::format("Cluster/NClsLayer_{0}", layer);
    std::string title = fmt::format("Illumination layer {0};Sectors;Padrows (z)", layer);
    mNClsLayer[layer].reset(new TH2F(label.c_str(), title.c_str(), 18 * 8, -0.5, 17.5, 76, -0.5, 75.5));
    getObjectsManager()->startPublishing(mNClsLayer[layer].get());
    getObjectsManager()->setDefaultDrawOptions(label.c_str(), "COLZ");
  }
  mADCvalue.reset(new TH1D("ADCvalue", "ADC value distribution;ADC value;Counts", 500, -0.5, 499.5));
  getObjectsManager()->startPublishing(mADCvalue.get());
  char buffer[256];
  for (Int_t iSM = 0; iSM < o2::trd::constants::NSECTOR; ++iSM) {

    std::string label = fmt::format("DigitsPerMCM/HCMCM_{0}", iSM);
    std::string title = fmt::format("MCM in Digits data stream for sector {0}; Pad Row; Pad Column", iSM);
    mHCMCM[iSM].reset(new TH2F(label.c_str(), title.c_str(), 76, -0.5, 75.5, 8 * 6, -0.5, 8 * 6 - 0.5));
    getObjectsManager()->startPublishing(mHCMCM[iSM].get());
    getObjectsManager()->setDefaultDrawOptions(mHCMCM[iSM]->GetName(), "COLZ");
    drawLinesMCM(mHCMCM[iSM].get());

    label = fmt::format("Cluster/ClsDetAmp_{0}", iSM);
    title = fmt::format("Cluster amplitude chamberwise in SM {0};Chamber;Amplitude", iSM);
    mClsDetAmp[iSM].reset(new TH2F(label.c_str(), title.c_str(), 30, -0.5, 29.5, 500, -0.5, 999.5));
    getObjectsManager()->startPublishing(mClsDetAmp[iSM].get());
    getObjectsManager()->setDefaultDrawOptions(mClsDetAmp[iSM]->GetName(), "COLZ");

    label = fmt::format("ADCTB_{0}", iSM);
    title = fmt::format("Signal in Timebins for sector {0};Timebin;ADC", iSM);
    mADCTB[iSM].reset(new TH2F(label.c_str(), title.c_str(), 30, -0.5, 29.5, 128, -0.5, 127.5));
    getObjectsManager()->startPublishing(mADCTB[iSM].get());
    getObjectsManager()->setDefaultDrawOptions(mADCTB[iSM]->GetName(), "COLZ");

    label = fmt::format("ADCTBfull_{0}", iSM);
    title = fmt::format("Signal in Timebins full for sector {0};Timebin;ADC", iSM);
    mADCTBfull[iSM].reset(new TH2F(label.c_str(), title.c_str(), 30, -0.5, 29.5, 128, -0.5, 127.5));
    getObjectsManager()->startPublishing(mADCTBfull[iSM].get());
    getObjectsManager()->setDefaultDrawOptions(mADCTBfull[iSM]->GetName(), "COLZ");

    label = fmt::format("ADC_{0}", iSM);
    title = fmt::format("ADC value distribution for sector {0};ADC value;Counts", iSM);
    mADC[iSM].reset(new TH1F(label.c_str(), title.c_str(), 500, -0.5, 499.5));
    getObjectsManager()->startPublishing(mADC[iSM].get());

    // clusters
    label = fmt::format("Cluster/ClsSM_{0}", iSM);
    title = fmt::format("Cluster for Sector {0};ADC value;Counts", iSM);
    mClsSM[iSM].reset(new TH1F(label.c_str(), title.c_str(), 100, 0, 200));
    getObjectsManager()->startPublishing(mClsSM[iSM].get());

    label = fmt::format("Cluster/ClsTbSM_{0}", iSM);
    title = fmt::format("Cluster Timbin  for sector {0};Timebin;ADC", iSM);
    mClsTbSM[iSM].reset(new TH2F(label.c_str(), title.c_str(), 30, -0.5, 29.5, 10, 0, 200));
    getObjectsManager()->startPublishing(mClsTbSM[iSM].get());
    getObjectsManager()->setDefaultDrawOptions(mClsTbSM[iSM]->GetName(), "COLZ");
  }

  mNCls.reset(new TH1F("Cluster/NCls", "Total number of clusters per sector", 18, -0.5, 17.5));
  mNCls->SetTitle("Total number of clusters per sector;Sector;Counts");
  getObjectsManager()->startPublishing(mNCls.get());

  mNClsAmp.reset(new TH1F("Cluster/NClsAmp", "Total number of clusters per sector;Sector;Counts", 200, -0.5, 199.5));
  getObjectsManager()->startPublishing(mNClsAmp.get());

  mClsTb.reset(new TH2F("Cluster/ClsTb", "Cluster  vs. timebin;Timebin;Cluster", 30, -0.5, 29.5, 200, 0, 2000));
  mClsTb->GetXaxis()->SetTitle("Timebin");
  mClsTb->GetYaxis()->SetTitle("Cluster");
  // getObjectsManager()->startPublishing(mClsTb.get());

  mClsAmpTb.reset(new TH1F("Cluster/ClsAmpTb", "Cluster amplitude vs. timebin;Timebin;Amplitude", 30, -0.5, 29.5));
  // getObjectsManager()->startPublishing(mClsAmpTb.get());

  mClsSector.reset(new TH2F("Cluster/ClsSector", "Cluster amplidue per sector;Sector,Amplitude", 18, -0.5, 17.5, 500, -0.5, 999.5));
  getObjectsManager()->startPublishing(mClsSector.get());

  mClsStack.reset(new TH2F("Cluster/ClsStack", "Cluster amplitude per stack;Stack;Amplitude", 5, -0.5, 4.5, 500, -0.5, 999.5));
  getObjectsManager()->startPublishing(mClsStack.get());

  // local histos
  std::string label = fmt::format("Cluster/clsAmp");
  std::string title = fmt::format("Cluster Amplitude for chamber;Cluster Amplidute;Chamber");
  mClusterAmplitudeChamber.reset(new TH2F(label.c_str(), title.c_str(), 300, -0.5, 299.5, 540, -0.5, 539.5));
  getObjectsManager()->startPublishing(mClusterAmplitudeChamber.get());

  mClsChargeTbTigg.reset(new TH1F("Cluster/ClsChargeTbTrgg", "Total charge vs. time bin;timebin; totalcharge", 30, -0.5, 29.5));
  getObjectsManager()->startPublishing(mClsChargeTbTigg.get());

  mPulseHeight.reset(new TH1F("PulseHeight/mPulseHeight", Form("Pulse height plot threshold:%i;Timebins;Counts", mPulseHeightThreshold), 30, -0.5, 29.5));
  drawLinesOnPulseHeight(mPulseHeight.get());
  getObjectsManager()->startPublishing(mPulseHeight.get());
  mPulseHeight.get()->GetYaxis()->SetTickSize(0.01);
  mPulseHeightn.reset(new TH1F("mPulseHeight2nonoise", "Pulse height plot v2 excluding noise;TimeBins;Counts", 30, -0.5, 29.5));
  drawLinesOnPulseHeight(mPulseHeightn.get());
  getObjectsManager()->startPublishing(mPulseHeightn.get());
  mPulseHeightn.get()->GetYaxis()->SetTickSize(0.01);

  mPulseHeightScaled.reset(new TH1F("PulseHeight/mPulseHeightScaled", "Scaled Pulse height plot;TimeBins;Scaled Counts", 30, -0.5, 29.5));
  getObjectsManager()->startPublishing(mPulseHeightScaled.get());
  mPulseHeightScaled.get()->GetYaxis()->SetTickSize(0.01);

  mTotalPulseHeight2D.reset(new TH2F("PulseHeight/TotalPulseHeight", "Total Pulse Height;Timebins;Summed charge of neighbors", 30, 0., 30., 200, 0., 200.));
  getObjectsManager()->startPublishing(mTotalPulseHeight2D.get());
  getObjectsManager()->setDefaultDrawOptions(mTotalPulseHeight2D->GetName(), "COLZ");

  mPulseHeightpro.reset(new TProfile("PulseHeight/mPulseHeightpro", "Pulse height profile  plot;Timebins;Counts", 30, -0.5, 29.5));
  mPulseHeightpro.get()->Sumw2();
  getObjectsManager()->startPublishing(mPulseHeightpro.get());

  mPulseHeightperchamber.reset(new TProfile2D("PulseHeight/mPulseHeightperchamber", "mPulseHeightperchamber;Timebin;Chamber", 30, -0.5, 29.5, 540, 0, 540));
  mPulseHeightperchamber.get()->Sumw2();
  getObjectsManager()->startPublishing(mPulseHeightperchamber.get());
  getObjectsManager()->setDefaultDrawOptions(mPulseHeightperchamber.get()->GetName(), "colz");

  for (int count = 0; count < 18; ++count) {
    std::string label = fmt::format("PulseHeight/pulseheight2d_sm_{0:02d}", count);
    std::string title = fmt::format("Pulse Height Spectrum for SM {0:02d};Timebin;Counts", count);
    TH1F* h = new TH1F(label.c_str(), title.c_str(), 30, -0.5, 29.5);
    mPulseHeight2DperSM[count].reset(h);
    getObjectsManager()->startPublishing(h);
  }

  for (int iLayer = 0; iLayer < 6; ++iLayer) {
    mLayers.push_back(new TH2F(Form("DigitsPerLayer/layer%i", iLayer), Form("Digit count per pad in layer %i;stack;sector", iLayer), 76, -0.5, 75.5, 2592, -0.5, 2591.5));
    auto xax = mLayers.back()->GetXaxis();
    auto yax = mLayers[iLayer]->GetYaxis();
    if (!mLayerLabelsIgnore) {
      xax->SetNdivisions(5);
      xax->SetBinLabel(8, "0");
      xax->SetBinLabel(24, "1");
      xax->SetBinLabel(38, "2");
      xax->SetBinLabel(52, "3");
      xax->SetBinLabel(68, "4");
      xax->SetTicks("");
      xax->SetTickSize(0.0);
      xax->SetLabelSize(0.045);
      xax->SetLabelOffset(0.005);
      xax->SetTitleOffset(0.95);
      xax->CenterTitle(true);
      yax->SetNdivisions(18);
      for (int iSec = 0; iSec < 18; ++iSec) {
        auto lbl = std::to_string(iSec);
        yax->SetBinLabel(iSec * 144 + 72, lbl.c_str());
      }
      yax->SetTicks("");
      yax->SetTickSize(0.0);
      yax->SetLabelSize(0.045);
      yax->SetLabelOffset(0.001);
      yax->SetTitleOffset(0.40);
      yax->CenterTitle(true);
    }
    mLayers.back()->SetStats(0);
    drawTrdLayersGrid(mLayers.back());
    fillLinesOnHistsPerLayer(iLayer);

    getObjectsManager()->startPublishing(mLayers.back());
    getObjectsManager()->setDefaultDrawOptions(mLayers.back()->GetName(), "COLZ");
    getObjectsManager()->setDisplayHint(mLayers.back(), "logz");
  }
}

void DigitsTask::initialize(o2::framework::InitContext& /*ctx*/)
{
  ILOG(Debug, Devel) << "initialize TRDDigitQcTask" << ENDM; // QcInfoLogger is used. FairMQ logs will go to there as well.

  // this is how to get access to custom parameters defined in the config file at qc.tasks.<task_name>.taskParameters
  if (auto param = mCustomParameters.find("driftregionstart"); param != mCustomParameters.end()) {
    mDriftRegion.first = stof(param->second);
    ILOG(Debug, Support) << "configure() : using peakregionstart = " << mDriftRegion.first << ENDM;
  } else {
    mDriftRegion.first = 7.0;
    ILOG(Debug, Support) << "configure() : using default dritfregionstart = " << mDriftRegion.first << ENDM;
  }
  if (auto param = mCustomParameters.find("driftregionend"); param != mCustomParameters.end()) {
    mDriftRegion.second = stof(param->second);
    ILOG(Debug, Support) << "configure() : using peakregionstart = " << mDriftRegion.second << ENDM;
  } else {
    mDriftRegion.second = 20.0;
    ILOG(Debug, Support) << "configure() : using default dritfregionstart = " << mDriftRegion.second << ENDM;
  }
  if (auto param = mCustomParameters.find("pulseheightpeaklower"); param != mCustomParameters.end()) {
    mPulseHeightPeakRegion.first = stof(param->second);
    ILOG(Debug, Support) << "configure() : using pulsehheightlower	= " << mPulseHeightPeakRegion.first << ENDM;
  } else {
    mPulseHeightPeakRegion.first = 0.0;
    ILOG(Debug, Support) << "configure() : using default pulseheightlower = " << mPulseHeightPeakRegion.first << ENDM;
  }
  if (auto param = mCustomParameters.find("pulseheightpeakupper"); param != mCustomParameters.end()) {
    mPulseHeightPeakRegion.second = stof(param->second);
    ILOG(Debug, Support) << "configure() : using pulsehheightupper	= " << mPulseHeightPeakRegion.second << ENDM;
  } else {
    mPulseHeightPeakRegion.second = 5.0;
    ILOG(Debug, Support) << "configure() : using default pulseheightupper = " << mPulseHeightPeakRegion.second << ENDM;
  }
  if (auto param = mCustomParameters.find("skippedshareddigits"); param != mCustomParameters.end()) {
    mSkipSharedDigits = stod(param->second);
    ILOG(Debug, Support) << "configure() : using skip shared digits 	= " << mSkipSharedDigits << ENDM;
  } else {
    mSkipSharedDigits = false;
    ILOG(Debug, Support) << "configure() : using default skip shared digits = " << mSkipSharedDigits << ENDM;
  }
  if (auto param = mCustomParameters.find("pulseheightthreshold"); param != mCustomParameters.end()) {
    mPulseHeightThreshold = stod(param->second);
    ILOG(Debug, Support) << "configure() : using skip shared digits 	= " << mPulseHeightThreshold << ENDM;
  } else {
    mPulseHeightThreshold = 400;
    ILOG(Debug, Support) << "configure() : using default skip shared digits = " << mPulseHeightThreshold << ENDM;
  }
  if (auto param = mCustomParameters.find("chamberstoignore"); param != mCustomParameters.end()) {
    mChambersToIgnore = param->second;
    ILOG(Debug, Support) << "configure() : chamberstoignore = " << mChambersToIgnore << ENDM;
  } else {
    mChambersToIgnore = "16_3_0";
    ILOG(Debug, Support) << "configure() : chambers to ignore for pulseheight calculations = " << mChambersToIgnore << ENDM;
  }
  if (auto param = mCustomParameters.find("ignorelayerlabels"); param != mCustomParameters.end()) {
    mLayerLabelsIgnore = stoi(param->second);
    ILOG(Debug, Support) << "configure() : ignoring labels on layer plots = " << mLayerLabelsIgnore << ENDM;
  }
  buildChamberIgnoreBP();

  retrieveCCDBSettings();
  buildHistograms();
}

void DigitsTask::startOfActivity(const Activity& /*activity*/)
{
  ILOG(Debug, Devel) << "startOfActivity" << ENDM;
} // set stats/stacs

void DigitsTask::startOfCycle()
{
  ILOG(Debug, Devel) << "startOfCycle" << ENDM;
}

bool digitIndexCompare(unsigned int A, unsigned int B, const std::vector<o2::trd::Digit>& originalDigits)
{
  // sort into ROC:padrow
  const o2::trd::Digit *a, *b;
  a = &originalDigits[A];
  b = &originalDigits[B];
  if (a->getDetector() < b->getDetector()) {
    return 1;
  }
  if (a->getDetector() > b->getDetector()) {
    return 0;
  }
  if (a->getPadRow() < b->getPadRow()) {
    return 1;
  }
  if (a->getPadRow() > b->getPadRow()) {
    return 0;
  }
  if (a->getPadCol() < b->getPadCol()) {
    return 1;
  }
  return 0;
}
bool DigitsTask::isChamberToBeIgnored(unsigned int sm, unsigned int stack, unsigned int layer)
{
  // just to make the calling method a bit cleaner
  return mChambersToIgnoreBP.test(sm * o2::trd::constants::NLAYER * o2::trd::constants::NSTACK + stack * o2::trd::constants::NLAYER + layer);
}

void DigitsTask::monitorData(o2::framework::ProcessingContext& ctx)
{
  for (auto&& input : ctx.inputs()) {
    if (input.header != nullptr && input.payload != nullptr) {

      auto digits = ctx.inputs().get<gsl::span<o2::trd::Digit>>("digits");
      std::vector<o2::trd::Digit> digitv(digits.begin(), digits.end());

      if (digitv.size() == 0)
        continue;
      auto tracklets = ctx.inputs().get<gsl::span<o2::trd::Tracklet64>>("tracklets"); // still deciding if we will ever need the tracklets here.
      auto triggerrecords = ctx.inputs().get<gsl::span<o2::trd::TriggerRecord>>("triggers");
      // create a vector indexing into the digits in question
      std::vector<unsigned int> digitsIndex(digitv.size());
      std::iota(digitsIndex.begin(), digitsIndex.end(), 0);
      int DigitTrig = 0;
      // we now have sorted digits, can loop sequentially and be going over det/row/pad
      for (auto& trigger : triggerrecords) {
        uint64_t numtracklets = trigger.getNumberOfTracklets();
        uint64_t numdigits = trigger.getNumberOfDigits();
        if (trigger.getNumberOfDigits() == 0)
          continue; // bail if we have no digits in this trigger
                    // now sort digits to det,row,pad
        DigitTrig++;
        std::sort(std::begin(digitsIndex) + trigger.getFirstDigit(), std::begin(digitsIndex) + trigger.getFirstDigit() + trigger.getNumberOfDigits(),
                  [&digitv](unsigned int i, unsigned int j) { return digitIndexCompare(i, j, digitv); });

        ///////////////////////////////////////////////////
        // Go through all chambers (using digits)
        const int adcThresh = 13;
        const int clsCutoff = 1000;
        const int startRow[5] = { 0, 16, 32, 44, 60 };
        if (trigger.getNumberOfDigits() > 10000) {
          mDigitsPerEvent->Fill(9999);
        } else {
          mDigitsPerEvent->Fill(trigger.getNumberOfDigits());
        }
        int trackletnumfix = trigger.getNumberOfTracklets();
        int digitnumfix = trigger.getNumberOfDigits();
        if (trigger.getNumberOfTracklets() > 2499)
          trackletnumfix = 2499;
        if (trigger.getNumberOfDigits() > 24999)
          trackletnumfix = 24999;
        mDigitsSizevsTrackletSize->Fill(trigger.getNumberOfTracklets(), trigger.getNumberOfDigits());
        int tbmax = 0;
        int tbhi = 0;
        int tblo = 0;

        int det = 0;
        int row = 0;
        int pad = 0;
        int channel = 0;

        for (int currentdigit = trigger.getFirstDigit() + 1; currentdigit < trigger.getFirstDigit() + trigger.getNumberOfDigits() - 1; ++currentdigit) { // -1 and +1 as we are looking for consecutive digits pre and post the current one indexed.
          if (digits[digitsIndex[currentdigit]].getChannel() > 21)
            continue;
          int detector = digits[digitsIndex[currentdigit]].getDetector();
          if (detector < 0 || detector >= 540) {
            // for some reason online the sm value causes an error below and digittrask crashes.
            // the only possibility is detector number is invalid
            ILOG(Info, Support) << "Bad detector number from digit : " << detector << " for digit index of " << digitsIndex[currentdigit] << ENDM;
            continue;
          }
          int sm = detector / 30;
          int detLoc = detector % 30;
          int layer = detector % 6;
          int stack = detLoc / 6;
          int chamber = sm * 30 + stack * o2::trd::constants::NLAYER + layer;
          int nADChigh = 0;
          int stackoffset = stack * o2::trd::constants::NROWC1;
          int row = 0, col = 0;
          if (stack >= 2)
            stackoffset -= 4; // account for stack 2 having 4 less.
          // for now the if statement is commented as there is a problem finding isShareDigit, will come back to that.
          /// if (!digits[digitsIndex[currentdigit]].isSharedDigit() && !mSkipSharedDigits.second) {
          int rowGlb = stack < 3 ? digits[digitsIndex[currentdigit]].getPadRow() + stack * 16 : digits[digitsIndex[currentdigit]].getPadRow() + 44 + (stack - 3) * 16; // pad row within whole sector
          int colGlb = digits[digitsIndex[currentdigit]].getPadCol() + sm * 144;                                                                                       // pad column number from 0 to NSectors * 144
          mLayers[layer]->Fill(rowGlb, colGlb);
          //}
          mHCMCM[sm]->Fill(digits[digitsIndex[currentdigit]].getPadRow() + stackoffset, digits[digitsIndex[currentdigit]].getPadCol());
          mDigitHCID->Fill(digits[digitsIndex[currentdigit]].getHCId());
          // after updating the 2 above histograms the first and last digits are of no use, as we are looking for 3 neighbouring digits after this.

          auto adcs = digits[digitsIndex[currentdigit]].getADC();
          row = digits[digitsIndex[currentdigit]].getPadRow();
          pad = digits[digitsIndex[currentdigit]].getPadCol();
          int sector = detector / 30;
          // do we have 3 digits next to each other:
          std::tuple<unsigned int, unsigned int, unsigned int> aa, ba, ca;
          aa = std::make_tuple(digits[digitsIndex[currentdigit - 1]].getDetector(), digits[digitsIndex[currentdigit - 1]].getPadRow(), digits[digitsIndex[currentdigit - 1]].getPadCol());
          ba = std::make_tuple(digits[digitsIndex[currentdigit]].getDetector(), digits[digitsIndex[currentdigit]].getPadRow(), digits[digitsIndex[currentdigit]].getPadCol());
          ca = std::make_tuple(digits[digitsIndex[currentdigit + 1]].getDetector(), digits[digitsIndex[currentdigit + 1]].getPadRow(), digits[digitsIndex[currentdigit + 1]].getPadCol());
          auto [det1, row1, col1] = aa;
          auto [det2, row2, col2] = ba;
          auto [det3, row3, col3] = ca;
          // do we have 3 digits next to each other:
          // check we 3 consecutive adc
          bool consecutive = false;
          if (det1 == det2 && det2 == det3 && row1 == row2 && row2 == row3 && col1 + 1 == col2 && col2 + 1 == col3) {
            consecutive = true;
          }
          // illumination
          mNClsLayer[layer]->Fill(sm - 0.5 + col / 144., startRow[stack] + row);
          int digitindex = digitsIndex[currentdigit];
          int digitindexbelow = digitsIndex[currentdigit - 1];
          int digitindexabove = digitsIndex[currentdigit + 1];
          for (int time = 1; time < o2::trd::constants::TIMEBINS - 1; ++time) {
            int value = digits[digitsIndex[currentdigit]].getADC()[time];
            if (value > adcThresh)
              nADChigh++;

            mADCvalue->Fill(value);
            mADC[sm]->Fill(value);
            mADCTB[sm]->Fill(time, value);
            mADCTBfull[sm]->Fill(time, value);

            if (consecutive) {
              // clusterize
              if (value > digits[digitindexbelow].getADC()[time] &&
                  value > digits[digitindexabove].getADC()[time]) {
                // How do we determine this? HV/LME/other?
                // if(ChamberHasProblems(sm,istack,layer)) continue;

                value -= 10;

                int value = digits[digitindex].getADC()[time];
                int valueLU = (digits[digitindexbelow].getADC()[time - 1] < 10) ? 0 : digits[digitindexbelow].getADC()[time - 1] - 10;
                int valueRU = (digits[digitindexabove].getADC()[time - 1] < 10) ? 0 : digits[digitindexabove].getADC()[time - 1] - 10;
                int valueU = digits[digitindex].getADC()[time - 1] - 10;

                int valueLD = (digits[digitindexbelow].getADC()[time + 1] < 10) ? 0 : digits[digitindexbelow].getADC()[time + 1] - 10;
                int valueRD = (digits[digitindexabove].getADC()[time + 1] < 10) ? 0 : digits[digitindexabove].getADC()[time + 1] - 10;
                int valueD = digits[digitindex].getADC()[time + 1] - 10;

                int valueL = (digits[digitindexbelow].getADC()[time] < 10) ? 0 : digits[digitindexbelow].getADC()[time] - 10;
                int valueR = (digits[digitindexabove].getADC()[time] < 10) ? 0 : digits[digitindexabove].getADC()[time] - 10;

                int sum = value + valueL + valueR;
                int sumU = valueU + valueLU + valueRU;
                int sumD = valueD + valueLD + valueRD;

                if (sumU < 10 || sumD < 10)
                  continue;
                if (TMath::Abs(1. * sum / sumU - 1) < 0.01)
                  continue;
                if (TMath::Abs(1. * sum / sumD - 1) < 0.01)
                  continue;
                if (TMath::Abs(1. * sumU / sumD - 1) < 0.01)
                  continue;

                mNCls->Fill(sm);
                mClsSM[sm]->Fill(sum);
                mClsTbSM[sm]->Fill(time, sum);
                mClsTb->Fill(time, sum);
                mClsChargeFirst->Fill(sum, (1. * sum / sumU) - 1.);
                mClsChargeFirst->Fill(sum, (1. * sum / sumD) - 1.);

                if (sum > 10 && sum < clsCutoff) {
                  mClsChargeTb->Fill(time, sum);
                  mClsChargeTbCycle->Fill(time, sum);
                  mClsNTb->Fill(time);
                }

                mClsAmp->Fill(sum);

                if (time > mDriftRegion.first && time < mDriftRegion.second) {
                  mClsAmpDrift->Fill(sum);
                  mClsDetAmp[sm]->Fill(detLoc, sum);
                  mClusterAmplitudeChamber->Fill(sum, chamber);
                }

                mClsSector->Fill(sm, sum);
                mClsStack->Fill(stack, sum);

                // This is pulseheight lifted from run2, probably not what was used.
                mClsChargeTbTigg->Fill(time, sum);

              } // if consecutive adc ( 3 next to each other and we are on the middle on
            }   // loop over time-bins
          }     // loop over col-pads

          const o2::trd::Digit* mid = &digits[digitsIndex[currentdigit]];
          const o2::trd::Digit* before = &digits[digitsIndex[currentdigit - 1]];
          const o2::trd::Digit* after = &digits[digitsIndex[currentdigit + 1]];
          uint32_t suma = before->getADCsum();
          uint32_t sumb = mid->getADCsum();
          uint32_t sumc = after->getADCsum();
          if (!isChamberToBeIgnored(sm, stack, layer)) {
            if (det1 == det2 && det2 == det3 && row1 == row2 && row2 == row3 && col1 + 1 == col2 && col2 + 1 == col3) {
              if (sumb > suma && sumb > sumc) {
                if (suma > sumc) {
                  tbmax = sumb;
                  tbhi = suma;
                  tblo = sumc;
                  if (tblo > mPulseHeightThreshold) {
                    int phVal = 0;
                    for (int tb = 0; tb < 30; tb++) {
                      phVal = (mid->getADC()[tb] + before->getADC()[tb] + after->getADC()[tb]);
                      // TODO do we have a corresponding tracklet?
                      mPulseHeight->Fill(tb, phVal);
                      mTotalPulseHeight2D->Fill(tb, phVal);
                      mPulseHeight2DperSM[sector]->Fill(tb, phVal);
                      mPulseHeightpro->Fill(tb, phVal);
                      mPulseHeight2DperSM[sector]->Fill(tb, phVal);
                      mPulseHeightperchamber->Fill(tb, det1, phVal);
                      // mPulseHeightPerChamber_1D[det1]->Fill(tb, phVal);
                      if (mNoiseMap != nullptr && !mNoiseMap->getIsNoisy(mid->getHCId(), mid->getROB(), mid->getMCM()))
                        mPulseHeightn->Fill(tb, phVal);
                    }
                  }
                } else {
                  tbmax = sumb;
                  tblo = suma;
                  tbhi = sumc;
                  if (tblo > mPulseHeightThreshold) {
                    int phVal = 0;
                    for (int tb = 0; tb < 30; tb++) {
                      phVal = (mid->getADC()[tb] + before->getADC()[tb] + after->getADC()[tb]);
                      mPulseHeight->Fill(tb, phVal);
                      mTotalPulseHeight2D->Fill(tb, phVal);
                      mPulseHeight2DperSM[sector]->Fill(tb, phVal);
                      mPulseHeightpro->Fill(tb, phVal);
                      mPulseHeight2DperSM[sector]->Fill(tb, phVal);
                      mPulseHeightperchamber->Fill(tb, det1, phVal);
                      // mPulseHeightPerChamber_1D[det1]->Fill(tb, phVal);
                      if (mNoiseMap != nullptr && !mNoiseMap->getIsNoisy(mid->getHCId(), mid->getROB(), mid->getMCM()))
                        mPulseHeightn->Fill(tb, phVal);
                    }
                  }
                }
              } // end else
            }   // end if 3 pads next to each other
          }     // end for c
        }
      } // end for r
      mEventswDigitsPerTimeFrame->Fill(DigitTrig);
    } // end for d
  }   // for loop over digits
} // loop over triggers

void DigitsTask::endOfCycle()
{
  ILOG(Debug, Devel) << "endOfCycle" << ENDM;
  for (Int_t det = 0; det < 540; det++) {
    mClsAmpCh->Fill(mClusterAmplitudeChamber->Integral(1, -1, det, det + 1)); // prof->GetBinContent(det));
  }
  double scale = mPulseHeight->GetEntries() / 30;
  for (int i = 0; i < 30; ++i)
    mPulseHeightScaled->SetBinContent(i, mPulseHeight->GetBinContent(i));
  mPulseHeightScaled->Scale(1 / scale);

  mClsChargeTbCycle.get()->Reset();
}

void DigitsTask::endOfActivity(const Activity& /*activity*/)
{
  ILOG(Debug, Devel) << "endOfActivity" << ENDM;
}

void DigitsTask::reset()
{
  // clean all the monitor objects here
  ILOG(Debug, Devel) << "Resetting the histogram" << ENDM;
  mDigitsPerEvent->Reset();
  mEventswDigitsPerTimeFrame->Reset();
  mDigitHCID->Reset();
  mTotalPulseHeight2D->Reset();
  mPulseHeight->Reset();

  mDigitsPerEvent.get()->Reset();
  mDigitHCID.get()->Reset();
  mClusterAmplitudeChamber.get()->Reset();
  for (auto& h : mNClsLayer) {
    h.get()->Reset();
  }
  mADCvalue.get()->Reset();
  for (auto& h : mADC) {
    h.get()->Reset();
  }
  for (auto& h : mADCTB) {
    h.get()->Reset();
  }
  for (auto& h : mADCTBfull) {
    h.get()->Reset();
  }
  mNCls.get()->Reset();
  for (auto& h : mHCMCM) {
    h.get()->Reset();
  }
  for (auto& h : mClsSM) {
    h.get()->Reset();
  }
  mClsTb.get()->Reset();
  mClsChargeFirst.get()->Reset();
  mClsChargeTb.get()->Reset();
  mClsChargeTbCycle.get()->Reset();
  mClsNTb.get()->Reset();
  mClsAmp.get()->Reset();
  mNClsAmp.get()->Reset();
  mClsAmpDrift.get()->Reset();
  mClsAmpTb.get()->Reset();
  mClsAmpCh.get()->Reset();
  for (auto& h : mClsDetAmp) {
    h.get()->Reset();
  }
  mClsSector.get()->Reset();
  mClsStack.get()->Reset();
  mClsChargeTbTigg.get()->Reset();
  for (auto& h : mClsTbSM) {
    h.get()->Reset();
  }
  mPulseHeight.get()->Reset();
  mPulseHeightScaled.get()->Reset();
  mTotalPulseHeight2D.get()->Reset();
  mPulseHeightn.get()->Reset();
  mPulseHeightpro.get()->Reset();
  mPulseHeightperchamber.get()->Reset();
  //  for (auto& h : mPulseHeightPerChamber_1D) {
  // j    h->Reset();
  //  }; // ph2DSM;
}
} // namespace o2::quality_control_modules::trd
