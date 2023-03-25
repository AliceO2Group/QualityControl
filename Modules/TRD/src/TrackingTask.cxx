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

#include "QualityControl/QcInfoLogger.h"
#include "DataFormatsTRD/TrackTRD.h"
#include "DataFormatsTRD/TrackTriggerRecord.h"
#include "DataFormatsTRD/Constants.h"
#include "DataFormatsTRD/Tracklet64.h"
#include "DataFormatsGlobalTracking/RecoContainer.h"
#include "DataFormatsTRD/CalibratedTracklet.h"
#include "TRDQC/Tracking.h"
#include "TRD/TrackingTask.h"
#include <Framework/InputRecord.h>
#include <Framework/InputRecordWalker.h>
#include <Framework/DataRefUtils.h>
#include <Framework/DataSpecUtils.h>
#include "CCDB/BasicCCDBManager.h"
#include "TLine.h"
#include "TCanvas.h"
#include "TMath.h"

using namespace o2::trd;
using namespace o2::trd::constants;

namespace o2::quality_control_modules::trd
{

void TrackingTask::retrieveCCDBSettings()
{
  if (auto param = mCustomParameters.find("ccdbtimestamp"); param != mCustomParameters.end()) {
    mTimestamp = std::stol(mCustomParameters["ccdbtimestamp"]);
    ILOG(Debug, Devel) << "configure() : using ccdbtimestamp = " << mTimestamp << ENDM;
  } else {
    mTimestamp = o2::ccdb::getCurrentTimestamp();
    ILOG(Debug, Devel) << "configure() : using default timestam of now = " << mTimestamp << ENDM;
  }
}

void TrackingTask::initialize(o2::framework::InitContext& /*ctx*/)
{
  ILOG(Debug, Devel) << "initialize TRD TrackingTask" << ENDM;

  retrieveCCDBSettings();
  buildHistograms();
}

void TrackingTask::startOfActivity(Activity& activity)
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

  auto trackqcArr = ctx.inputs().get<gsl::span<o2::trd::TrackQC>>("tracksqc");
  auto tracktrigArr = ctx.inputs().get<gsl::span<o2::trd::TrackTriggerRecord>>("trigrectrk");

