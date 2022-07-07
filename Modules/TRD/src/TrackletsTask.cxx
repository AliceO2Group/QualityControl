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
#include <TLine.h>
#include <sstream>
#include <string>

#include "QualityControl/QcInfoLogger.h"
#include "TRD/TrackletsTask.h"
#include "TRDQC/StatusHelper.h"
#include <Framework/InputRecord.h>
#include <Framework/InputRecordWalker.h>
#include "DataFormatsTRD/Tracklet64.h"
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
        line = new TLine(15.5, 0, 15.5, 75.5);
        hist->GetListOfFunctions()->Add(line);
        line->SetLineStyle(kDashed);
        line->SetLineColor(kBlack);
        break;
      case 1:
        line = new TLine(31.5, 0, 31.5, 75.5);
        hist->GetListOfFunctions()->Add(line);
        line->SetLineStyle(kDashed);
        line->SetLineColor(kBlack);
        break;
      case 2:
        line = new TLine(43.5, 0, 43.5, 75.5);
        hist->GetListOfFunctions()->Add(line);
        line->SetLineStyle(kDashed);
        line->SetLineColor(kBlack);
        break;
      case 3:
        line = new TLine(59.5, 0, 59.5, 75.5);
        hist->GetListOfFunctions()->Add(line);
        line->SetLineStyle(kDashed);
        line->SetLineColor(kBlack);
        break;
    }
  }
  for (int iSec = 1; iSec < 18; ++iSec) {
    float yPos = iSec * 8 - 0.5;
    line = new TLine(-0.5, yPos, 143.5, yPos);
    line->SetLineStyle(kDashed);
    line->SetLineColor(kBlack);
    hist->GetListOfFunctions()->Add(line);
  }
}

