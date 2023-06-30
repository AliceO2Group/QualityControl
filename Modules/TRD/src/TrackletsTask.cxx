// Copyright 2019-2020 CERN and copyright holders of ALICE O2.
// See https://alice-o2.web.cern.ch/copyright for details of the copyright holders.
// All rights not expressly granted are reserved.
//
// This software is distributed under the terms of the GNU General Public
// License v3 (GPL Version 3), copied verbatim in the file "COPYING".
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.

///
/// \file   TrackletsTask.cxx
/// \author My Name
///

#include <TCanvas.h>
#include <TH1.h>
#include <TH2.h>
#include <THashList.h>
#include <TLine.h>
#include <sstream>
#include <string>

#include "QualityControl/QcInfoLogger.h"
#include "TRD/TrackletsTask.h"
#include "TRDQC/StatusHelper.h"
#include <Framework/InputRecord.h>
#include <Framework/InputRecordWalker.h>
#include "DataFormatsTRD/Tracklet64.h"
#include "DataFormatsTRD/HelperMethods.h"
#include "DataFormatsTRD/Digit.h"
#include "DataFormatsTRD/Digit.h"
#include "DataFormatsTRD/NoiseCalibration.h"
#include "DataFormatsTRD/TriggerRecord.h"
#include "CCDB/BasicCCDBManager.h"

namespace o2::quality_control_modules::trd
{

TrackletsTask::~TrackletsTask()
{
}

void TrackletsTask::drawLinesMCM(TH2F* histo)
{

  TLine* l;
  Int_t nPos[o2::trd::constants::NSTACK - 1] = { 16, 32, 44, 60 };

  for (Int_t iStack = 0; iStack < o2::trd::constants::NSTACK - 1; ++iStack) {
    l = new TLine(nPos[iStack] - 0.5, -0.5, nPos[iStack] - 0.5, 47.5);
    l->SetLineStyle(2);
    histo->GetListOfFunctions()->Add(l);
  }

  for (Int_t iLayer = 0; iLayer < o2::trd::constants::NLAYER; ++iLayer) {
    l = new TLine(-0.5, iLayer * 8 - 0.5, 75.5, iLayer * 8 - 0.5);
    l = new TLine(0.5, iLayer * 8 - 0.5, 75.5, iLayer * 8 - 0.5);
    l->SetLineStyle(2);
    histo->GetListOfFunctions()->Add(l);
  }
}

void TrackletsTask::drawTrdLayersGrid(TH2F* hist)
{
  TLine* line;
  for (int i = 0; i < 5; ++i) {
    switch (i) {
      case 0:
        line = new TLine(15.5, 0, 15.5, 143.5);
        hist->GetListOfFunctions()->Add(line);
        line->SetLineStyle(kDashed);
        line->SetLineColor(kBlack);
        break;
      case 1:
        line = new TLine(31.5, 0, 31.5, 143.5);
        hist->GetListOfFunctions()->Add(line);
        line->SetLineStyle(kDashed);
        line->SetLineColor(kBlack);
        break;
      case 2:
        line = new TLine(43.5, 0, 43.5, 143.5);
        hist->GetListOfFunctions()->Add(line);
        line->SetLineStyle(kDashed);
        line->SetLineColor(kBlack);
        break;
      case 3:
        line = new TLine(59.5, 0, 59.5, 143.5);
        hist->GetListOfFunctions()->Add(line);
        line->SetLineStyle(kDashed);
        line->SetLineColor(kBlack);
        break;
    }
  }
  for (int iSec = 1; iSec < 18; ++iSec) {
    float yPos = iSec * 8 - 0.5;
    line = new TLine(-0.5, yPos, 75.5, yPos);
    line->SetLineStyle(kDashed);
    line->SetLineColor(kBlack);
    hist->GetListOfFunctions()->Add(line);
  }
}

void TrackletsTask::retrieveCCDBSettings()
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
  // j  else{
  //    drawHashedOnHistsPerLayer();
  //   }
}

