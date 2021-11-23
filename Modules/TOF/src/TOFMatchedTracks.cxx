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
/// \brief  Task to monitor TOF matching efficiency
/// \since  03/08/2021
///

#include <TCanvas.h>
#include <TH1.h>
#include <TEfficiency.h>

#include "QualityControl/QcInfoLogger.h"
#include "TOF/TOFMatchedTracks.h"
#include <Framework/InputRecord.h>
#include <Framework/InputRecordWalker.h>
#include <Framework/InputSpec.h>
#include "GlobalTrackingWorkflowHelpers/InputHelper.h"
#include "ReconstructionDataFormats/TrackTPCITS.h"
#include "ReconstructionDataFormats/TrackTPCITS.h"
#include "DataFormatsTPC/TrackTPC.h"
#include "DataFormatsGlobalTracking/RecoContainerCreateTracksVariadic.h"
#include "ReconstructionDataFormats/TrackParametrization.h"
#include "DetectorsBase/Propagator.h"
#include "DetectorsBase/GeometryManager.h"

using GTrackID = o2::dataformats::GlobalTrackID;

namespace o2::quality_control_modules::tof
{

TOFMatchedTracks::~TOFMatchedTracks()
{
  for (int i = 0; i < trkType::SIZE; ++i) {
    delete mMatchedTracksPt[i];
    delete mMatchedTracksEta[i];
    if (mUseMC) {
      delete mFakeMatchedTracksPt[i];
      delete mFakeMatchedTracksEta[i];
    }
    delete mInTracksPt[i];
    delete mInTracksEta[i];
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

  // for track selection
  if (auto param = mCustomParameters.find("minPtCut"); param != mCustomParameters.end()) {
    ILOG(Info, Devel) << "Custom parameter - minPtCut (for track selection): " << param->second << ENDM;
    setPtCut(atof(param->second.c_str()));
  }
  if (auto param = mCustomParameters.find("etaCut"); param != mCustomParameters.end()) {
    ILOG(Info, Devel) << "Custom parameter - etaCut (for track selection): " << param->second << ENDM;
    setEtaCut(atof(param->second.c_str()));
  }
  if (auto param = mCustomParameters.find("minNTPCClustersCut"); param != mCustomParameters.end()) {
    ILOG(Info, Devel) << "Custom parameter - minNTPCClustersCut (for track selection): " << param->second << ENDM;
    setMinNTPCClustersCut(atoi(param->second.c_str()));
  }
  if (auto param = mCustomParameters.find("minDCACut"); param != mCustomParameters.end()) {
    ILOG(Info, Devel) << "Custom parameter - minDCACut (for track selection): " << param->second << ENDM;
    setMinDCAtoBeamPipeDistanceCut(atof(param->second.c_str()));
  }
  if (auto param = mCustomParameters.find("minDCACutY"); param != mCustomParameters.end()) {
    ILOG(Info, Devel) << "Custom parameter - minDCACutY (for track selection): " << param->second << ENDM;
    setMinDCAtoBeamPipeYCut(atof(param->second.c_str()));
  }

  // for track type selection
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

  std::array<std::string, 2> title{ "UNCONS", "CONSTR" };
  for (int i = 0; i < trkType::SIZE; ++i) {
    mInTracksPt[i] = new TH1F(Form("mInTracksPt_%s", title[i].c_str()), Form("mInTracksPt (trkType: %s); Pt; counts", title[i].c_str()), 100, 0.f, 20.f);
    mInTracksEta[i] = new TH1F(Form("mInTracksEta_%s", title[i].c_str()), Form("mInTracksEta (trkType: %s); Eta; counts", title[i].c_str()), 100, -1.0f, 1.0f);
    mMatchedTracksPt[i] = new TH1F(Form("mMatchedTracksPt_%s", title[i].c_str()), Form("mMatchedTracksPt (trkType: %s); Pt; counts", title[i].c_str()), 100, 0.f, 20.f);
    mMatchedTracksEta[i] = new TH1F(Form("mMatchedTracksEta_%s", title[i].c_str()), Form("mMatchedTracksEta (trkType: %s); Eta; counts", title[i].c_str()), 100, -1.0f, 1.0f);
    if (mUseMC) {
      mFakeMatchedTracksPt[i] = new TH1F(Form("mFakeMatchedTracksPt_%s", title[i].c_str()), Form("mFakeMatchedTracksPt (trkType: %s); Pt; counts", title[i].c_str()), 100, 0.f, 20.f);
      mFakeMatchedTracksEta[i] = new TH1F(Form("mFakeMatchedTracksEta_%s", title[i].c_str()), Form("mFakeMatchedTracksEta (trkType: %s); Eta; counts", title[i].c_str()), 100, -1.0f, 1.0f);
      mFakeFractionTracksPt[i] = new TEfficiency(Form("mFakeFractionPt_%s", title[i].c_str()), Form("Fraction of fake matches vs Pt (trkType: %s); Pt; Eff", title[i].c_str()), 100, 0.f, 20.f);
      mFakeFractionTracksEta[i] = new TEfficiency(Form("mFakeFractionEta_%s", title[i].c_str()), Form("Fraction of fake matches vs Eta (trkType: %s); Eta; Eff", title[i].c_str()), 100, -1.0f, 1.0f);
    }
    mEffPt[i] = new TEfficiency(Form("mEffPt_%s", title[i].c_str()), Form("Efficiency vs Pt (trkType: %s); Pt; Eff", title[i].c_str()), 100, 0.f, 20.f);
    mEffEta[i] = new TEfficiency(Form("mEffEta_%s", title[i].c_str()), Form("Efficiency vs Eta (trkType: %s); Eta; Eff", title[i].c_str()), 100, -1.f, 1.f);
  }

  // initialize B field and geometry for track selection
  o2::base::GeometryManager::loadGeometry(mGeomFileName);
  o2::base::Propagator::initFieldFromGRP(mGRPFileName);
  mBz = o2::base::Propagator::Instance()->getNominalBz();

  if (mSrc[GID::Source::TPCTOF] == 1) {
    getObjectsManager()->startPublishing(mInTracksPt[trkType::UNCONS]);
    getObjectsManager()->startPublishing(mInTracksEta[trkType::UNCONS]);
    getObjectsManager()->startPublishing(mMatchedTracksPt[trkType::UNCONS]);
    getObjectsManager()->startPublishing(mMatchedTracksEta[trkType::UNCONS]);
    if (mUseMC) {
      getObjectsManager()->startPublishing(mFakeMatchedTracksPt[trkType::UNCONS]);
      getObjectsManager()->startPublishing(mFakeMatchedTracksEta[trkType::UNCONS]);
      getObjectsManager()->startPublishing(mFakeFractionTracksPt[trkType::UNCONS]);
      getObjectsManager()->startPublishing(mFakeFractionTracksEta[trkType::UNCONS]);
    }
    getObjectsManager()->startPublishing(mEffPt[trkType::UNCONS]);
    getObjectsManager()->startPublishing(mEffEta[trkType::UNCONS]);
  }

  if (mSrc[GID::Source::ITSTPCTOF] == 1) {
    getObjectsManager()->startPublishing(mInTracksPt[trkType::CONSTR]);
    getObjectsManager()->startPublishing(mInTracksEta[trkType::CONSTR]);
    getObjectsManager()->startPublishing(mMatchedTracksPt[trkType::CONSTR]);
    getObjectsManager()->startPublishing(mMatchedTracksEta[trkType::CONSTR]);
    if (mUseMC) {
      getObjectsManager()->startPublishing(mFakeMatchedTracksPt[trkType::CONSTR]);
      getObjectsManager()->startPublishing(mFakeMatchedTracksEta[trkType::CONSTR]);
      getObjectsManager()->startPublishing(mFakeFractionTracksPt[trkType::CONSTR]);
      getObjectsManager()->startPublishing(mFakeFractionTracksEta[trkType::CONSTR]);
    }
    getObjectsManager()->startPublishing(mEffPt[trkType::CONSTR]);
    getObjectsManager()->startPublishing(mEffEta[trkType::CONSTR]);
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

  ++mTF;
  LOG(DEBUG) << " ************************ ";
  LOG(DEBUG) << " *** Processing TF " << mTF << " *** ";
  LOG(DEBUG) << " ************************ ";
  mRecoCont.collectData(ctx, *mDataRequest.get());

  // TPC-TOF
  if (mRecoCont.isTrackSourceLoaded(GID::TPCTOF)) { // this is enough to know that also TPC was loades, see "initialize"
    mTPCTracks = mRecoCont.getTPCTracks();
    mTPCTOFMatches = mRecoCont.getTPCTOFMatches();
    LOG(DEBUG) << "We found " << mTPCTracks.size() << " TPC-only tracks";
    LOG(DEBUG) << "We found " << mRecoCont.getTPCTOFTracks().size() << " TPC-TOF tracks";
    LOG(DEBUG) << "We found " << mTPCTOFMatches.size() << " TPC-only tracks matched to TOF";
    if (mRecoCont.getTPCTOFTracks().size() != mTPCTOFMatches.size()) {
      ILOG(Fatal, Support) << "Number of TPCTOF tracks (" << mRecoCont.getTPCTOFTracks().size() << ") differs from number of TPCTOF matches (" << mTPCTOFMatches.size() << ")" << ENDM;
    }
    // loop over TOF MatchInfo
    for (const auto& matchTOF : mTPCTOFMatches) {
      GTrackID gTrackId = matchTOF.getTrackRef();
      const auto& trk = mTPCTracks[gTrackId.getIndex()];
      if (!selectTrack(trk)) {
        LOG(DEBUG) << "NUM UNCONS: track with eta " << trk.getEta() << " and pt " << trk.getPt() << " DISCARDED for numerator, UNCONS";
        continue;
      }
      LOG(DEBUG) << "NUM UNCONS: track with eta " << trk.getEta() << " and pt " << trk.getPt() << " ACCEPTED for numerator, UNCONS";
      mMatchedTracksPt[trkType::UNCONS]->Fill(trk.getPt());
      mMatchedTracksEta[trkType::UNCONS]->Fill(trk.getEta());
      if (mUseMC) {
        auto lbl = mRecoCont.getTrackMCLabel(gTrackId);
        if (lbl.isFake()) {
          mFakeMatchedTracksPt[trkType::UNCONS]->Fill(trk.getPt());
          mFakeMatchedTracksEta[trkType::UNCONS]->Fill(trk.getEta());
        }
      }
    }
  }

  // ITS-TPC-TOF
  if (mRecoCont.isTrackSourceLoaded(GID::ITSTPCTOF)) { // this is enough to know that also ITSTPC was loades, see "initialize"
    mITSTPCTracks = mRecoCont.getTPCITSTracks();
    mITSTPCTOFMatches = mRecoCont.getITSTPCTOFMatches();
    LOG(DEBUG) << "We found " << mITSTPCTracks.size() << " ITS-TPC tracks";
    LOG(DEBUG) << "We found " << mITSTPCTOFMatches.size() << " ITS-TPC tracks matched to TOF";
    // loop over TOF MatchInfo
    for (const auto& matchTOF : mITSTPCTOFMatches) {
      GTrackID gTrackId = matchTOF.getTrackRef();
      const auto& trk = mITSTPCTracks[gTrackId.getIndex()];
      const auto& trkTPC = mTPCTracks[trk.getRefTPC()];
      if (!selectTrack(trkTPC)) {
        LOG(DEBUG) << "NUM CONSTR: track with eta " << trkTPC.getEta() << " and pT " << trkTPC.getPt() << " DISCARDED for numerator, CONSTR";
        continue;
      }
      LOG(DEBUG) << "NUM CONSTR: track with eta " << trkTPC.getEta() << " and pT " << trkTPC.getPt() << " ACCEPTED for numerator, CONSTR"
                 << " gid: " << gTrackId << " TPC gid =" << trk.getRefTPC();
      mMatchedTracksPt[trkType::CONSTR]->Fill(trkTPC.getPt());
      mMatchedTracksEta[trkType::CONSTR]->Fill(trkTPC.getEta());
      if (mUseMC) {
        auto lbl = mRecoCont.getTrackMCLabel(gTrackId);
        if (lbl.isFake()) {
          mFakeMatchedTracksPt[trkType::CONSTR]->Fill(trkTPC.getPt());
          mFakeMatchedTracksEta[trkType::CONSTR]->Fill(trkTPC.getEta());
        }
      }
    }
  }

  auto creator = [this](auto& trk, GID gid, float, float) {
    // Getting the tracks for the denominator of the efficiencies for TPC-TOF tracks;
    // The RecoContainer will provide as TPCtracks only those not matched to TOF (lower
    // quality), so we need to ask also for the TPCTOF to have the full set in the denominator
    if constexpr (isTPCTrack<decltype(trk)>() || isTPCTOFTrack<decltype(trk)>()) {
      // some quality cuts should be applied on the track.
      // they might depend on the track type, so in case one needs to acces
      // the correct track. For now, using an "if-statement" approach
      // but it should be replaced with the official TrackCut class in O2.
      // E.g. (till the TrackCut class is used):
      if constexpr (isTPCTrack<decltype(trk)>()) {
        if (!selectTrack(trk)) {
          LOG(DEBUG) << "DEN UNCONS: track with eta " << trk.getEta() << " and pT " << trk.getPt() << " DISCARDED for denominator UNCONS, TPC track";
          return true;
        }
        LOG(DEBUG) << "DEN UNCONS: track with eta " << trk.getEta() << " and pT " << trk.getPt() << " ACCEPTED for denominator UNCONS, TPC track"
                   << " gid: " << gid;
        this->mInTracksPt[trkType::UNCONS]->Fill(trk.getPt());
        this->mInTracksEta[trkType::UNCONS]->Fill(trk.getEta());
      } else if constexpr (isTPCTOFTrack<decltype(trk)>()) {
        const auto& tpcTrack = mTPCTracks[mTPCTOFMatches[trk.getRefMatch()].getTrackRef().getIndex()];
        if (!selectTrack(tpcTrack)) {
          LOG(DEBUG) << "DEN UNCONS: track with eta " << tpcTrack.getEta() << " and pT " << tpcTrack.getPt() << " DISCARDED for denominator UNCONS, TPCTOF track";
          return true;
        }
        LOG(DEBUG) << "DEN UNCONS: track with eta " << tpcTrack.getEta() << " and pT " << tpcTrack.getPt() << " ACCEPTED for denominator UNCONS, TPCTOF track";
        this->mInTracksPt[trkType::UNCONS]->Fill(tpcTrack.getPt());
        this->mInTracksEta[trkType::UNCONS]->Fill(tpcTrack.getEta());
      }
    }
    // In case of ITS-TPC-TOF, the ITS-TPC tracks contain also the ITS-TPC-TOF
    if constexpr (isTPCITSTrack<decltype(trk)>()) {
      const auto& tpcTrack = mTPCTracks[trk.getRefTPC().getIndex()];
      if (!selectTrack(tpcTrack)) {
        LOG(DEBUG) << "DEN CONSTR: track with eta " << tpcTrack.getEta() << " and pT " << tpcTrack.getPt() << " DISCARDED for denominator CONSTR, ITSTPC track";
        return true;
      }
      LOG(DEBUG) << "DEN CONSTR: track with eta " << tpcTrack.getEta() << " and pT " << tpcTrack.getPt() << " ACCEPTED for denominator CONSTR, ITSTPC track";
      this->mInTracksPt[trkType::CONSTR]->Fill(tpcTrack.getPt());
      this->mInTracksEta[trkType::CONSTR]->Fill(tpcTrack.getEta());
    }
    return true;
  };
  mRecoCont.createTracksVariadic(creator);

  int nTPCOnlytracks = mRecoCont.getTPCTracks().size() > 0 ? (mRecoCont.getTPCTracks().size() - mITSTPCTracks.size()) : 0;

  LOG(DEBUG) << "We have " << mInTracksPt[trkType::UNCONS]->GetEntries() << " unconstrained tracks at denominator (should be " << nTPCOnlytracks << "), and " << mInTracksPt[trkType::CONSTR]->GetEntries() << " constrained tracks at denominator (should be " << mRecoCont.getTPCITSTracks().size() << " but *before any quality cut!!*)";
  LOG(DEBUG) << "We have " << mMatchedTracksPt[trkType::UNCONS]->GetEntries() << " TOF matches from unconstrained tracks (should be " << mTPCTOFMatches.size() << "), and " << mMatchedTracksPt[trkType::CONSTR]->GetEntries() << " TOF matches from constrained tracks (should be " << mITSTPCTOFMatches.size() << " but *before any quality cut!!*))";

  LOG(DEBUG) << "We have " << mInTracksPt[trkType::UNCONS]->GetEntries() << " unconstrained tracks at denominator, and " << mInTracksPt[trkType::CONSTR]->GetEntries() << " constrained tracks at denominator";
  LOG(DEBUG) << "We have " << mMatchedTracksPt[trkType::UNCONS]->GetEntries() << " TOF matches from unconstrained tracks and " << mMatchedTracksPt[trkType::CONSTR]->GetEntries() << " TOF matches from constrained tracks";

  // logging in case denominator has less tracks than numerator
  for (int i = 0; i < trkType::SIZE; ++i) {
    for (int ibin = 1; ibin <= mMatchedTracksPt[i]->GetNbinsX(); ++ibin) {
      LOG(DEBUG) << "check: ibin " << ibin << ": mInTracksPt[" << i << "]->GetBinContent(" << ibin << ") = " << mInTracksPt[i]->GetBinContent(ibin) << ", mMatchedTracksPt[" << i << "]->GetBinContent(" << ibin << ") = " << mMatchedTracksPt[i]->GetBinContent(ibin);
      if (mInTracksPt[i]->GetBinContent(ibin) < mMatchedTracksPt[i]->GetBinContent(ibin)) {
        LOG(ERROR) << "issue spotted: ibin " << ibin << ": mInTracksPt[" << i << "]->GetBinContent(" << ibin << ") = " << mInTracksPt[i]->GetBinContent(ibin) << ", mMatchedTracksPt[" << i << "]->GetBinContent(" << ibin << ") = " << mMatchedTracksPt[i]->GetBinContent(ibin);
      }
      if (mUseMC) {
        if (mMatchedTracksPt[i]->GetBinContent(ibin) < mFakeMatchedTracksPt[i]->GetBinContent(ibin)) {
          LOG(ERROR) << "issue spotted: ibin " << ibin << ": mMatchedTracksPt[" << i << "]->GetBinContent(" << ibin << ") = " << mMatchedTracksPt[i]->GetBinContent(ibin) << ", mFakeMatchedTracksPt[" << i << "]->GetBinContent(" << ibin << ") = " << mFakeMatchedTracksPt[i]->GetBinContent(ibin);
        }
      }
    }
    for (int ibin = 1; ibin <= mMatchedTracksEta[i]->GetNbinsX(); ++ibin) {
      LOG(DEBUG) << "check: ibin " << ibin << ": mInTracksEta[" << i << "]->GetBinContent(" << ibin << ") = " << mInTracksEta[i]->GetBinContent(ibin) << ", mMatchedTracksEta[" << i << "]->GetBinContent(" << ibin << ") = " << mMatchedTracksEta[i]->GetBinContent(ibin);
      if (mInTracksEta[i]->GetBinContent(ibin) < mMatchedTracksEta[i]->GetBinContent(ibin)) {
        LOG(ERROR) << "issue spotted: ibin " << ibin << ": mInTracksEta[" << i << "]->GetBinContent(" << ibin << ") = " << mInTracksEta[i]->GetBinContent(ibin) << ", mMatchedTracksEta[" << i << "]->GetBinContent(" << ibin << ") = " << mMatchedTracksEta[i]->GetBinContent(ibin);
      }
      if (mUseMC) {
        if (mMatchedTracksEta[i]->GetBinContent(ibin) < mFakeMatchedTracksEta[i]->GetBinContent(ibin)) {
          LOG(ERROR) << "issue spotted: ibin " << ibin << ": mMatchedTracksEta[" << i << "]->GetBinContent(" << ibin << ") = " << mMatchedTracksEta[i]->GetBinContent(ibin) << ", mFakeMatchedTracksEta[" << i << "]->GetBinContent(" << ibin << ") = " << mFakeMatchedTracksEta[i]->GetBinContent(ibin);
        }
      }
    }
  }
}

void TOFMatchedTracks::endOfCycle()
{

  ILOG(Info, Support) << "endOfCycle" << ENDM;

  // Logging in case any denominator has less entries than the corresponding numerator
  for (int i = 0; i < trkType::SIZE; ++i) {
    for (int ibin = 1; ibin <= mMatchedTracksPt[i]->GetNbinsX(); ++ibin) {
      if (mInTracksPt[i]->GetBinContent(ibin) < mMatchedTracksPt[i]->GetBinContent(ibin)) {
        LOG(ERROR) << "End Of Cycle issue spotted: ibin " << ibin << ": mInTracksPt[" << i << "]->GetBinContent(" << ibin << ") = " << mInTracksPt[i]->GetBinContent(ibin) << ", mMatchedTracksPt[" << i << "]->GetBinContent(" << ibin << ") = " << mMatchedTracksPt[i]->GetBinContent(ibin);
      }
      if (mUseMC) {
        if (mMatchedTracksPt[i]->GetBinContent(ibin) < mFakeMatchedTracksPt[i]->GetBinContent(ibin)) {
          LOG(ERROR) << "End Of Cycle issue spotted: ibin " << ibin << ": mMatchedTracksPt[" << i << "]->GetBinContent(" << ibin << ") = " << mMatchedTracksPt[i]->GetBinContent(ibin) << ", mFakeMatchedTracksPt[" << i << "]->GetBinContent(" << ibin << ") = " << mFakeMatchedTracksPt[i]->GetBinContent(ibin);
        }
      }
    }
    for (int ibin = 1; ibin <= mMatchedTracksEta[i]->GetNbinsX(); ++ibin) {
      if (mInTracksEta[i]->GetBinContent(ibin) < mMatchedTracksEta[i]->GetBinContent(ibin)) {
        LOG(ERROR) << "End Of Cycle issue spotted: ibin " << ibin << ": mInTracksEta[" << i << "]->GetBinContent(" << ibin << ") = " << mInTracksEta[i]->GetBinContent(ibin) << ", mMatchedTracksEta[" << i << "]->GetBinContent(" << ibin << ") = " << mMatchedTracksEta[i]->GetBinContent(ibin);
      }
      if (mUseMC) {
        if (mMatchedTracksEta[i]->GetBinContent(ibin) < mFakeMatchedTracksEta[i]->GetBinContent(ibin)) {
          LOG(ERROR) << "End Of Cycle issue spotted: ibin " << ibin << ": mMatchedTracksEta[" << i << "]->GetBinContent(" << ibin << ") = " << mMatchedTracksEta[i]->GetBinContent(ibin) << ", mFakeMatchedTracksEta[" << i << "]->GetBinContent(" << ibin << ") = " << mFakeMatchedTracksEta[i]->GetBinContent(ibin);
        }
      }
    }
  }

  if (mRecoCont.isTrackSourceLoaded(GID::TPCTOF)) {
    if (!mEffPt[trkType::UNCONS]->SetTotalHistogram(*mInTracksPt[trkType::UNCONS], "f") || // all total have to be forcely replaced, or the passed from previous processing may have incompatible number of entries
        !mEffPt[trkType::UNCONS]->SetPassedHistogram(*mMatchedTracksPt[trkType::UNCONS], "") ||
        !mEffEta[trkType::UNCONS]->SetTotalHistogram(*mInTracksEta[trkType::UNCONS], "f") ||
        !mEffEta[trkType::UNCONS]->SetPassedHistogram(*mMatchedTracksEta[trkType::UNCONS], "")) {
      ILOG(Fatal, Support) << "Something went wrong in defining the efficiency histograms, UNCONS!!";
    }
    if (mUseMC) {
      if (!mFakeFractionTracksPt[trkType::UNCONS]->SetTotalHistogram(*mMatchedTracksPt[trkType::UNCONS], "f") ||
          !mFakeFractionTracksPt[trkType::UNCONS]->SetPassedHistogram(*mFakeMatchedTracksPt[trkType::UNCONS], "") ||
          !mFakeFractionTracksEta[trkType::UNCONS]->SetTotalHistogram(*mMatchedTracksEta[trkType::UNCONS], "f") ||
          !mFakeFractionTracksEta[trkType::UNCONS]->SetPassedHistogram(*mFakeMatchedTracksEta[trkType::UNCONS], "")) {
        ILOG(Fatal, Support) << "Something went wrong in defining the efficiency histograms for MC, UNCONS!!";
      }
    }
  }
  if (mRecoCont.isTrackSourceLoaded(GID::ITSTPCTOF)) {
    if (!mEffPt[trkType::CONSTR]->SetTotalHistogram(*mInTracksPt[trkType::CONSTR], "f") ||
        !mEffPt[trkType::CONSTR]->SetPassedHistogram(*mMatchedTracksPt[trkType::CONSTR], "") ||
        !mEffEta[trkType::CONSTR]->SetTotalHistogram(*mInTracksEta[trkType::CONSTR], "f") ||
        !mEffEta[trkType::CONSTR]->SetPassedHistogram(*mMatchedTracksEta[trkType::CONSTR], "")) {
      ILOG(Fatal, Support) << "Something went wrong in defining the efficiency histograms, CONSTR!!";
    }
    if (mUseMC) {
      if (!mFakeFractionTracksPt[trkType::CONSTR]->SetTotalHistogram(*mMatchedTracksPt[trkType::CONSTR], "f") ||
          !mFakeFractionTracksPt[trkType::CONSTR]->SetPassedHistogram(*mFakeMatchedTracksPt[trkType::CONSTR], "") ||
          !mFakeFractionTracksEta[trkType::CONSTR]->SetTotalHistogram(*mMatchedTracksEta[trkType::CONSTR], "f") ||
          !mFakeFractionTracksEta[trkType::CONSTR]->SetPassedHistogram(*mFakeMatchedTracksEta[trkType::CONSTR], "")) {
        ILOG(Fatal, Support) << "Something went wrong in defining the efficiency histograms for MC, CONSTR!!";
      }
    }
  }

  // Printing, for checks
  if (mVerbose) {
    for (int i = 0; i < trkType::SIZE; ++i) {
      ILOG(Debug, Support) << "Type " << i << ENDM;
      ILOG(Debug, Support) << "Pt plot" << ENDM;
      for (int ibin = 0; ibin < mMatchedTracksPt[i]->GetNbinsX(); ++ibin) {
        ILOG(Debug, Support) << "mMatchedTracksPt[" << i << "]->GetBinContent(" << ibin + 1 << ") = " << mMatchedTracksPt[i]->GetBinContent(ibin + 1) << ", with error = " << mMatchedTracksPt[i]->GetBinError(ibin + 1);
        ILOG(Debug, Support) << ", mInTracksPt[" << i << "]->GetBinContent(" << ibin + 1 << ") = " << mInTracksPt[i]->GetBinContent(ibin + 1) << ", with error = " << mInTracksPt[i]->GetBinError(ibin + 1);
        ILOG(Debug, Support) << ", mEffPt[" << i << "]->GetEfficiency(" << ibin + 1 << ") = " << mEffPt[i]->GetEfficiency(ibin + 1) << ", with error low = " << mEffPt[i]->GetEfficiencyErrorLow(ibin + 1) << " and error up = " << mEffPt[i]->GetEfficiencyErrorUp(ibin + 1);
        if (mInTracksPt[i]->GetBinContent(ibin + 1) > 0) {
          ILOG(Debug, Support) << " (should be " << mMatchedTracksPt[i]->GetBinContent(ibin + 1) / mInTracksPt[i]->GetBinContent(ibin + 1) << ")" << ENDM;
        } else {
          ILOG(Debug, Support) << ENDM;
        }
      }
      ILOG(Debug, Support) << ENDM;
      ILOG(Debug, Support) << "Eta plot" << ENDM;
      for (int ibin = 0; ibin < mMatchedTracksEta[i]->GetNbinsX(); ++ibin) {
        ILOG(Debug, Support) << "mMatchedTracksEta[" << i << "]->GetBinContent(" << ibin + 1 << ") = " << mMatchedTracksEta[i]->GetBinContent(ibin + 1) << ", with error = " << mMatchedTracksEta[i]->GetBinError(ibin + 1);
        ILOG(Debug, Support) << ", mInTracksEta[" << i << "]->GetBinContent(" << ibin + 1 << ") = " << mInTracksEta[i]->GetBinContent(ibin + 1) << ", with error = " << mInTracksEta[i]->GetBinError(ibin + 1);
        ILOG(Debug, Support) << ", mEffEta[" << i << "]->GetEfficiency(" << ibin + 1 << ") = " << mEffEta[i]->GetEfficiency(ibin + 1) << ", with error low = " << mEffEta[i]->GetEfficiencyErrorLow(ibin + 1) << " and error up = " << mEffEta[i]->GetEfficiencyErrorUp(ibin + 1);
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
    mMatchedTracksPt[i]->Reset();
    mMatchedTracksEta[i]->Reset();
    if (mUseMC) {
      mFakeMatchedTracksPt[i]->Reset();
      mFakeMatchedTracksEta[i]->Reset();
    }
    mInTracksPt[i]->Reset();
    mInTracksEta[i]->Reset();
  }
}

//__________________________________________________________

bool TOFMatchedTracks::selectTrack(o2::tpc::TrackTPC const& track)
{

  if (track.getPt() < mPtCut) {
    return false;
  }
  if (std::abs(track.getEta()) > mEtaCut) {
    return false;
  }
  if (track.getNClusters() < mNTPCClustersCut) {
    return false;
  }

  math_utils::Point3D<float> v{};
  std::array<float, 2> dca;
  if (!(const_cast<o2::tpc::TrackTPC&>(track).propagateParamToDCA(v, mBz, &dca, mDCACut)) || std::abs(dca[0]) > mDCACutY) {
    return false;
  }

  return true;
}

} // namespace o2::quality_control_modules::tof
