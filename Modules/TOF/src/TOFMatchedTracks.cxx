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
/// \file   TOFMatchedTracks.cxx
/// \author Chiara Zampolli
///

#include <TCanvas.h>
#include <TH1.h>

#include "QualityControl/QcInfoLogger.h"
#include "TOF/TOFMatchedTracks.h"
#include <Framework/InputRecord.h>
#include <Framework/InputRecordWalker.h>
#include <Framework/InputSpec.h>
#include "GlobalTrackingWorkflowHelpers/InputHelper.h"
#include "ReconstructionDataFormats/TrackTPCITS.h"
#include "ReconstructionDataFormats/MatchInfoTOF.h"
#include "DataFormatsGlobalTracking/RecoContainerCreateTracksVariadic.h"

namespace o2::quality_control_modules::tof
{

TOFMatchedTracks::~TOFMatchedTracks()
{
  for (int i = 0; i < trkType::SIZE; ++i) {
    delete mInTracksPt[i];
    delete mInTracksEta[i];
    delete mMatchedTracksPt[i];
    delete mMatchedTracksEta[i];
    delete mEffPt[i];
    delete mEffEta[i];
  }
}

void TOFMatchedTracks::initialize(o2::framework::InitContext& /*ctx*/)
{
  ILOG(Info, Support) << "initialize TOFMatchedTracks" << ENDM; // QcInfoLogger is used. FairMQ logs will go to there as well.

  // this is how to get access to custom parameters defined in the config file at qc.tasks.<task_name>.taskParameters
  if (auto param = mCustomParameters.find("isMC"); param != mCustomParameters.end()) {
    ILOG(Info, Devel) << "Custom parameter - isMC (= use of MC info): " << param->second << ENDM;
    if (param->second == "true" || param->second == "True" || param->second == "TRUE") {
      mUseMC = true;
    }
  }
  if (auto param = mCustomParameters.find("verbose"); param != mCustomParameters.end()) {
    ILOG(Info, Devel) << "Custom parameter - verbose (= verbose printouts): " << param->second << ENDM;
    if (param->second == "true" || param->second == "True" || param->second == "TRUE") {
      mVerbose = true;
    }
  }
  if (auto param = mCustomParameters.find("GID"); param != mCustomParameters.end()) {
    ILOG(Info, Devel) << "Custom parameter - GID (= sources by user): " << param->second << ENDM;
    ILOG(Info, Devel) << "Allowed Sources = " << mAllowedSources << ENDM;
    mSrc = mAllowedSources & GID::getSourcesMask(param->second);
    ILOG(Info, Devel) << "Final requested sources = " << mSrc << ENDM;
  }
  mDataRequest = std::make_shared<o2::globaltracking::DataRequest>();
  mDataRequest->requestTracks(mSrc, mUseMC);

  if ((mSrc[GID::Source::TPCTOF] == 1 && mSrc[GID::Source::TPC] == 0) || (mSrc[GID::Source::TPCTOF] == 0 && mSrc[GID::Source::TPC] == 1)) {
    ILOG(Fatal, Support) << "Check the requested sources: TPCTOF = " << mSrc[GID::Source::TPCTOF] << ", TPC = " << mSrc[GID::Source::TPC] << ENDM;
  }

  if ((mSrc[GID::Source::ITSTPCTOF] == 1 && mSrc[GID::Source::ITSTPC] == 0) || (mSrc[GID::Source::ITSTPCTOF] == 0 && mSrc[GID::Source::ITSTPC] == 1)) {
    ILOG(Fatal, Support) << "Check the requested sources: ITSTPCTOF = " << mSrc[GID::Source::ITSTPCTOF] << ", ITSTPC = " << mSrc[GID::Source::ITSTPC] << ENDM;
  }

  for (int i = 0; i < trkType::SIZE; ++i) {
    mInTracksPt[i] = new TH1D(Form("mInTracksPt_%d", i), Form("mInTracksPt (trkType %d); Pt; counts", i), 100, 0.f, 20.f);
    mInTracksEta[i] = new TH1D(Form("mInTracksEta_%d", i), Form("mInTracksEta (trkType %d); Eta; counts", i), 100, -1.0f, 1.0f);
    mMatchedTracksPt[i] = new TH1D(Form("mMatchedTracksPt_%d", i), Form("mMatchedTracksPt (trkType %d); Pt; counts", i), 100, 0.f, 20.f);
    mMatchedTracksEta[i] = new TH1D(Form("mMatchedTracksEta_%d", i), Form("mMatchedTracksEta (trkType %d); Eta; counts", i), 100, -1.0f, 1.0f);
    mEffPt[i] = new TH1D(Form("mEffPt_%d", i), Form("Efficiency vs Pt (trkType %d); Pt; Eff", i), 100, 0.f, 20.f);
    mEffEta[i] = new TH1D(Form("mEffEta_%d", i), Form("Efficiency vs Eta (trkType %d); Eta; Eff", i), 100, -1.f, 1.f);
    getObjectsManager()->startPublishing(mInTracksPt[i]);
    getObjectsManager()->startPublishing(mInTracksEta[i]);
    getObjectsManager()->startPublishing(mMatchedTracksPt[i]);
    getObjectsManager()->startPublishing(mMatchedTracksEta[i]);
    getObjectsManager()->startPublishing(mEffPt[i]);
    getObjectsManager()->startPublishing(mEffEta[i]);
  }
}

void TOFMatchedTracks::startOfActivity(Activity& activity)
{
  ILOG(Info, Support) << "startOfActivity " << activity.mId << ENDM;
  reset();
}

void TOFMatchedTracks::startOfCycle()
{
  ILOG(Info, Support) << "startOfCycle" << ENDM;
}

void TOFMatchedTracks::monitorData(o2::framework::ProcessingContext& ctx)
{

  mRecoCont.collectData(ctx, *mDataRequest.get());

  // TPC-TOF
  if (mRecoCont.isTrackSourceLoaded(GID::TPCTOF)) { // this is enough to know that also TPC was loades, see "initialize"
    mTPCTracks = mRecoCont.getTPCTracks();
    mTPCTOFMatches = mRecoCont.getTPCTOFMatches();
    ILOG(Info, Support) << "We found " << mTPCTracks.size() << " TPC-only tracks" << ENDM;
    ILOG(Info, Support) << "We found " << mRecoCont.getTPCTOFTracks().size() << " TPC-TOF tracks" << ENDM;
    ILOG(Info, Support) << "We found " << mTPCTOFMatches.size() << " TPC-only tracks matched to TOF" << ENDM;
    if (mRecoCont.getTPCTOFTracks().size() != mTPCTOFMatches.size()) {
      ILOG(Fatal, Support) << "Number of TPCTOF tracks (" << mRecoCont.getTPCTOFTracks().size() << ") differs from number of TPCTOF matches (" << mTPCTOFMatches.size() << ")" << ENDM;
    }
    // loop over TOF MatchInfo
    for (const auto& matchTOF : mTPCTOFMatches) {
      const auto& trk = mTPCTracks[matchTOF.getTrackIndex()];
      mMatchedTracksPt[trkType::UNCONS]->Fill(trk.getPt());
      mMatchedTracksEta[trkType::UNCONS]->Fill(trk.getEta());
    }
  }

  // ITS-TPC-TOF
  if (mRecoCont.isTrackSourceLoaded(GID::ITSTPCTOF)) { // this is enough to know that also ITSTPC was loades, see "initialize"
    mITSTPCTracks = mRecoCont.getTPCITSTracks();
    mITSTPCTOFMatches = mRecoCont.getITSTPCTOFMatches();
    ILOG(Info, Support) << "We found " << mITSTPCTracks.size() << " ITS-TPC tracks" << ENDM;
    ILOG(Info, Support) << "We found " << mITSTPCTOFMatches.size() << " ITS-TPC tracks matched to TOF" << ENDM;
    // loop over TOF MatchInfo
    for (const auto& matchTOF : mITSTPCTOFMatches) {
      const auto& trk = mITSTPCTracks[matchTOF.getTrackIndex()];
      mMatchedTracksPt[trkType::CONSTR]->Fill(trk.getPt());
      mMatchedTracksEta[trkType::CONSTR]->Fill(trk.getEta());
    }
  }

  auto creator = [this](auto& trk, GID, float, float) {
    // Getting the tracks for the denominator of the efficiencies for TPC-TOF tracks;
    // The RecoContainer will provide as TPCtracks only those not matched to TOF (lower
    // quality), so we need to ask also for the TPCTOF to have the full set in the denominator
    if constexpr (isTPCTrack<decltype(trk)>() || isTPCTOFTrack<decltype(trk)>()) {
      // some quality cuts should be applied on the track.
      // they might depend on the track type, so in case one needs to acces
      // the correct track. For now, using an "if-statement" approach
      // but it should be replaced with the official TrackCut class in O2.
      // E.g. (till the TrackCut class is used):
      int nMinTPCClusters = 0;
      if constexpr (isTPCTrack<decltype(trk)>()) {
        if (trk.getNClusters() < nMinTPCClusters) {
          return true;
        }
      } else if constexpr (isTPCTOFTrack<decltype(trk)>()) {
        const auto& tpcTrack = mTPCTracks[mTPCTOFMatches[trk.getRefMatch()].getTrackIndex()];
        if (tpcTrack.getNClusters() < nMinTPCClusters) {
          return true;
        }
      }
      this->mInTracksPt[trkType::UNCONS]->Fill(trk.getPt());
      this->mInTracksEta[trkType::UNCONS]->Fill(trk.getEta());
    }
    // In case of ITS-TPC-TOF, the ITS-TPC tracks contain also the ITS-TPC-TOF
    if constexpr (isTPCITSTrack<decltype(trk)>()) {
      this->mInTracksPt[trkType::CONSTR]->Fill(trk.getPt());
      this->mInTracksEta[trkType::CONSTR]->Fill(trk.getEta());
    }
    return true;
  };
  mRecoCont.createTracksVariadic(creator);

  int nTPCOnlytracks = mRecoCont.getTPCTracks().size() > 0 ? (mRecoCont.getTPCTracks().size() - mITSTPCTracks.size()) : 0;
  ILOG(Info, Support) << "We have " << mInTracksPt[trkType::UNCONS]->GetEntries() << " unconstrained tracks at denominator (should be " << nTPCOnlytracks << "), and " << mInTracksPt[trkType::CONSTR]->GetEntries() << " constrained tracks at denominator (should be " << mRecoCont.getTPCITSTracks().size() << " but *before any quality cut!!*)" << ENDM;
  ILOG(Info, Support) << "We have " << mMatchedTracksPt[trkType::UNCONS]->GetEntries() << " TOF matches from unconstrained tracks (should be " << mTPCTOFMatches.size() << "), and " << mMatchedTracksPt[trkType::CONSTR]->GetEntries() << " TOF matches from constrained tracks (should be " << mITSTPCTOFMatches.size() << " but *before any quality cut!!*))" << ENDM;
}

void TOFMatchedTracks::endOfCycle()
{

  ILOG(Info, Support) << "endOfCycle" << ENDM;
  for (int i = 0; i < trkType::SIZE; ++i) {
    mEffPt[i]->Divide(mMatchedTracksPt[i], mInTracksPt[i], 1, 1, "b");
    mEffEta[i]->Divide(mMatchedTracksEta[i], mInTracksEta[i], 1, 1, "b");
  }

  // Printing, for checks
  if (mVerbose) {
    for (int i = 0; i < trkType::SIZE; ++i) {
      ILOG(Debug, Support) << "Type " << i << ENDM;
      ILOG(Debug, Support) << "Pt plot" << ENDM;
      for (int ibin = 0; ibin < mEffPt[i]->GetNbinsX(); ++ibin) {
        ILOG(Debug, Support) << "mMatchedTracksPt[" << i << "]->GetBinContent(" << ibin + 1 << ") = " << mMatchedTracksPt[i]->GetBinContent(ibin + 1);
        ILOG(Debug, Support) << ", mInTracksPt[" << i << "]->GetBinContent(" << ibin + 1 << ") = " << mInTracksPt[i]->GetBinContent(ibin + 1);
        ILOG(Debug, Support) << ", mEffPt[" << i << "]->GetBinContent(" << ibin + 1 << ") = " << mEffPt[i]->GetBinContent(ibin + 1);
        if (mInTracksPt[i]->GetBinContent(ibin + 1) > 0) {
          ILOG(Debug, Support) << " (should be " << mMatchedTracksPt[i]->GetBinContent(ibin + 1) / mInTracksPt[i]->GetBinContent(ibin + 1) << ")" << ENDM;
        } else {
          ILOG(Debug, Support) << ENDM;
        }
      }
      ILOG(Debug, Support) << ENDM;
      ILOG(Debug, Support) << "Eta plot" << ENDM;
      for (int ibin = 0; ibin < mEffEta[i]->GetNbinsX(); ++ibin) {
        ILOG(Debug, Support) << "mMatchedTracksEta[" << i << "]->GetBinContent(" << ibin + 1 << ") = " << mMatchedTracksEta[i]->GetBinContent(ibin + 1);
        ILOG(Debug, Support) << ", mInTracksEta[" << i << "]->GetBinContent(" << ibin + 1 << ") = " << mInTracksEta[i]->GetBinContent(ibin + 1);
        ILOG(Debug, Support) << ", mEffEta[" << i << "]->GetBinContent(" << ibin + 1 << ") = " << mEffEta[i]->GetBinContent(ibin + 1);
        if (mInTracksEta[i]->GetBinContent(ibin + 1) > 0) {
          ILOG(Debug, Support) << " (should be " << mMatchedTracksEta[i]->GetBinContent(ibin + 1) / mInTracksEta[i]->GetBinContent(ibin + 1) << ")" << ENDM;
        } else {
          ILOG(Debug, Support) << ENDM;
        }
      }
      ILOG(Debug, Support) << ENDM;
    }
  }
}

void TOFMatchedTracks::endOfActivity(Activity& /*activity*/)
{
  ILOG(Info, Support) << "endOfActivity" << ENDM;
}

void TOFMatchedTracks::reset()
{
  // clean all the monitor objects here

  ILOG(Info, Support) << "Resetting the histogram" << ENDM;
  for (int i = 0; i < trkType::SIZE; ++i) {
    mInTracksPt[i]->Reset();
    mInTracksEta[i]->Reset();
    mMatchedTracksPt[i]->Reset();
    mMatchedTracksEta[i]->Reset();
    mEffPt[i]->Reset();
    mEffEta[i]->Reset();
  }
}

} // namespace o2::quality_control_modules::tof