void TrackletsTask::buildHistograms()
{
  for (Int_t sm = 0; sm < o2::trd::constants::NSECTOR; ++sm) {
    std::string label = fmt::format("TrackletHCMCM_{0}", sm);
    std::string title = fmt::format("MCM in Tracklets data stream for sector {0};mcm in rob in layer;ROB in stack", sm);
    moHCMCM[sm] = new TH2F(label.c_str(), title.c_str(), 76, -0.5, 75.5, 8 * 5, -0.5, 8 * 5 - 0.5);
    getObjectsManager()->startPublishing(moHCMCM[sm]);
    getObjectsManager()->setDefaultDrawOptions(moHCMCM[sm]->GetName(), "COLZ");
    drawLinesMCM(moHCMCM[sm]);
  }
  mTrackletSlope = new TH1F("trackletslope", "uncalibrated Slope of tracklets;Slope;Counts", 1024, -6.0, 6.0); // slope is 8 bits in the tracklet
  getObjectsManager()->startPublishing(mTrackletSlope);
  mTrackletSlopeRaw = new TH1F("trackletsloperaw", "Raw Slope of tracklets;Slope;Counts", 256, 0, 256); // slope is 8 bits in the tracklet
  getObjectsManager()->startPublishing(mTrackletSlopeRaw);
  mTrackletHCID = new TH1F("tracklethcid", "Tracklet distribution over Halfchambers;HalfChamber ID; Counts", 1080, 0, 1080);
  getObjectsManager()->startPublishing(mTrackletHCID);
  mTrackletPosition = new TH1F("trackletpos", "Uncalibrated Position of Tracklets;Position;Counts", 1400, -70, 70);
  getObjectsManager()->startPublishing(mTrackletPosition);
  mTrackletPositionRaw = new TH1F("trackletposraw", "Raw Position of Tracklets;Position;Counts", 2048, 0, 2048);
  getObjectsManager()->startPublishing(mTrackletPositionRaw);
  mTrackletsPerEvent = new TH1F("trackletsperevent", "Number of Tracklets per event;Tracklets in Event;Counts", 25000, 0, 25000);
  getObjectsManager()->startPublishing(mTrackletsPerEvent);
  mTrackletsPerHC2D = new TH2F("trackletsperHC2D", "Tracklets distribution in half-chambers;Sector_Side;Stack_Side", 36, 0, 36, 30, 0, 30);
  mTrackletsPerHC2D->SetStats(0);
  mTrackletsPerHC2D->GetXaxis()->SetTitle("Sector_Side");
  mTrackletsPerHC2D->GetXaxis()->CenterTitle(kTRUE);
  mTrackletsPerHC2D->GetYaxis()->SetTitle("Stack_Layer");
  mTrackletsPerHC2D->GetYaxis()->CenterTitle(kTRUE);
  for (int s = 0; s < o2::trd::constants::NSTACK; ++s) {
    for (int l = 0; l < o2::trd::constants::NLAYER; ++l) {
      std::string label = fmt::format("{0}_{1}", s, l);
      int pos = s * o2::trd::constants::NLAYER + l + 1;
      mTrackletsPerHC2D->GetYaxis()->SetBinLabel(pos, label.c_str());
    }
  }
  for (int sm = 0; sm < o2::trd::constants::NSECTOR; ++sm) {
    for (int side = 0; side < 2; ++side) {
      std::string label = fmt::format("{0}_{1}", sm, side == 0 ? "A" : "B");
      int pos = sm * 2 + side + 1;
      mTrackletsPerHC2D->GetXaxis()->SetBinLabel(pos, label.c_str());
    }
  }

  // Mask known inactive halfchambers in the active chamber map
  TLine* line[6];
  std::pair<int, int> x, y;
  for (int iHC = 0; iHC < o2::trd::constants::NCHAMBER * 2; ++iHC) {
    if (mChamberStatus != nullptr) {
      if (mChamberStatus->isMasked(iHC)) {
        int stackLayer = o2::trd::HelperMethods::getStack(iHC / 2) * o2::trd::constants::NLAYER + o2::trd::HelperMethods::getLayer(iHC / 2);
        int sectorSide = (iHC / o2::trd::constants::NHCPERSEC) * 2 + (iHC % 2);
        x.first = sectorSide;
        x.second = sectorSide + 1;
        y.first = stackLayer;
        y.second = stackLayer + 1;

        line[0] = new TLine(x.first, y.first, x.second, y.second);
        line[1] = new TLine(x.second, y.first, x.first, y.second);
        line[2] = new TLine(x.first, y.first, x.second, y.first);
        line[3] = new TLine(x.first, y.second, x.second, y.second);
        line[4] = new TLine(x.first, y.first, x.first, y.second);
        line[5] = new TLine(x.second, y.first, x.second, y.second);
        for (int i = 0; i < 6; ++i) {
          line[i]->SetLineColor(kBlack);
          mTrackletsPerHC2D->GetListOfFunctions()->Add(line[i]);
        }
      }
    }
  }
  getObjectsManager()->startPublishing(mTrackletsPerHC2D);
  getObjectsManager()->setDefaultDrawOptions("trackletsperHC2D", "COLZ");
  getObjectsManager()->setDisplayHint(mTrackletsPerHC2D->GetName(), "logz");

  for (Int_t sm = 0; sm < o2::trd::constants::NSECTOR; ++sm) {
    std::string label = fmt::format("TrackletHCMCMnoise_{0}", sm);
    std::string title = fmt::format("MCM in Tracklets data stream for sector {0} noise in;mcm in rob in layer;ROB in stack", sm);
    moHCMCMn[sm] = new TH2F(label.c_str(), title.c_str(), 76, -0.5, 75.5, 8 * 5, -0.5, 8 * 5 - 0.5);
    getObjectsManager()->startPublishing(moHCMCMn[sm]);
    getObjectsManager()->setDefaultDrawOptions(moHCMCMn[sm]->GetName(), "COLZ");
    drawLinesMCM(moHCMCM[sm]);

    label = fmt::format("TrackletCharge0perSector_{0}", sm);
    title = fmt::format("Tracklet Charge 0 for sector {0}; charge;counts", sm);
    mTrackletQ0perSector[sm] = new TH1F(label.c_str(), title.c_str(), 256, -0.5, 255.5);
    getObjectsManager()->startPublishing(mTrackletQ0perSector[sm]);
    getObjectsManager()->setDefaultDrawOptions(mTrackletQ0perSector[sm]->GetName(), "logy");
    label = fmt::format("TrackletCharge1perSector_{0}", sm);
    title = fmt::format("Tracklet Charge 1 for sector {0}; charge;counts", sm);
    mTrackletQ1perSector[sm] = new TH1F(label.c_str(), title.c_str(), 256, -0.5, 255.5);
    getObjectsManager()->startPublishing(mTrackletQ1perSector[sm]);
    getObjectsManager()->setDefaultDrawOptions(mTrackletQ1perSector[sm]->GetName(), "logy");
    label = fmt::format("TrackletCharge2perSector_{0}", sm);
    title = fmt::format("Tracklet Charge 2 for sector {0}; charge;counts", sm);
    mTrackletQ2perSector[sm] = new TH1F(label.c_str(), title.c_str(), 256, -0.5, 255.5);
    getObjectsManager()->startPublishing(mTrackletQ2perSector[sm]);
    getObjectsManager()->setDefaultDrawOptions(mTrackletQ2perSector[sm]->GetName(), "logy");
  }

  for (int chargewindow = 0; chargewindow < 3; ++chargewindow) {
    std::string label = fmt::format("TrackletCharge{0}", chargewindow);
    std::string title = fmt::format("Tracklet Charge{0}; charge;counts", chargewindow);
    mTrackletQ[chargewindow] = new TH1F(label.c_str(), title.c_str(), 256, -0.5, 255.5);
    getObjectsManager()->startPublishing(mTrackletQ[chargewindow]);
    getObjectsManager()->setDefaultDrawOptions(mTrackletQ[chargewindow]->GetName(), "logy");
  }

  mTrackletSlopen = new TH1F("trackletslopenoise", "uncalibrated Slope of tracklets noise in;Position;Counts", 1024, -6.0, 6.0); // slope is 8 bits in the tracklet
  getObjectsManager()->startPublishing(mTrackletSlopen);
  mTrackletSlopeRawn = new TH1F("trackletsloperawnoise", "Raw Slope of tracklets noise in;Slope;Counts", 256, 0, 256); // slope is 8 bits in the tracklet
  getObjectsManager()->startPublishing(mTrackletSlopeRawn);
  mTrackletHCIDn = new TH1F("tracklethcidnoise", "Tracklet distribution over Halfchambers noise in;HalfChamber ID; Counts", 1080, 0, 1080);
  getObjectsManager()->startPublishing(mTrackletHCIDn);
  mTrackletPositionn = new TH1F("trackletposnoise", "Uncalibrated Position of Tracklets noise in;Position;Counts", 1400, -70, 70);
  getObjectsManager()->startPublishing(mTrackletPositionn);
  mTrackletPositionRawn = new TH1F("trackletposrawnoise", "Raw Position of Tracklets noise in;Position;Counts", 2048, 0, 2048);
  getObjectsManager()->startPublishing(mTrackletPositionRawn);
  mTrackletsPerEventn = new TH1F("trackletspereventn", "Number of Tracklets per event noise in;Tracklets in Events;Counts", 25000, 0, 25000);
  getObjectsManager()->startPublishing(mTrackletsPerEventn);
  mTrackletsPerTimeFrame = new TH1F("trackletspertimeframe", "Number of Tracklets per timeframe;Tracklets in TimeFrame;Counts", 25000, 0, 500000);
  getObjectsManager()->startPublishing(mTrackletsPerTimeFrame);
  mTrackletsPerTimeFrameCycled = new TH1F("trackletspertimeframecycled", "Number of Tracklets per timeframe, this cycle;Tracklets in TimeFrame;Counts", 25000, 0, 500000);
  getObjectsManager()->startPublishing(mTrackletsPerTimeFrameCycled);
  mTriggersPerTimeFrame = new TH1F("triggerspertimeframe", "Number of Triggers per timeframe;Triggers in TimeFrame;Counts", 10000, 0, 10000);
  getObjectsManager()->startPublishing(mTriggersPerTimeFrame);

  buildTrackletLayers();
}

