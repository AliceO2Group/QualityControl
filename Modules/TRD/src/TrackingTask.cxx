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
/// \file   TrackingTask.cxx
/// \author Salman Malik
///

// ROOT includes
#include "TLine.h"
#include "TCanvas.h"
#include "TMath.h"
// O2 includes
#include "QualityControl/QcInfoLogger.h"
#include "TRDQC/Tracking.h"
#include <Framework/InputRecord.h>
#include "CCDB/BasicCCDBManager.h"
// QC includes
#include "TRD/TrackingTask.h"
#include "Common/Utils.h"

using namespace o2::trd;
using namespace o2::trd::constants;
using namespace o2::quality_control_modules::common;

namespace o2::quality_control_modules::trd
{
using GID = o2::dataformats::GlobalTrackID;

void TrackingTask::initialize(o2::framework::InitContext& /*ctx*/)
{
  ILOG(Debug, Devel) << "initialize TRD TrackingTask" << ENDM;

  mPtMin = getFromConfig<float>(mCustomParameters, "pTminvalue", 0.f);
  mDetailedTrackQC = getFromConfig<bool>(mCustomParameters, "detailedQC");
  std::string trackSources = getFromConfig<string>(mCustomParameters, "trackSources", "all");
  GID::mask_t allowedSources = GID::getSourcesMask("TPC-TRD,ITS-TPC-TRD");
  mSrcSelected = allowedSources & GID::getSourcesMask(trackSources);
  mDataRequest = std::make_shared<o2::globaltracking::DataRequest>();
  mDataRequest->requestTracks(mSrcSelected, false);

  buildHistograms();
}

void TrackingTask::startOfActivity(const Activity& activity)
{
  ILOG(Debug, Devel) << "startOfActivity " << activity.mId << ENDM;
}

void TrackingTask::startOfCycle()
{
  ILOG(Debug, Devel) << "startOfCycle" << ENDM;
}

void TrackingTask::monitorData(o2::framework::ProcessingContext& ctx)
{
  ILOG(Debug, Devel) << "monitorData" << ENDM;

  mRecoCont.collectData(ctx, *mDataRequest.get());
  std::vector<uint8_t> sources;
  if (mSrcSelected[GID::Source::ITSTPCTRD] == 1) {
    mITSTPCTRDTracks = mRecoCont.getITSTPCTRDTracks<TrackTRD>();
    mTrigITSTPCTRD = mRecoCont.getITSTPCTRDTriggers();
    sources.push_back(GID::Source::ITSTPCTRD);
  }
  if (mSrcSelected[GID::Source::TPCTRD] == 1) {
    mTPCTRDTracks = mRecoCont.getTPCTRDTracks<TrackTRD>();
    mTrigTPCTRD = mRecoCont.getTPCTRDTriggers();
    sources.push_back(GID::Source::TPCTRD);
  }

  const gsl::span<const TrackQC>* trackQcPtr = nullptr;
  using trkQcType = std::decay_t<decltype(ctx.inputs().get<gsl::span<o2::trd::TrackQC>>(""))>;
  std::optional<trkQcType> trackQc;
  if (mDetailedTrackQC) {
    trackQc.emplace(ctx.inputs().get<gsl::span<o2::trd::TrackQC>>("tracksqc"));
    trackQcPtr = &trackQc.value();
  }

  for (auto src : sources) {
    const gsl::span<const TrackTriggerRecord>* trackTriggers = (src == GID::Source::ITSTPCTRD) ? &mTrigITSTPCTRD : &mTrigTPCTRD;
    const gsl::span<const TrackTRD>* tracks = (src == GID::Source::ITSTPCTRD) ? &mITSTPCTRDTracks : &mTPCTRDTracks;
    for (const auto& tracktrig : *trackTriggers) {
      int start = tracktrig.getFirstTrack();
      int end = start + tracktrig.getNumberOfTracks();
      mNtracks->Fill(tracktrig.getNumberOfTracks());
      for (int itrack = start; itrack < end; ++itrack) {
        const auto& trackTRD = (*tracks)[itrack];
        // remove tracks with pt below threshold
        if (trackTRD.getPt() < mPtMin) {
          continue;
        }
        // filling track information
        mTrackPt->Fill(trackTRD.getPt());
        mTrackPhi->Fill(trackTRD.getOuterParam().getPhiPos());
        mNtracklets->Fill(trackTRD.getNtracklets());
        mTrackChi2->Fill(trackTRD.getReducedChi2());
        mTrackEta->Fill(trackTRD.getOuterParam().getEta());
        mTrackEtaPhi->Fill(trackTRD.getOuterParam().getEta(), trackTRD.getOuterParam().getPhiPos());
        // find charge bin of the track
        int charge = trackTRD.getCharge() > 0 ? 0 : 1;
        // eta-phi distribution of tracklets per layer
        mTrackletsEtaPhi[charge]->Fill(trackTRD.getOuterParam().getEta(), trackTRD.getOuterParam().getPhiPos(), trackTRD.getNtracklets());
        mTrackletsEtaPhi[2]->Fill(trackTRD.getOuterParam().getEta(), trackTRD.getOuterParam().getPhiPos(), trackTRD.getNtracklets());
        for (int iLayer = 0; iLayer < NLAYER; iLayer++) {
          // skip layers with no tracklet
          if (trackTRD.getTrackletIndex(iLayer) < 0) {
            continue;
          }
          // eta-phi distribution per layer
          mTracksEtaPhiPerLayer[iLayer][charge]->Fill(trackTRD.getOuterParam().getEta(), trackTRD.getOuterParam().getPhiPos());
        } // end of loop over layers
      }   // end of loop over tracks
    }     // end of loop over track trigger records
  }

  if (mDetailedTrackQC && trackQcPtr) {
    // Residuals in y and z using TRD/TRACKINGQC
    for (const auto& trackqc : *trackQcPtr) {
      // match only ITS-TPC tracks
      auto trackType = trackqc.refGlobalTrackId.getSource();
      if (!(trackType == GID::Source::ITSTPC)) {
        continue;
      }
      // remove tracks with pt below threshold
      if (trackqc.trackTRD.getPt() < mPtMin) {
        continue;
      }
      for (int iLayer = 0; iLayer < NLAYER; iLayer++) {
        // skip layers with no tracklet
        if (trackqc.trackTRD.getTrackletIndex(iLayer) < 0) {
          continue;
        }
        // residuals information
        mDeltaY->Fill(trackqc.trackProp[iLayer].getY() - trackqc.trackletY[iLayer]);
        mDeltaZ->Fill(trackqc.trackProp[iLayer].getZ() - trackqc.trackletZ[iLayer]);
        mDeltaYDet->Fill(trackqc.trklt64[iLayer].getDetector(), trackqc.trackProp[iLayer].getY() - trackqc.trackletY[iLayer]);
        mDeltaZDet->Fill(trackqc.trklt64[iLayer].getDetector(), trackqc.trackProp[iLayer].getZ() - trackqc.trackletZ[iLayer]);
        mDeltaYvsSphi->Fill(trackqc.trackProp[iLayer].getSnp(), trackqc.trackProp[iLayer].getY() - trackqc.trackletY[iLayer]);
        mDeltaYinEtaPerLayer[iLayer]->Fill(trackqc.trackProp[iLayer].getEta(), trackqc.trackProp[iLayer].getY() - trackqc.trackletY[iLayer]);
        mDeltaYinPhiPerLayer[iLayer]->Fill(trackqc.trackProp[iLayer].getPhiPos(), trackqc.trackProp[iLayer].getY() - trackqc.trackletY[iLayer]);
        mTrackletDef->Fill(trackqc.trackProp[iLayer].getSnp(), trackqc.trkltCalib[iLayer].getDy());
      } // end of loop over layers
    }   // end of loop over tracks
  }
}

void TrackingTask::endOfCycle()
{
  ILOG(Debug, Devel) << "endOfCycle" << ENDM;
}

void TrackingTask::endOfActivity(const Activity& /*activity*/)
{
  ILOG(Debug, Devel) << "endOfActivity" << ENDM;
}

void TrackingTask::reset()
{
  ILOG(Debug, Devel) << "Resetting the histogram" << ENDM;
  mNtracks->Reset();
  mNtracklets->Reset();
  mTrackEta->Reset();
  mTrackPhi->Reset();
  mTrackEtaPhi->Reset();
  mTrackPt->Reset();
  mTrackChi2->Reset();
  if (mDetailedTrackQC) {
    mDeltaY->Reset();
    mDeltaZ->Reset();
    mDeltaYDet->Reset();
    mDeltaZDet->Reset();
    mDeltaYvsSphi->Reset();
    mTrackletDef->Reset();
    for (auto h : mDeltaYinEtaPerLayer) {
      h->Reset();
    }
    for (auto h : mDeltaYinPhiPerLayer) {
      h->Reset();
    }
  }
  for (auto h : mTrackletsEtaPhi) {
    h->Reset();
  }
  for (auto h : mTracksEtaPhiPerLayer) {
    for (auto hh : h) {
      hh->Reset();
    }
  }
}

void TrackingTask::buildHistograms()
{
  mNtracks = new TH1D("Ntracks", "Number of matched tracks", 100, 0.0, 100.0);
  axisConfig(mNtracks, "# of tracks", "", "", 1, 1.0, 1.3);
  publishObject(mNtracks);

  mNtracklets = new TH1D("Ntracklets", "Number of Tracklets per Track", 7, -0.5, 6.5);
  axisConfig(mNtracklets, "# of tracklets", "", "", 1, 1.0, 1.1);
  publishObject(mNtracklets);

  mTrackEta = new TH1D("TrackEta", "Eta Distribution", 20, -1.0, 1.0);
  axisConfig(mTrackEta, "#eta", "Counts", "", 1, 1.0, 1.1);
  publishObject(mTrackEta);

  mTrackPhi = new TH1D("TrackPhi", "Phi Distribution", 60, 0, TMath::TwoPi());
  axisConfig(mTrackPhi, "#phi", "Counts", "", 1, 1.0, 1.1);
  publishObject(mTrackPhi);

  mTrackEtaPhi = new TH2D("TrackEtaPhi", "Number of TRD matched tracks;track #eta;track #phi;counts", 100, -1., 1., 180, 0, TMath::TwoPi());
  publishObject(mTrackEtaPhi, "colz");

  mTrackPt = new TH1D("TrackPt", "p_{T} Distribution", 100, 0.0, 10.0);
  axisConfig(mTrackPt, "p_{T} (GeV)", "Counts", "", 1, 1.0, 1.1);
  publishObject(mTrackPt, "", "logx");

  mTrackChi2 = new TH1D("TrackChi2", "Reduced Chi2Distribution", 100, 0.0, 10.0);
  axisConfig(mTrackChi2, "reduced #chi^{2}", "Counts", "", 1, 1.0, 1.1);
  publishObject(mTrackChi2);

  if (mDetailedTrackQC) {
    mDeltaY = new TH1D("DeltaY", "Residuals in Y", 100, -10.0, 10.0);
    axisConfig(mDeltaY, "track Y - tracklet Y (cm)", "Counts", "", 1, 1.0, 1.1);
    publishObject(mDeltaY);

    mDeltaZ = new TH1D("DeltaZ", "Residuals in Z", 100, -25.0, 25.0);
    axisConfig(mDeltaZ, "track Z - tracklet Z (cm)", "Counts", "", 1, 1.0, 1.1);
    publishObject(mDeltaZ);

    mDeltaYDet = new TH2D("DeltaYvsDet", "YResiduals per chamber", 540, -0.5, 539.5, 50, -10, 10);
    axisConfig(mDeltaYDet, "chamber number", "track Y - tracklet Y (cm)", "Counts", 0, 1.0, 1.1);
    publishObject(mDeltaYDet, "colz", "logz");

    mDeltaZDet = new TH2D("DeltaZvsDet", "ZResiduals per chamber", 540, -0.5, 539.5, 50, -25, 25);
    axisConfig(mDeltaZDet, "chamber number", "track Z - tracklet Z (cm)", "Counts", 0, 1.0, 1.1);
    publishObject(mDeltaZDet, "colz", "logz");

    mDeltaYvsSphi = new TH2D("DeltaYsnphi", "YResiduals vs Track sin(#phi)", 50, -0.4, 0.4, 50, -10, 10);
    axisConfig(mDeltaYvsSphi, "track #it{sin(#phi)}", "track Y - tracklet Y (cm)", "Counts", 0, 1.0, 1.1);
    publishObject(mDeltaYvsSphi, "colz", "logz");

    mTrackletDef = new TH2D("TrackletDeflection", "Tracklet slope vs Track sin(#phi)", 100, -0.5, 0.5, 100, -1.5, 1.5);
    axisConfig(mTrackletDef, "track #it{sin(#phi)}", "tracklet D_{y}", "Counts", 0, 1.0, 1.1);
    publishObject(mTrackletDef, "colz", "logz");
  }

  for (int i = 0; i < NLAYER; ++i) {
    for (int j = 0; j < 2; ++j) {
      mTracksEtaPhiPerLayer[i][j] = new TH2D(Form("EtaPhi%sTrackPerLayer/layer%i", mChargeLabel[j].Data(), i), Form("EtaPhi for %s tracks in layer %i", mChargeLabel[j].Data(), i), 100, -0.856, 0.856, 180, 0, TMath::TwoPi());
      axisConfig(mTracksEtaPhiPerLayer[i][j], "#eta", "#phi", "Counts", 0, 1.0, 1.1);
      drawLayers(mTracksEtaPhiPerLayer[i][j]);
      publishObject(mTracksEtaPhiPerLayer[i][j], "colz", "");
    }
    if (mDetailedTrackQC) {
      mDeltaYinEtaPerLayer[i] = new TH2D(Form("YResinEtaPerLayer/layer%i", i), Form("YResiduals in eta for layer %i", i), 100, -0.856, 0.856, 100, -10, 10);
      axisConfig(mDeltaYinEtaPerLayer[i], "#eta", "track Y - tracklet Y (cm)", "Counts", 0, 1.0, 1.1);
      publishObject(mDeltaYinEtaPerLayer[i], "colz", "logz");

      mDeltaYinPhiPerLayer[i] = new TH2D(Form("YResinPhiPerLayer/layer%i", i), Form("YResiduals in phi for layer %i", i), 180, 0, TMath::TwoPi(), 100, -10, 10);
      axisConfig(mDeltaYinPhiPerLayer[i], "#phi", "track Y - tracklet Y (cm)", "Counts", 0, 1.0, 1.1);
      publishObject(mDeltaYinPhiPerLayer[i], "colz", "logz");
    }
  }
  for (int i = 0; i < 3; ++i) {
    mTrackletsEtaPhi[i] = new TProfile2D(Form("EtaPhiTracklets/%sTracks", mChargeLabel[i].Data()), Form("Av. # of tracklets for %s TRD tracks", mChargeLabel[i].Data()), 100, -0.856, 0.856, 180, 0, TMath::TwoPi(), 0, 6);
    axisConfig(mTrackletsEtaPhi[i], "track #eta", "track #phi", "<N_{tracklets}>", 0, 1.0, 1.1);
    drawLayers(mTrackletsEtaPhi[i]);
    publishObject(mTrackletsEtaPhi[i], "colz", "");
  }
}

void TrackingTask::drawLayers(TH2* hist)
{
  TLine* lineEta[4];
  TLine* linePhi[17];
  double etabins[6] = { -0.85, -0.54, -0.16, 0.16, 0.54, 0.85 };
  double phibins[18];
  for (int i = 0; i < 18; i++) {
    phibins[i] = (TMath::TwoPi() / 18) * i;
  }
  for (int iSec = 0; iSec < 4; ++iSec) {
    lineEta[iSec] = new TLine(etabins[iSec + 1], 0, etabins[iSec + 1], TMath::TwoPi());
    lineEta[iSec]->SetLineStyle(2);
    hist->GetListOfFunctions()->Add(lineEta[iSec]);
  }
  for (int iStack = 0; iStack < 17; ++iStack) {
    linePhi[iStack] = new TLine(-0.85, phibins[iStack + 1], 0.85, phibins[iStack + 1]);
    linePhi[iStack]->SetLineStyle(2);
    hist->GetListOfFunctions()->Add(linePhi[iStack]);
  }
}

void TrackingTask::axisConfig(TH1* h, const char* xTitle, const char* yTitle, const char* zTitle, bool stat, float xOffset, float yOffset)
{
  h->GetXaxis()->SetTitle(xTitle);
  h->GetYaxis()->SetTitle(yTitle);
  h->GetXaxis()->SetTitleOffset(xOffset);
  h->GetYaxis()->SetTitleOffset(yOffset);
  h->SetStats(stat);
  if (zTitle) {
    h->GetZaxis()->SetTitle(zTitle);
  }
}

void TrackingTask::publishObject(TObject* aObject, const char* drawOpt, const char* displayOpt)
{
  if (!aObject) {
    ILOG(Debug, Devel) << " ERROR: trying to publish a non-existent histogram " << ENDM;
    return;
  } else {
    getObjectsManager()->startPublishing(aObject);
    if (drawOpt) {
      getObjectsManager()->setDefaultDrawOptions(aObject->GetName(), drawOpt);
    }
    if (displayOpt) {
      getObjectsManager()->setDisplayHint(aObject->GetName(), displayOpt);
    }
    ILOG(Debug, Devel) << " Object will be published: " << aObject->GetName() << ENDM;
  }
}

} // namespace o2::quality_control_modules::trd
