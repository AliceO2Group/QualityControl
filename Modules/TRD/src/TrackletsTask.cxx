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
#include "TRD/TRDHelpers.h"
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

void TrackletsTask::initialize(o2::framework::InitContext& /*ctx*/)
{
  ILOG(Debug, Devel) << "initialize TrackletsTask" << ENDM;
  mRemoveNoise = getFromConfig<bool>(mCustomParameters, "removeNoise", false);
  buildHistograms();
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
  mTrackletSlope.reset(new TH1F("trackletslope", "Tracklet inclination in natural units;pads per time bin;counts", 100, -0.15, 0.15));
  getObjectsManager()->startPublishing(mTrackletSlope.get());

  mTrackletHCID.reset(new TH1F("tracklethcid", "Tracklet distribution over Halfchambers;HalfChamber ID;counts", 1080, -0.5, 1079.5));
  getObjectsManager()->startPublishing(mTrackletHCID.get());

  mTrackletPosition.reset(new TH1F("trackletpos", "Tracklet position relative to MCM center;number of pads;counts", 200, -30, 30));
  getObjectsManager()->startPublishing(mTrackletPosition.get());

  mTrackletsPerEvent.reset(new TH1F("trackletsperevent", "Number of Tracklets per event;Tracklets in Event;Counts", nLogBins, xBins));
  getObjectsManager()->startPublishing(mTrackletsPerEvent.get());
  getObjectsManager()->setDefaultDrawOptions(mTrackletsPerEvent->GetName(), "logx");

  mTrackletsPerEventPP.reset(new TH1F("trackletspereventPP", "Number of Tracklets per event;Tracklets in Event;Counts", 1000, 0, 5000));
  getObjectsManager()->startPublishing(mTrackletsPerEventPP.get());

  mTrackletsPerEventPbPb.reset(new TH1F("trackletspereventPbPb", "Number of Tracklets per event;Tracklets in Event;Counts", 1000, 0, 100000));
  getObjectsManager()->startPublishing(mTrackletsPerEventPbPb.get());

  mTrackletsPerHC2D.reset(new TH2F("trackletsperHC2D", "Tracklets distribution in half-chambers;Sector_Side;Stack_Side", 36, 0, 36, 30, 0, 30));
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

  getObjectsManager()->startPublishing(mTrackletsPerHC2D.get());
  getObjectsManager()->setDefaultDrawOptions(mTrackletsPerHC2D->GetName(), "COLZ");
  getObjectsManager()->setDisplayHint(mTrackletsPerHC2D->GetName(), "logz");

  for (int chargeWindow = 0; chargeWindow < 3; ++chargeWindow) {
    mTrackletQ[chargeWindow].reset(new TH1F(Form("TrackletQ%i", chargeWindow), Form("Tracklet Q%i;charge (a.u.);counts", chargeWindow), 256, -0.5, 255.5));
    getObjectsManager()->startPublishing(mTrackletQ[chargeWindow].get());
    getObjectsManager()->setDefaultDrawOptions(mTrackletQ[chargeWindow]->GetName(), "logy");
  }

  mTrackletsPerTimeFrame.reset(new TH1F("trackletspertimeframe", "Number of Tracklets per timeframe;Tracklets in TimeFrame;Counts", nLogBins, xBins));
  getObjectsManager()->startPublishing(mTrackletsPerTimeFrame.get());
  getObjectsManager()->setDefaultDrawOptions(mTrackletsPerTimeFrame->GetName(), "logx");

  mTriggersPerTimeFrame.reset(new TH1F("triggerspertimeframe", "Number of Triggers per timeframe;Triggers in TimeFrame;Counts", 1000, 0, 1000));
  getObjectsManager()->startPublishing(mTriggersPerTimeFrame.get());

  // Build tracklet layers
  int unitsPerSection = NCOLUMN / NSECTOR;
  for (int iLayer = 0; iLayer < NLAYER; ++iLayer) {
    mLayers[iLayer].reset(new TH2F(Form("TrackletsPerMCM_Layer%i", iLayer), Form("Tracklet count per MCM in layer %i;glb pad row;glb MCM col", iLayer),
                                   76, -0.5, 75.5, unitsPerSection * 18, -0.5, unitsPerSection * 18 - 0.5));
    mLayers[iLayer]->SetStats(0);
    TRDHelpers::addChamberGridToHistogram(mLayers[iLayer].get(), unitsPerSection);
    getObjectsManager()->startPublishing(mLayers[iLayer].get());
    getObjectsManager()->setDefaultDrawOptions(mLayers[iLayer]->GetName(), "COLZ");
    getObjectsManager()->setDisplayHint(mLayers[iLayer].get(), "logz");
  }
}

void TrackletsTask::monitorData(o2::framework::ProcessingContext& ctx)
{
  // Load CCDB objects (needs to be done only once)
  if (!mNoiseMap) {
    auto ptr = ctx.inputs().get<o2::trd::NoiseStatusMCM*>("noiseMap");
    mNoiseMap = ptr.get();
  }

  if (!mChamberStatus) {
    auto ptr = ctx.inputs().get<std::array<int, MAXCHAMBER>*>("chamberStatus");
    mChamberStatus = ptr.get();
    // LB: only draw in plots if it is first instance, e.g. null ptr to non null ptr
    if (mChamberStatus) {
      TRDHelpers::drawChamberStatusOnHistograms(mChamberStatus, mTrackletsPerHC2D, mLayers, NCOLUMN / NSECTOR);
    } else {
      ILOG(Info, Support) << "Failed to retrieve ChamberStatus, so it will not show on plots" << ENDM;
    }
  }

  // Fill histograms
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

void TrackletsTask::startOfActivity(const Activity& activity)
{
  ILOG(Debug, Devel) << "startOfActivity " << activity.mId << ENDM;
}

void TrackletsTask::startOfCycle()
{
  ILOG(Debug, Devel) << "startOfCycle" << ENDM;
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