void TrackletsTask::drawHashOnLayers(int layer, int hcid, int rowstart, int rowend)
{
  // instead of using overlays, draw a simple box in red with a cross on it.

  std::pair<float, float> topright, bottomleft; // coordinates of box
  TLine* boxlines[9];
  int det = hcid / 2;
  int side = hcid % 2;
  int sec = hcid / 60;
  bottomleft.first = rowstart - 0.5;
  bottomleft.second = (sec * 2 + side) * 4 - 0.5;
  topright.first = rowend - 0.5;
  topright.second = (sec * 2 + side) * 4 + 4 - 0.5;

  // LOG(info) << "Box for layer : " << layer << " hcid : " << hcid << ": " << bottomleft.first << ":" << bottomleft.second << " -- " << topright.first << ":" << topright.second;
  boxlines[0] = new TLine(bottomleft.first, bottomleft.second, topright.first, bottomleft.second);                                                                                         // bottom
  boxlines[1] = new TLine(bottomleft.first, topright.second, topright.first, topright.second);                                                                                             // top
  boxlines[2] = new TLine(bottomleft.first, bottomleft.second, bottomleft.first, topright.second);                                                                                         // left
  boxlines[3] = new TLine(topright.first, bottomleft.second, topright.first, topright.second);                                                                                             // right
  boxlines[4] = new TLine(bottomleft.first, topright.second - (topright.second - bottomleft.second) / 2, topright.first, topright.second - (topright.second - bottomleft.second) / 2);     // horizontal middle
  boxlines[5] = new TLine(topright.first, bottomleft.second, bottomleft.first, topright.second);                                                                                           // backslash
  boxlines[6] = new TLine(bottomleft.first, bottomleft.second, topright.first, topright.second);                                                                                           // forwardslash
  boxlines[7] = new TLine(bottomleft.first + (topright.first - bottomleft.first) / 2, bottomleft.second, bottomleft.first + (topright.first - bottomleft.first) / 2, topright.second);     // vertical middle
  boxlines[8] = new TLine(bottomleft.first, bottomleft.second + (topright.second - bottomleft.second) / 2, topright.first, bottomleft.second + (topright.second - bottomleft.second) / 2); // bottom
  for (int line = 0; line < 9; ++line) {
    boxlines[line]->SetLineColor(kBlack);
    mLayers[layer]->GetListOfFunctions()->Add(boxlines[line]);
  }
}

