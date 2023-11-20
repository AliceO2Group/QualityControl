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
///

#include <TCanvas.h>
#include <TH1.h>
#include <TH2.h>
#include <THashList.h>
#include <TLine.h>
#include <TMath.h>
#include <sstream>
#include <string>

#include "QualityControl/QcInfoLogger.h"
#include "Common/Utils.h"
#include "TRD/TrackletsTask.h"
#include "TRDQC/StatusHelper.h"
#include <Framework/InputRecord.h>
#include <Framework/InputRecordWalker.h>
#include "DataFormatsTRD/Tracklet64.h"
#include "DataFormatsTRD/HelperMethods.h"
#include "DataFormatsTRD/Digit.h"
#include "DataFormatsTRD/NoiseCalibration.h"
#include "DataFormatsTRD/TriggerRecord.h"

using namespace o2::quality_control_modules::common;
using namespace o2::trd::constants;
using Helper = o2::trd::HelperMethods;

namespace o2::quality_control_modules::trd
{

void TrackletsTask::drawLinesMCM(TH2F* histo)
{
  TLine* l;
  Int_t nPos[NSTACK - 1] = { 16, 32, 44, 60 };

  for (Int_t iStack = 0; iStack < NSTACK - 1; ++iStack) {
    l = new TLine(nPos[iStack] - 0.5, -0.5, nPos[iStack] - 0.5, 47.5);
    l->SetLineStyle(2);
    histo->GetListOfFunctions()->Add(l);
  }

  for (Int_t iLayer = 0; iLayer < NLAYER; ++iLayer) {
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

void TrackletsTask::buildHistograms()
{
  constexpr int nLogBins = 100;
  float xBins[nLogBins + 1];
  float xBinLogMin = 0.f;
  float xBinLogMax = 8.f;
  float logBinWidth = (xBinLogMax - xBinLogMin) / nLogBins;
  for (int iBin = 0; iBin <= nLogBins; ++iBin) {
    xBins[iBin] = TMath::Power(10, xBinLogMin + iBin * logBinWidth);
  }
  mTrackletSlope = new TH1F("trackletslope", "Tracklet inclination in natural units;pads per time bin;counts", 100, -0.15, 0.15);
  getObjectsManager()->startPublishing(mTrackletSlope);

  mTrackletHCID = new TH1F("tracklethcid", "Tracklet distribution over Halfchambers;HalfChamber ID;counts", 1080, -0.5, 1079.5);
  getObjectsManager()->startPublishing(mTrackletHCID);

  mTrackletPosition = new TH1F("trackletpos", "Tracklet position relative to MCM center;number of pads;counts", 200, -30, 30);
  getObjectsManager()->startPublishing(mTrackletPosition);

  mTrackletsPerEvent = new TH1F("trackletsperevent", "Number of Tracklets per event;Tracklets in Event;Counts", nLogBins, xBins);
  getObjectsManager()->startPublishing(mTrackletsPerEvent);
  getObjectsManager()->setDefaultDrawOptions(mTrackletsPerEvent->GetName(), "logx");

  mTrackletsPerEventPP = new TH1F("trackletspereventPP", "Number of Tracklets per event;Tracklets in Event;Counts", 1000, 0, 5000);
  getObjectsManager()->startPublishing(mTrackletsPerEventPP);

  mTrackletsPerEventPbPb = new TH1F("trackletspereventPbPb", "Number of Tracklets per event;Tracklets in Event;Counts", 1000, 0, 100000);
  getObjectsManager()->startPublishing(mTrackletsPerEventPbPb);

  mTrackletsPerHC2D = new TH2F("trackletsperHC2D", "Tracklets distribution in half-chambers;Sector_Side;Stack_Side", 36, 0, 36, 30, 0, 30);
  mTrackletsPerHC2D->SetStats(0);
  mTrackletsPerHC2D->GetXaxis()->SetTitle("Sector_Side");
  mTrackletsPerHC2D->GetXaxis()->CenterTitle(kTRUE);
  mTrackletsPerHC2D->GetYaxis()->SetTitle("Stack_Layer");
  mTrackletsPerHC2D->GetYaxis()->CenterTitle(kTRUE);
  for (int s = 0; s < NSTACK; ++s) {
    for (int l = 0; l < NLAYER; ++l) {
      std::string label = fmt::format("{0}_{1}", s, l);
      int pos = s * NLAYER + l + 1;
      mTrackletsPerHC2D->GetYaxis()->SetBinLabel(pos, label.c_str());
    }
  }
  for (int sm = 0; sm < NSECTOR; ++sm) {
    for (int side = 0; side < 2; ++side) {
      std::string label = fmt::format("{0}_{1}", sm, side == 0 ? "A" : "B");
      int pos = sm * 2 + side + 1;
      mTrackletsPerHC2D->GetXaxis()->SetBinLabel(pos, label.c_str());
    }
  }

  // Mask known inactive halfchambers in the active chamber map
  TLine* line[6];
  std::pair<int, int> x, y;
  for (int iHC = 0; iHC < NCHAMBER * 2; ++iHC) {
    if (mChamberStatus != nullptr) {
      if (mChamberStatus->isMasked(iHC)) {
        int stackLayer = Helper::getStack(iHC / 2) * NLAYER + Helper::getLayer(iHC / 2);
        int sectorSide = (iHC / NHCPERSEC) * 2 + (iHC % 2);
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

  for (int chargeWindow = 0; chargeWindow < 3; ++chargeWindow) {
    mTrackletQ[chargeWindow] = new TH1F(Form("TrackletQ%i", chargeWindow), Form("Tracklet Q%i;charge (a.u.);counts", chargeWindow), 256, -0.5, 255.5);
    getObjectsManager()->startPublishing(mTrackletQ[chargeWindow]);
    getObjectsManager()->setDefaultDrawOptions(mTrackletQ[chargeWindow]->GetName(), "logy");
  }

  mTrackletsPerTimeFrame = new TH1F("trackletspertimeframe", "Number of Tracklets per timeframe;Tracklets in TimeFrame;Counts", nLogBins, xBins);
  getObjectsManager()->startPublishing(mTrackletsPerTimeFrame);
  getObjectsManager()->setDefaultDrawOptions(mTrackletsPerTimeFrame->GetName(), "logx");

  mTriggersPerTimeFrame = new TH1F("triggerspertimeframe", "Number of Triggers per timeframe;Triggers in TimeFrame;Counts", 1000, 0, 1000);
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
  for (int iLayer = 0; iLayer < NLAYER; ++iLayer) {
    mLayers[iLayer] = new TH2F(Form("TrackletsPerMCM_Layer%i", iLayer), Form("Tracklet count per MCM in layer %i;glb pad row;glb MCM col", iLayer), 76, -0.5, 75.5, 144, -0.5, 143.5);
    mLayers[iLayer]->SetStats(0);
    drawTrdLayersGrid(mLayers[iLayer]);
    drawHashedOnHistsPerLayer(iLayer);
    getObjectsManager()->startPublishing(mLayers[iLayer]);
    getObjectsManager()->setDefaultDrawOptions(mLayers[iLayer]->GetName(), "COLZ");
    getObjectsManager()->setDisplayHint(mLayers[iLayer], "logz");
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
  mRemoveNoise = getFromConfig<bool>(mCustomParameters, "removeNoise", false);
  buildHistograms();
}

void TrackletsTask::startOfActivity(const Activity& activity)
{
  ILOG(Debug, Devel) << "startOfActivity " << activity.mId << ENDM;
}

void TrackletsTask::startOfCycle()
{
  ILOG(Debug, Devel) << "startOfCycle" << ENDM;
}

void TrackletsTask::monitorData(o2::framework::ProcessingContext& ctx)
{
  // Load CCDB objects (needs to be done only once)
  if (!mNoiseMap) {
    auto ptr = ctx.inputs().get<o2::trd::NoiseStatusMCM*>("noiseMap");
    mNoiseMap = ptr.get();
  }
  if (!mChamberStatus) {
    auto ptr = ctx.inputs().get<o2::trd::HalfChamberStatusQC*>("chamberStatus");
    mChamberStatus = ptr.get();
  }

  auto tracklets = ctx.inputs().get<gsl::span<o2::trd::Tracklet64>>("tracklets");
  auto triggerrecords = ctx.inputs().get<gsl::span<o2::trd::TriggerRecord>>("triggers");
  mTrackletsPerTimeFrame->Fill(tracklets.size());
  mTriggersPerTimeFrame->Fill(triggerrecords.size());

  for (const auto& trigger : triggerrecords) {
    mTrackletsPerEvent->Fill(trigger.getNumberOfTracklets());
    mTrackletsPerEventPP->Fill(trigger.getNumberOfTracklets());
    mTrackletsPerEventPbPb->Fill(trigger.getNumberOfTracklets());
    for (int currenttracklet = trigger.getFirstTracklet(); currenttracklet < trigger.getFirstTracklet() + trigger.getNumberOfTracklets(); ++currenttracklet) {
      const auto& trklt = tracklets[currenttracklet];
      if (mNoiseMap != nullptr && mRemoveNoise && mNoiseMap->isTrackletFromNoisyMCM(trklt)) {
        continue;
      }
      int hcid = trklt.getHCID();
      int layer = Helper::getLayer(hcid / 2);
      int stack = Helper::getStack(hcid / 2);
      int sector = Helper::getSector(hcid / 2);
      int stackLayer = stack * NLAYER + layer;
      int sectorSide = sector * 2 + (hcid % 2);
      int rowGlb = FIRSTROW[stack] + trklt.getPadRow();
      int colGlb = trklt.getMCMCol() + sector * 8;
      mTrackletsPerHC2D->Fill(sectorSide, stackLayer);
      mTrackletSlope->Fill(trklt.getSlopeFloat());
      mTrackletPosition->Fill(trklt.getPositionFloat());
      mTrackletHCID->Fill(hcid);
      mTrackletQ[0]->Fill(trklt.getQ0());
      mTrackletQ[1]->Fill(trklt.getQ1());
      mTrackletQ[2]->Fill(trklt.getQ2());
      mLayers[layer]->Fill(rowGlb, colGlb);
    }
  }
}

void TrackletsTask::endOfCycle()
{
  ILOG(Debug, Devel) << "endOfCycle" << ENDM;
}

void TrackletsTask::endOfActivity(const Activity& /*activity*/)
{
  ILOG(Debug, Devel) << "endOfActivity" << ENDM;
}

void TrackletsTask::reset()
{
  // clean all the monitor objects here

  ILOG(Debug, Devel) << "Resetting the histograms" << ENDM;
  mTrackletSlope->Reset();
  mTrackletHCID->Reset();
  mTrackletPosition->Reset();
  mTrackletsPerEvent->Reset();
  mTrackletsPerEventPP->Reset();
  mTrackletsPerEventPbPb->Reset();
  mTrackletsPerHC2D->Reset();
  mTrackletsPerTimeFrame->Reset();
  mTriggersPerTimeFrame->Reset();
  for (auto h : mLayers) {
    h->Reset();
  }
  for (auto h : mTrackletQ) {
    h->Reset();
  }
}

} // namespace o2::quality_control_modules::trd