void TrackletsTask::retrieveCCDBSettings()
{
  if (auto param = mCustomParameters.find("ccdbtimestamp"); param != mCustomParameters.end()) {
    mTimestamp = std::stol(mCustomParameters["ccdbtimestamp"]);
    ILOG(Info, Support) << "configure() : using ccdbtimestamp = " << mTimestamp << ENDM;
  } else {
    mTimestamp = o2::ccdb::getCurrentTimestamp();
    ILOG(Info, Support) << "configure() : using default timestam of now = " << mTimestamp << ENDM;
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
  //j  else{
  //   drawHashedOnHistsPerLayer();
  //  }
}

void TrackletsTask::buildHistograms()
{
  for (Int_t sm = 0; sm < o2::trd::constants::NSECTOR; ++sm) {
    std::string label = fmt::format("TrackletHCMCM_{0}", sm);
    std::string title = fmt::format("MCM in Tracklets data stream for sector {0};mcm in rob in layer;ROB in stack", sm);
    moHCMCM[sm].reset(new TH2F(label.c_str(), title.c_str(), 76, -0.5, 75.5, 8 * 5, -0.5, 8 * 5 - 0.5));
    getObjectsManager()->startPublishing(moHCMCM[sm].get());
    getObjectsManager()->setDefaultDrawOptions(moHCMCM[sm]->GetName(), "COLZ");
    drawLinesMCM(moHCMCM[sm].get());
  }
  mTrackletSlope.reset(new TH1F("trackletslope", "uncalibrated Slope of tracklets;Slope;Counts", 1024, -6.0, 6.0)); // slope is 8 bits in the tracklet
  getObjectsManager()->startPublishing(mTrackletSlope.get());
  mTrackletSlopeRaw.reset(new TH1F("trackletsloperaw", "Raw Slope of tracklets;Slope;Counts", 256, 0, 256)); // slope is 8 bits in the tracklet
  getObjectsManager()->startPublishing(mTrackletSlopeRaw.get());
  mTrackletHCID.reset(new TH1F("tracklethcid", "Tracklet distribution over Halfchambers;HalfChamber ID; Counts", 1080, 0, 1080));
  getObjectsManager()->startPublishing(mTrackletHCID.get());
  mTrackletPosition.reset(new TH1F("trackletpos", "Uncalibrated Position of Tracklets;Position;Counts", 1400, -70, 70));
  getObjectsManager()->startPublishing(mTrackletPosition.get());
  mTrackletPositionRaw.reset(new TH1F("trackletposraw", "Raw Position of Tracklets;Position;Counts", 2048, 0, 2048));
  getObjectsManager()->startPublishing(mTrackletPositionRaw.get());
  mTrackletsPerEvent.reset(new TH1F("trackletsperevent", "Number of Tracklets per event;Tracklets in Event;Counts", 25000, 0, 25000));
  getObjectsManager()->startPublishing(mTrackletsPerEvent.get());

  for (Int_t sm = 0; sm < o2::trd::constants::NSECTOR; ++sm) {
    std::string label = fmt::format("TrackletHCMCMnoise_{0}", sm);
    std::string title = fmt::format("MCM in Tracklets data stream for sector {0} noise in;mcm in rob in layer;ROB in stack", sm);
    moHCMCMn[sm].reset(new TH2F(label.c_str(), title.c_str(), 76, -0.5, 75.5, 8 * 5, -0.5, 8 * 5 - 0.5));
    getObjectsManager()->startPublishing(moHCMCMn[sm].get());
    getObjectsManager()->setDefaultDrawOptions(moHCMCMn[sm]->GetName(), "COLZ");
    drawLinesMCM(moHCMCM[sm].get());
  }
  mTrackletSlopen.reset(new TH1F("trackletslopenoise", "uncalibrated Slope of tracklets noise in;Position;Counts", 1024, -6.0, 6.0)); // slope is 8 bits in the tracklet
  getObjectsManager()->startPublishing(mTrackletSlopen.get());
  mTrackletSlopeRawn.reset(new TH1F("trackletsloperawnoise", "Raw Slope of tracklets noise in;Slope;Counts", 256, 0, 256)); // slope is 8 bits in the tracklet
  getObjectsManager()->startPublishing(mTrackletSlopeRawn.get());
  mTrackletHCIDn.reset(new TH1F("tracklethcidnoise", "Tracklet distribution over Halfchambers noise in;HalfChamber ID; Counts", 1080, 0, 1080));
  getObjectsManager()->startPublishing(mTrackletHCIDn.get());
  mTrackletPositionn.reset(new TH1F("trackletposnoise", "Uncalibrated Position of Tracklets noise in;Position;Counts", 1400, -70, 70));
  getObjectsManager()->startPublishing(mTrackletPositionn.get());
  mTrackletPositionRawn.reset(new TH1F("trackletposrawnoise", "Raw Position of Tracklets noise in;Position;Counts", 2048, 0, 2048));
  getObjectsManager()->startPublishing(mTrackletPositionRawn.get());
  mTrackletsPerEventn.reset(new TH1F("trackletspereventn", "Number of Tracklets per event noise in;Tracklets in Events;Counts", 25000, 0, 25000));
  getObjectsManager()->startPublishing(mTrackletsPerEventn.get());
  mTrackletsPerTimeFrame.reset(new TH1F("trackletspertimeframe", "Number of Tracklets per timeframe;Tracklets in TimeFrame;Counts", 25000, 0, 25000));
  getObjectsManager()->startPublishing(mTrackletsPerTimeFrame.get());
  mTrackletsPerTimeFrameCycled.reset(new TH1F("trackletspertimeframecycled", "Number of Tracklets per timeframe, this cycle;Tracklets in TimeFrame;Counts", 25000, 0, 25000));
  getObjectsManager()->startPublishing(mTrackletsPerTimeFrameCycled.get());

  buildTrackletLayers();
}

void TrackletsTask::drawHashOnLayers(int layer, int hcid, int rowstart, int rowend)
{
  //instead of using overlays, draw a simple box in red with a cross on it.

  std::pair<float, float> topright, bottomleft; //coordinates of box
  TLine* boxlines[9];
  int det = hcid / 2;
  int side = hcid % 2;
  int sec = hcid / 60;
  bottomleft.first = rowstart; // - 0.5;
  bottomleft.second = (sec * 2 + side) * 4 - 0.5;
  topright.first = rowend - 0.5;
  topright.second = (sec * 2 + side) * 4 + 4 - 0.5;

  boxlines[0] = new TLine(bottomleft.first, bottomleft.second, topright.first, bottomleft.second);                                                                                         //bottom
  boxlines[1] = new TLine(bottomleft.first, topright.second, topright.first, topright.second);                                                                                             // top
  boxlines[2] = new TLine(bottomleft.first, bottomleft.second, bottomleft.first, topright.second);                                                                                         // left
  boxlines[3] = new TLine(topright.first, bottomleft.second, topright.first, topright.second);                                                                                             // right
  boxlines[4] = new TLine(bottomleft.first, topright.second - (topright.second - bottomleft.second) / 2, topright.first, topright.second - (topright.second - bottomleft.second) / 2);     //horizontal middle
  boxlines[5] = new TLine(topright.first, bottomleft.second, bottomleft.first, topright.second);                                                                                           //backslash
  boxlines[6] = new TLine(bottomleft.first, bottomleft.second, topright.first, topright.second);                                                                                           //forwardslash
  boxlines[7] = new TLine(bottomleft.first + (topright.first - bottomleft.first) / 2, bottomleft.second, bottomleft.first + (topright.first - bottomleft.first) / 2, topright.second);     //vertical middle
  boxlines[8] = new TLine(bottomleft.first, bottomleft.second + (topright.second - bottomleft.second) / 2, topright.first, bottomleft.second + (topright.second - bottomleft.second) / 2); //bottom
  for (int line = 0; line < 9; ++line) {
    boxlines[line]->SetLineColor(kRed);
    mLayers[layer]->GetListOfFunctions()->Add(boxlines[line]);
  }
}

void TrackletsTask::buildTrackletLayers()
{
  for (int iLayer = 0; iLayer < 6; ++iLayer) {
    mLayers[iLayer].reset(new TH2F(Form("TrackletsPerLayer/layer%i", iLayer), Form("Tracklet count per mcm in layer %i;stack;sector", iLayer), 76, -10.5, 75.5, 144, -10.5, 143.5));

    auto xax = mLayers[iLayer].get()->GetXaxis();
    xax->SetBinLabel(8, "0");
    xax->SetBinLabel(24, "1");
    xax->SetBinLabel(38, "2");
    xax->SetBinLabel(52, "3");
    xax->SetBinLabel(68, "4");
    xax->SetTicks("-");
    xax->SetTickSize(0.01);
    xax->SetLabelSize(0.045);
    xax->SetLabelOffset(0.01);
    auto yax = mLayers[iLayer].get()->GetYaxis();
    for (int iSec = 0; iSec < 18; ++iSec) {
      auto lbl = std::to_string(iSec);
      yax->SetBinLabel(iSec * 8 + 4, lbl.c_str());
    }
    yax->SetTicks("");
    yax->SetTickSize(0.01);
    yax->SetLabelSize(0.045);
    yax->SetLabelOffset(0.01);
    mLayers[iLayer].get()->SetStats(0);

    drawTrdLayersGrid(mLayers[iLayer].get());
    drawHashedOnHistsPerLayer(iLayer); //drawHashOnLayers(iLayer,1);

    getObjectsManager()->startPublishing(mLayers[iLayer].get());
    getObjectsManager()->setDefaultDrawOptions(mLayers[iLayer]->GetName(), "COLZ");
    getObjectsManager()->setDisplayHint(mLayers[iLayer].get(), "logz");
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
        for (int side = 0; side < 8; ++side) {
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
  ILOG(Info, Support) << "initialize TrackletsTask" << ENDM;
  if (auto param = mCustomParameters.find("markerstyle"); param != mCustomParameters.end()) {
    mMarkerStyle = stof(param->second);
    ILOG(Info, Support) << "configure() : using marketstyle : = " << mMarkerStyle << ENDM;
  } else {
    mMarkerStyle = 3; // a plus sign
    ILOG(Info, Support) << "configure() : using default dritfregionstart = " << mMarkerStyle << ENDM;
  }
  if (auto param = mCustomParameters.find("markersize"); param != mCustomParameters.end()) {
    mMarkerSize = stof(param->second);
    ILOG(Info, Support) << "configure() : using markersize : = " << mMarkerSize << ENDM;
  } else {
    mMarkerSize = 3; // a plus sign
    ILOG(Info, Support) << "configure() : using default markersize = " << mMarkerSize << ENDM;
  }

  retrieveCCDBSettings();
  buildHistograms();
}

void TrackletsTask::startOfActivity(Activity& activity)
{
  ILOG(Info, Support) << "startOfActivity " << activity.mId << ENDM;
  for (Int_t sm = 0; sm < o2::trd::constants::NSECTOR; ++sm) {
    moHCMCM[sm]->Reset();
  }
}

void TrackletsTask::startOfCycle()
{
  ILOG(Info, Support) << "startOfCycle" << ENDM;
}

void TrackletsTask::monitorData(o2::framework::ProcessingContext& ctx)
{
  for (auto&& input : ctx.inputs()) {
    if (input.header != nullptr && input.payload != nullptr) {

      auto digits = ctx.inputs().get<gsl::span<o2::trd::Digit>>("digits");
      auto tracklets = ctx.inputs().get<gsl::span<o2::trd::Tracklet64>>("tracklets");
      auto triggerrecords = ctx.inputs().get<gsl::span<o2::trd::TriggerRecord>>("triggers");
      mTrackletsPerTimeFrame->Fill(tracklets.size());
      mTrackletsPerTimeFrameCycled->Fill(tracklets.size());
      for (auto& trigger : triggerrecords) {
        if (trigger.getNumberOfTracklets() == 0) {
          continue; //bail if we have no digits in this trigger
        }
        //now sort digits to det,row,pad
        mTrackletsPerEvent->Fill(trigger.getNumberOfTracklets());
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
          //8 rob x 16 mcm each per chamber
          // 5 stack(y), 6 layers(x)
          // y=stack_rob, x=layer_mcm
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
  ILOG(Info, Support) << "endOfCycle" << ENDM;
  //scale 2d mHCMCM plots so they all have the same max height.
  int max = 0;
  for (auto& hist : moHCMCM) {
    if (hist->GetMaximum() > max) {
      max = hist->GetMaximum();
    }
  }
  for (auto& hist : moHCMCM) {
    hist->SetMaximum(max);
  }
  //reset the TrackletPerTimeCycled
  mTrackletsPerTimeFrameCycled->Reset();
}

void TrackletsTask::endOfActivity(Activity& /*activity*/)
{
  ILOG(Info, Support) << "endOfActivity" << ENDM;
}

void TrackletsTask::reset()
{
  // clean all the monitor objects here

  ILOG(Info, Support) << "Resetting the histogram" << ENDM;
  for (auto h : moHCMCM) {
    h.get()->Reset();
  }
  mTrackletSlope.get()->Reset();
  mTrackletSlopeRaw.get()->Reset();
  mTrackletHCID.get()->Reset();
  mTrackletPosition.get()->Reset();
  mTrackletPositionRaw.get()->Reset();
  mTrackletsPerEvent.get()->Reset();
  for (auto h : moHCMCMn) {
    h.get()->Reset();
  }
  mTrackletSlopen.get()->Reset();
  mTrackletSlopeRawn.get()->Reset();
  mTrackletHCIDn.get()->Reset();
  mTrackletPositionn.get()->Reset();
  mTrackletPositionRawn.get()->Reset();
  mTrackletsPerEventn.get()->Reset();
  mTrackletsPerTimeFrame->Reset();
  mTrackletsPerTimeFrameCycled->Reset();
  for (auto h : mLayers) {
    h->Reset();
  }
}

} // namespace o2::quality_control_modules::trd