void TrackletsTask::buildTrackletLayers()
{
  for (int iLayer = 0; iLayer < 6; ++iLayer) {
    mLayers[iLayer] = new TH2F(Form("TrackletsPerLayer/layer%i", iLayer), Form("Tracklet count per mcm in layer %i;Stack;Sector", iLayer), 76, -0.5, 75.5, 144, -0.5, 143.5);
    auto xax = mLayers[iLayer]->GetXaxis();
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
        yax->SetBinLabel(iSec * 8 + 4, lbl.c_str());
      }
      yax->SetTicks("");
      yax->SetTickSize(0.0);
      yax->SetLabelSize(0.045);
      yax->SetLabelOffset(0.001);
      yax->SetTitleOffset(0.40);
      yax->CenterTitle(true);
    }

    mLayers[iLayer]->SetStats(0);

    drawTrdLayersGrid(mLayers[iLayer]);
    drawHashedOnHistsPerLayer(iLayer); // drawHashOnLayers(iLayer,1);

    getObjectsManager()->startPublishing(mLayers[iLayer]);
    getObjectsManager()->setDefaultDrawOptions(mLayers[iLayer]->GetName(), "COLZ");
    getObjectsManager()->setDisplayHint(mLayers[iLayer], "logz");
    // check axises :

    /*    std::cout << "Test Tracklet Xlabels for layer : " << iLayer << std::endl;
        auto binsizex=xax->GetNbins();
        auto labelsx=xax->GetLabels();
        auto labelssizex=labelsx->GetSize();
        std::cout << "binsize:" << binsizex << "  labelsize:" << labelssizex << std::endl;
        if(binsizex!= labelssizex){
          std::cout << "binsize != labsize ?= " << binsizex << "!=" << labelssizex << std::endl;
        }
        std::cout << "Test Tracklet Ylabels for layer : " << iLayer << std::endl;
        auto binsizey=yax->GetNbins();
        auto labelsy=yax->GetLabels();
        auto labelssizey=labelsy->GetSize();
        std::cout << "binsize:" << binsizey << "  labelsize:" << labelssizey << std::endl;
        if(binsizey!= labelssizey){
          std::cout << "binsize != labsize ?= " << binsizey << "!=" << labelssizey << std::endl;
        }  */
  }
}