  for (const auto& tracktrig : tracktrigArr) {
    int first = tracktrig.getFirstTrack();
    int end = first + tracktrig.getNumberOfTracks();
    // count number of matched tracks
    int nMatched = 0;

    for (int itrack = first; itrack < end; ++itrack) {
      auto& trackqc = trackqcArr[itrack];
      // match only ITS-TPC tracks
      auto trackType = trackqc.refGlobalTrackId.getSource();
      if (!(trackType == GTrackID::Source::ITSTPC))
        continue;
      nMatched++;
      // remove tracks with pt < 1 GeV/c
      if (trackqc.trackTRD.getPt() < 1)
        continue;
      mNtracklets->Fill(trackqc.trackTRD.getNtracklets());
      mTrackPt->Fill(trackqc.trackTRD.getPt());
      mTrackChi2->Fill(trackqc.trackTRD.getReducedChi2());
      // flag to fill some objects only once per track
      bool fillOnce = false;
      for (auto ilayer : { 0, 1, 2, 3, 4, 5 }) {
        // cut on x position of the track
        if (trackqc.trackProp[ilayer].getX() < 2)
          continue;
        // remove tracks with no tracklet in a layer
        if (trackqc.trackTRD.getTrackletIndex(ilayer) < 0)
          continue;
        // find charge bin of the track
        int charge = -1;
        if (trackqc.trackProp[ilayer].getQ2Pt() > 0) {
          charge = 0;
        } else if (trackqc.trackProp[ilayer].getQ2Pt() < 0) {
          charge = 1;
        }
        if (charge == -1)
          continue;
        if (!fillOnce) {
          mTrackEta->Fill(trackqc.trackProp[ilayer].getEta());
          mTrackPhi->Fill(trackqc.trackProp[ilayer].getPhiPos());
          mTrackletsEtaPhi[charge]->Fill(trackqc.trackProp[ilayer].getEta(), trackqc.trackProp[ilayer].getPhiPos(), trackqc.trackTRD.getNtracklets());
          fillOnce = true;
        }
        // fill eta-phi map of tracks
        mTracksEtaPhiPerLayer[charge][ilayer]->Fill(trackqc.trackProp[ilayer].getEta(), trackqc.trackProp[ilayer].getPhiPos());
        // residuals
        mDeltaY->Fill(trackqc.trackProp[ilayer].getY() - trackqc.trackletY[ilayer]);
        mDeltaZ->Fill(trackqc.trackProp[ilayer].getZ() - trackqc.trackletZ[ilayer]);
        mDeltaYDet->Fill(trackqc.trklt64[ilayer].getDetector(), trackqc.trackProp[ilayer].getY() - trackqc.trackletY[ilayer]);
        mDeltaZDet->Fill(trackqc.trklt64[ilayer].getDetector(), trackqc.trackProp[ilayer].getZ() - trackqc.trackletZ[ilayer]);
        mDeltaYsphi->Fill(trackqc.trackProp[ilayer].getSnp(), trackqc.trackProp[ilayer].getY() - trackqc.trackletY[ilayer]);
        mDeltaYinEtaPerLayer[ilayer]->Fill(trackqc.trackProp[ilayer].getEta(), trackqc.trackProp[ilayer].getY() - trackqc.trackletY[ilayer]);
        mDeltaYinPhiPerLayer[ilayer]->Fill(trackqc.trackProp[ilayer].getPhiPos(), trackqc.trackProp[ilayer].getY() - trackqc.trackletY[ilayer]);
        mTrackletDef->Fill(trackqc.trackProp[ilayer].getSnp(), trackqc.trkltCalib[ilayer].getDy());
      } // end of loop over layers
    }   // end of loop over tracks
    mNtracks->Fill(nMatched);
  } // end of loop over track trigger records
}

void TrackingTask::endOfCycle()
{
  ILOG(Debug, Devel) << "endOfCycle" << ENDM;
}

void TrackingTask::endOfActivity(Activity& /*activity*/)
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
  mTrackPt->Reset();
  mTrackChi2->Reset();
  mDeltaY->Reset();
  mDeltaZ->Reset();
  mDeltaYDet->Reset();
  mDeltaZDet->Reset();
  mDeltaYsphi->Reset();
  mTrackletDef->Reset();
  for (auto h : mTrackletsEtaPhi) {
    h->Reset();
  }
  for (auto h : mTracksEtaPhiPerLayer) {
    for (auto hh : h) {
      hh->Reset();
    }
  }
  for (auto h : mDeltaYinEtaPerLayer) {
    h->Reset();
  }
  for (auto h : mDeltaYinPhiPerLayer) {
    h->Reset();
  }
}