void TrackletsTask::drawHashedOnHistsPerLayer(int iLayer)
{
  std::bitset<1080> hciddone;
  hciddone.reset();
  if (mChamberStatus != nullptr) {
    for (int iSec = 0; iSec < 18; ++iSec) {
      for (int iStack = 0; iStack < 5; ++iStack) {
        int rowMax = (iStack == 2) ? 12 : 16;
        for (int side = 0; side < 2; ++side) {
          int det = iSec * 30 + iStack * 6 + iLayer;
          int hcid = (side == 0) ? det * 2 : det * 2 + 1;
          int rowstart = iStack < 3 ? iStack * 16 : 44 + (iStack - 3) * 16;                 // pad row within whole sector
          int rowend = iStack < 3 ? rowMax + iStack * 16 : rowMax + 44 + (iStack - 3) * 16; // pad row within whole sector
          if (mChamberStatus->isMasked(hcid) && (!hciddone.test(hcid))) {
            drawHashOnLayers(iLayer, hcid, rowstart, rowend);
            hciddone.set(hcid);
          }
        }
      }
    }
  } else {
    ILOG(Info, Support) << " Failed to retrieve ChamberStatus, so it will be blank" << ENDM;
  }
}

void TrackletsTask::initialize(o2::framework::InitContext& /*ctx*/)
{
  ILOG(Debug, Devel) << "initialize TrackletsTask" << ENDM;
  if (auto param = mCustomParameters.find("markerstyle"); param != mCustomParameters.end()) {
    mMarkerStyle = stof(param->second);
    ILOG(Debug, Support) << "configure() : using marketstyle : = " << mMarkerStyle << ENDM;
  } else {
    mMarkerStyle = 3; // a plus sign
    ILOG(Debug, Support) << "configure() : using default dritfregionstart = " << mMarkerStyle << ENDM;
  }
  if (auto param = mCustomParameters.find("markersize"); param != mCustomParameters.end()) {
    mMarkerSize = stof(param->second);
    ILOG(Debug, Support) << "configure() : using markersize : = " << mMarkerSize << ENDM;
  } else {
    mMarkerSize = 3; // a plus sign
    ILOG(Debug, Support) << "configure() : using default markersize = " << mMarkerSize << ENDM;
  }
  if (auto param = mCustomParameters.find("ignorelayerlabels"); param != mCustomParameters.end()) {
    mLayerLabelsIgnore = stoi(param->second);
    ILOG(Debug, Support) << "configure() : ignoring labels on layer plots = " << mLayerLabelsIgnore << ENDM;
  }

  retrieveCCDBSettings();
  buildHistograms();
}