void TrackingTask::buildHistograms()
{
  mNtracks = new TH1D("Ntracks", "Number of matched tracks per tf", 100, 0.0, 100.0);
  axisConfig(mNtracks, "# of tracks", "Counts", "", 1, 1.0, 1.3);
  publishObject(mNtracks);

  mNtracklets = new TH1D("Ntracklets", "Number of Tracklets per track", 6, 0.0, 6.0);
  axisConfig(mNtracklets, "# of tracklets", "Counts", "", 1, 1.0, 1.1);
  publishObject(mNtracklets);

  mTrackEta = new TH1D("TrackEta", "Eta Distribution", 20, -1.0, 1.0);
  axisConfig(mTrackEta, "#eta", "Counts", "", 1, 1.0, 1.1);
  publishObject(mTrackEta);

  mTrackPhi = new TH1D("TrackPhi", "Phi Distribution", 60, 0, TMath::TwoPi());
  axisConfig(mTrackPhi, "#phi", "Counts", "", 1, 1.0, 1.1);
  publishObject(mTrackPhi);

  mTrackPt = new TH1D("TrackPt", "p_{#it{T}} Distribution", 100, 0.0, 10.0);
  axisConfig(mTrackPt, "p_{T}", "Counts", "", 1, 1.0, 1.1);
  publishObject(mTrackPt);

  mTrackChi2 = new TH1D("TrackChi2", "Reduced Chi2Distribution", 100, 0.0, 10.0);
  axisConfig(mTrackChi2, "reduced #chi^{2}", "Counts", "", 1, 1.0, 1.1);
  publishObject(mTrackChi2);

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

  mDeltaYsphi = new TH2D("DeltaYsnphi", "YResiduals vs Track sin(#phi)", 50, -0.4, 0.4, 50, -10, 10);
  axisConfig(mDeltaYsphi, "track #it{sin(#phi)}", "track Y - tracklet Y (cm)", "Counts", 0, 1.0, 1.1);
  publishObject(mDeltaYsphi, "colz", "logz");

  mTrackletDef = new TH2D("TrackletDeflection", "Tracklet slope vs Track sin(#phi)", 100, -0.5, 0.5, 100, -1.5, 1.5);
  axisConfig(mTrackletDef, "track #it{sin(#phi)}", "tracklet D_{y}", "Counts", 0, 1.0, 1.1);
  publishObject(mTrackletDef, "colz", "logz");

  for (int i = 0; i < NLayers; ++i) {
    for (int j = 0; j < 2; ++j) {
      mTracksEtaPhiPerLayer[j][i] = new TH2D(Form("EtaPhi%sTrackPerLayer/layer%i", chrg[j].Data(), i), Form("EtaPhi for %s tracks in layer %i", chrg[j].Data(), i), 100, -0.856, 0.856, 180, 0, TMath::TwoPi());
      axisConfig(mTracksEtaPhiPerLayer[j][i], "#eta", "#phi", "Counts", 0, 1.0, 1.1);
      drawLayers(mTracksEtaPhiPerLayer[j][i]);
      publishObject(mTracksEtaPhiPerLayer[j][i], "colz", "");
    }
    mDeltaYinEtaPerLayer[i] = new TH2D(Form("YResinEtaPerLayer/layer%i", i), Form("YResiduals in eta for layer %i", i), 100, -0.856, 0.856, 100, -10, 10);
    axisConfig(mDeltaYinEtaPerLayer[i], "#eta", "track Y - tracklet Y (cm)", "Counts", 0, 1.0, 1.1);
    publishObject(mDeltaYinEtaPerLayer[i], "colz", "logz");

    mDeltaYinPhiPerLayer[i] = new TH2D(Form("YResinPhiPerLayer/layer%i", i), Form("YResiduals in phi for layer %i", i), 180, 0, TMath::TwoPi(), 100, -10, 10);
    axisConfig(mDeltaYinPhiPerLayer[i], "#phi", "track Y - tracklet Y (cm)", "Counts", 0, 1.0, 1.1);
    publishObject(mDeltaYinPhiPerLayer[i], "colz", "logz");
  }
  for (int i = 0; i < 2; ++i) {
    mTrackletsEtaPhi[i] = new TProfile2D(Form("EtaPhiTracklets/%sTracks", chrg[i].Data()), Form("EtaPhi for %s tracks (tracklets)", chrg[i].Data()), 100, -0.856, 0.856, 180, 0, TMath::TwoPi(), 0, 6);
    axisConfig(mTrackletsEtaPhi[i], "#eta", "#phi", "av. # of tracklets per track", 0, 1.0, 1.1);
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
    if (drawOpt != "") {
      getObjectsManager()->setDefaultDrawOptions(aObject->GetName(), drawOpt);
    }
    if (displayOpt != "") {
      getObjectsManager()->setDisplayHint(aObject->GetName(), displayOpt);
    }
    ILOG(Debug, Devel) << " Object will be published: " << aObject->GetName() << ENDM;
  }
}

} // namespace o2::quality_control_modules::trd