void TrackletsTask::startOfActivity(const Activity& activity)
{
  ILOG(Debug, Devel) << "startOfActivity " << activity.mId << ENDM;
  for (Int_t sm = 0; sm < o2::trd::constants::NSECTOR; ++sm) {
    moHCMCM[sm]->Reset();
  }
}

void TrackletsTask::startOfCycle()
{
  ILOG(Debug, Devel) << "startOfCycle" << ENDM;
}

void TrackletsTask::monitorData(o2::framework::ProcessingContext& ctx)
{
  for (auto&& input : ctx.inputs()) {
    if (input.header != nullptr && input.payload != nullptr) {

      auto digits = ctx.inputs().get<gsl::span<o2::trd::Digit>>("digits");
      auto tracklets = ctx.inputs().get<gsl::span<o2::trd::Tracklet64>>("tracklets");
      auto triggerrecords = ctx.inputs().get<gsl::span<o2::trd::TriggerRecord>>("triggers");
      // std::cout << "Tracklets per time frame: " << tracklets.size();
      mTrackletsPerTimeFrame->Fill(tracklets.size());
      mTrackletsPerTimeFrameCycled->Fill(tracklets.size());
      mTriggersPerTimeFrame->Fill(triggerrecords.size());
      for (auto& trigger : triggerrecords) {
        mTrackletsPerEvent->Fill(trigger.getNumberOfTracklets());
        if (trigger.getNumberOfTracklets() == 0) {
          continue; // bail if we have no digits in this trigger
        }
        // now sort digits to det,row,pad
        for (int currenttracklet = trigger.getFirstTracklet(); currenttracklet < trigger.getFirstTracklet() + trigger.getNumberOfTracklets() - 1; ++currenttracklet) {
          int detector = tracklets[currenttracklet].getDetector();
          int sm = detector / 30;
          int detLoc = detector % 30;
          int layer = detector % 6;
          int istack = detLoc / 6;
          int iChamber = sm * 30 + istack * o2::trd::constants::NLAYER + layer;
          int stackoffset = istack * o2::trd::constants::NSTACK * o2::trd::constants::NROBC1;
          if (istack >= 2) {
            stackoffset -= 2; // only 12in stack 2
          }
          int hcid = tracklets[currenttracklet].getHCID();
          int stackLayer = o2::trd::HelperMethods::getStack(hcid / 2) * o2::trd::constants::NLAYER + o2::trd::HelperMethods::getLayer(hcid / 2);
          int sectorSide = (hcid / o2::trd::constants::NHCPERSEC) * 2 + (hcid % 2);
          mTrackletsPerHC2D->Fill(sectorSide, stackLayer);
          // 8 rob x 16 mcm each per chamber
          //  5 stack(y), 6 layers(x)
          //  y=stack_rob, x=layer_mcm
          int x = o2::trd::constants::NMCMROB * layer + tracklets[currenttracklet].getMCM();
          int y = o2::trd::constants::NROBC1 * istack + tracklets[currenttracklet].getROB();
          if (mNoiseMap != nullptr && mNoiseMap->isTrackletFromNoisyMCM(tracklets[currenttracklet])) {
            moHCMCMn[sm]->Fill(x, y);
            mTrackletSlopen->Fill(tracklets[currenttracklet].getUncalibratedDy());
            mTrackletSlopeRawn->Fill(tracklets[currenttracklet].getSlope());
            mTrackletPositionn->Fill(tracklets[currenttracklet].getUncalibratedY());
            mTrackletPositionRawn->Fill(tracklets[currenttracklet].getPosition());
            mTrackletHCIDn->Fill(tracklets[currenttracklet].getHCID());
          } else {
            moHCMCM[sm]->Fill(x, y);
            mTrackletSlope->Fill(tracklets[currenttracklet].getUncalibratedDy());
            mTrackletSlopeRaw->Fill(tracklets[currenttracklet].getSlope());
            mTrackletPosition->Fill(tracklets[currenttracklet].getUncalibratedY());
            mTrackletPositionRaw->Fill(tracklets[currenttracklet].getPosition());
            mTrackletHCID->Fill(tracklets[currenttracklet].getHCID());
            mTrackletQ[0]->Fill(tracklets[currenttracklet].getQ0());
            mTrackletQ[1]->Fill(tracklets[currenttracklet].getQ1());
            mTrackletQ[2]->Fill(tracklets[currenttracklet].getQ2());
            mTrackletQ0perSector[sm]->Fill(tracklets[currenttracklet].getQ0());
            mTrackletQ1perSector[sm]->Fill(tracklets[currenttracklet].getQ1());
            mTrackletQ2perSector[sm]->Fill(tracklets[currenttracklet].getQ2());
          }
          int side = tracklets[currenttracklet].getHCID() % 2; // 0: A-side, 1: B-side
          int stack = (detector % 30) / 6;
          int sec = detector / 30;
          int rowGlb = stack < 3 ? tracklets[currenttracklet].getPadRow() + stack * 16 : tracklets[currenttracklet].getPadRow() + 44 + (stack - 3) * 16; // pad row within whole sector
          int colGlb = tracklets[currenttracklet].getColumn() + sec * 8 + side * 4;
          mLayers[layer]->Fill(rowGlb, colGlb);
        }
      }
    }
  }
}

void TrackletsTask::endOfCycle()
{
  ILOG(Debug, Devel) << "endOfCycle" << ENDM;
  // scale 2d mHCMCM plots so they all have the same max height.
  int max = 0;
  for (auto& hist : moHCMCM) {
    if (hist->GetMaximum() > max) {
      max = hist->GetMaximum();
    }
  }
  for (auto& hist : moHCMCM) {
    hist->SetMaximum(max);
  }
  // reset the TrackletPerTimeCycled
  mTrackletsPerTimeFrameCycled->Reset();
}

void TrackletsTask::endOfActivity(const Activity& /*activity*/)
{
  ILOG(Debug, Devel) << "endOfActivity" << ENDM;
}

void TrackletsTask::reset()
{
  // clean all the monitor objects here

  ILOG(Debug, Devel) << "Resetting the histograms" << ENDM;
  for (auto h : moHCMCM) {
    h->Reset();
  }
  mTrackletSlope->Reset();
  mTrackletSlopeRaw->Reset();
  mTrackletHCID->Reset();
  mTrackletPosition->Reset();
  mTrackletPositionRaw->Reset();
  mTrackletsPerEvent->Reset();
  mTrackletsPerHC2D->Reset();
  for (auto h : moHCMCMn) {
    h->Reset();
  }
  mTrackletSlopen->Reset();
  mTrackletSlopeRawn->Reset();
  mTrackletHCIDn->Reset();
  mTrackletPositionn->Reset();
  mTrackletPositionRawn->Reset();
  mTrackletsPerEventn->Reset();
  mTrackletsPerTimeFrame->Reset();
  mTrackletsPerTimeFrameCycled->Reset();
  for (auto h : mLayers) {
    h->Reset();
  }
  for (auto h : mTrackletQ0perSector) {
    h->Reset();
  }
  for (auto h : mTrackletQ1perSector) {
    h->Reset();
  }
  for (auto h : mTrackletQ2perSector) {
    h->Reset();
  }
  for (auto h : mTrackletQ) {
    h->Reset();
  }
}

} // namespace o2::quality_control_modules::trd
