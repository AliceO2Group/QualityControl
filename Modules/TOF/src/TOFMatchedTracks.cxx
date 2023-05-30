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
#include <TH2.h>
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

#include "TOF/Utils.h"
#include "TOFBase/Geo.h"
#include "DataFormatsTOF/Cluster.h"

using GTrackID = o2::dataformats::GlobalTrackID;

namespace o2::quality_control_modules::tof
{

TOFMatchedTracks::~TOFMatchedTracks()
{
  for (int i = 0; i < matchType::SIZE; ++i) {
    delete mMatchedTracksPt[i];
    delete mMatchedTracksEta[i];
    delete mMatchedTracks2DPtEta[i];
    if (mUseMC) {
      delete mFakeMatchedTracksPt[i];
      delete mFakeMatchedTracksEta[i];
    }
    delete mInTracksPt[i];
    delete mInTracksEta[i];
    delete mInTracks2DPtEta[i];
    delete mEffPt[i];
    delete mEffEta[i];
    delete mEff2DPtEta[i];
    delete mDeltaZEta[i];
    delete mDeltaZPhi[i];
    delete mDeltaZPt[i];
    delete mDeltaXEta[i];
    delete mDeltaXPhi[i];
    delete mDeltaXPt[i];
    delete mTOFChi2[i];
    delete mTOFChi2Pt[i];
  }
  for (int isec = 0; isec < 18; isec++) {
    delete mDTimeTrk[isec];
    delete mDTimeTrkTPC[isec];
    delete mDTimeTrkTRD[isec];
  }
}

void TOFMatchedTracks::initialize(o2::framework::InitContext& /*ctx*/)
{
  ILOG(Debug, Devel) << "initialize TOFMatchedTracks" << ENDM; // QcInfoLogger is used. FairMQ logs will go to there as well.

  // this is how to get access to custom parameters defined in the config file at qc.tasks.<task_name>.taskParameters

  utils::parseBooleanParameter(mCustomParameters, "isMC", mUseMC);
  utils::parseBooleanParameter(mCustomParameters, "verbose", mVerbose);

  // for track selection
  if (auto param = mCustomParameters.find("minPtCut"); param != mCustomParameters.end()) {
    ILOG(Debug, Devel) << "Custom parameter - minPtCut (for track selection): " << param->second << ENDM;
    setPtCut(atof(param->second.c_str()));
  }
  if (auto param = mCustomParameters.find("etaCut"); param != mCustomParameters.end()) {
    ILOG(Debug, Devel) << "Custom parameter - etaCut (for track selection): " << param->second << ENDM;
    setEtaCut(atof(param->second.c_str()));
  }
  if (auto param = mCustomParameters.find("minNTPCClustersCut"); param != mCustomParameters.end()) {
    ILOG(Debug, Devel) << "Custom parameter - minNTPCClustersCut (for track selection): " << param->second << ENDM;
    setMinNTPCClustersCut(atoi(param->second.c_str()));
  }
  if (auto param = mCustomParameters.find("minDCACut"); param != mCustomParameters.end()) {
    ILOG(Debug, Devel) << "Custom parameter - minDCACut (for track selection): " << param->second << ENDM;
    setMinDCAtoBeamPipeDistanceCut(atof(param->second.c_str()));
  }
  if (auto param = mCustomParameters.find("minDCACutY"); param != mCustomParameters.end()) {
    ILOG(Debug, Devel) << "Custom parameter - minDCACutY (for track selection): " << param->second << ENDM;
    setMinDCAtoBeamPipeYCut(atof(param->second.c_str()));
  }

  // for track type selection
  if (auto param = mCustomParameters.find("GID"); param != mCustomParameters.end()) {
    ILOG(Debug, Devel) << "Custom parameter - GID (= sources by user): " << param->second << ENDM;
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

  if ((mSrc[GID::Source::TPCTRDTOF] == 1 && mSrc[GID::Source::TPCTRD] == 0) || (mSrc[GID::Source::TPCTRDTOF] == 0 && mSrc[GID::Source::TPCTRD] == 1)) {
    ILOG(Fatal, Support) << "Check the requested sources: TPCTRDTOF = " << mSrc[GID::Source::TPCTRDTOF] << ", TPCTRD = " << mSrc[GID::Source::TPCTRD] << ENDM;
  }

  if ((mSrc[GID::Source::ITSTPCTRDTOF] == 1 && mSrc[GID::Source::ITSTPCTRD] == 0) || (mSrc[GID::Source::ITSTPCTRDTOF] == 0 && mSrc[GID::Source::ITSTPCTRD] == 1)) {
    ILOG(Fatal, Support) << "Check the requested sources: ITSTPCTRDTOF = " << mSrc[GID::Source::ITSTPCTRDTOF] << ", ITSTPCTRD = " << mSrc[GID::Source::ITSTPCTRD] << ENDM;
  }

  std::array<std::string, 3> title{ "TPC", "ITSTPC-ITSTPCTRD", "TPCTRD" };
  for (int i = 0; i < matchType::SIZE; ++i) {
    mInTracksPt[i] = new TH1F(Form("mInTracksPt_%s", title[i].c_str()), Form("mInTracksPt (matchType: %s); #it{p}_{T}; counts", title[i].c_str()), 100, 0.f, 20.f);
    mInTracksEta[i] = new TH1F(Form("mInTracksEta_%s", title[i].c_str()), Form("mInTracksEta (matchType: %s); #eta; counts", title[i].c_str()), 100, -1.0f, 1.0f);
    mInTracks2DPtEta[i] = new TH2F(Form("mInTracks2DPtEta_%s", title[i].c_str()), Form("mInTracks2DPtEta (matchType: %s); #it{p}_{T}; #eta; counts", title[i].c_str()), 100, 0.f, 20.f, 100, -1.0f, 1.0f);
    mMatchedTracksPt[i] = new TH1F(Form("mMatchedTracksPt_%s", title[i].c_str()), Form("mMatchedTracksPt (matchType: %s); #it{p}_{T}; counts", title[i].c_str()), 100, 0.f, 20.f);
    mMatchedTracksEta[i] = new TH1F(Form("mMatchedTracksEta_%s", title[i].c_str()), Form("mMatchedTracksEta (matchType: %s); #eta; counts", title[i].c_str()), 100, -1.0f, 1.0f);
    mMatchedTracks2DPtEta[i] = new TH2F(Form("mMatchedTracks2DPtEta_%s", title[i].c_str()), Form("mMatchedTracks2DPtEta (matchType: %s); #it{p}_{T}; #eta; counts", title[i].c_str()), 100, 0.f, 20.f, 100, -1.0f, 1.0f);
    if (mUseMC) {
      mFakeMatchedTracksPt[i] = new TH1F(Form("mFakeMatchedTracksPt_%s", title[i].c_str()), Form("mFakeMatchedTracksPt (matchType: %s); #it{p}_{T}; counts", title[i].c_str()), 100, 0.f, 20.f);
      mFakeMatchedTracksEta[i] = new TH1F(Form("mFakeMatchedTracksEta_%s", title[i].c_str()), Form("mFakeMatchedTracksEta (matchType: %s); #eta; counts", title[i].c_str()), 100, -1.0f, 1.0f);
      mFakeFractionTracksPt[i] = new TEfficiency(Form("mFakeFractionPt_%s", title[i].c_str()), Form("Fraction of fake matches vs Pt (matchType: %s); #it{p}_{T}; Eff", title[i].c_str()), 100, 0.f, 20.f);
      mFakeFractionTracksEta[i] = new TEfficiency(Form("mFakeFractionEta_%s", title[i].c_str()), Form("Fraction of fake matches vs Eta (matchType: %s); #eta; Eff", title[i].c_str()), 100, -1.0f, 1.0f);
    }
    mEffPt[i] = new TEfficiency(Form("mEffPt_%s", title[i].c_str()), Form("Efficiency vs Pt (matchType: %s); #it{p}_{T}; Eff", title[i].c_str()), 100, 0.f, 20.f);
    mEffEta[i] = new TEfficiency(Form("mEffEta_%s", title[i].c_str()), Form("Efficiency vs Eta (matchType: %s); #eta; Eff", title[i].c_str()), 100, -1.f, 1.f);
    mEff2DPtEta[i] = new TEfficiency(Form("mEff2DPtEta_%s", title[i].c_str()), Form("Efficiency vs Pt vs Eta (matchType: %s); #it{p}_{T}; #eta; Eff", title[i].c_str()), 100, 0.f, 20.f, 100, -1.0f, 1.0f);
    mDeltaZEta[i] = new TH2F(Form("mDeltaZEta%s", title[i].c_str()), Form("mDeltaZEta (matchType: %s); #eta; #Delta z (cm); counts", title[i].c_str()), 100, -1.0f, 1.0f, 100, -10.f, 10.f);
    mDeltaZPhi[i] = new TH2F(Form("mDeltaZPhi%s", title[i].c_str()), Form("mDeltaZPhi (matchType: %s); #phi; #Delta z (cm); counts", title[i].c_str()), 100, .0f, 6.3f, 100, -10.f, 10.f);
    mDeltaZPt[i] = new TH2F(Form("mDeltaZPt%s", title[i].c_str()), Form("mDeltaZPt (matchType: %s); #it{p}_{T}; #Delta z (cm); counts", title[i].c_str()), 100, 0.f, 20.f, 100, -10.f, 10.f);
    mDeltaXEta[i] = new TH2F(Form("mDeltaXEta%s", title[i].c_str()), Form("mDeltaXEta (matchType: %s); #eta; #Delta x (cm); counts", title[i].c_str()), 100, -1.0f, 1.0f, 100, -10.f, 10.f);
    mDeltaXPhi[i] = new TH2F(Form("mDeltaXPhi%s", title[i].c_str()), Form("mDeltaXPhi (matchType: %s); #phi; #Delta x (cm); counts", title[i].c_str()), 100, .0f, 6.3f, 100, -10.f, 10.f);
    mDeltaXPt[i] = new TH2F(Form("mDeltaXPt%s", title[i].c_str()), Form("mDeltaXPt (matchType: %s); #it{p}_{T}; #Delta z (cm); counts", title[i].c_str()), 100, 0.f, 20.f, 100, -10.f, 10.f);
    mTOFChi2[i] = new TH1F(Form("mTOFChi2%s", title[i].c_str()), Form("mTOFChi2 (matchType: %s); #chi^{2}; counts", title[i].c_str()), 100, 0.f, 10.f);
    mTOFChi2Pt[i] = new TH2F(Form("mTOFChi2Pt%s", title[i].c_str()), Form("mTOFChi2Pt (matchType: %s); #it{p}_{T}; #chi^{2}; counts", title[i].c_str()), 100, 0.f, 20.f, 100, 0.f, 10.f);
  }

  for (int isec = 0; isec < 18; isec++) {
    mDTimeTrk[isec] = new TH2F(Form("DTimeTrk_sec%02d", isec), Form("Sector %d: ITS-TPC track-tof #Deltat vs #eta; #eta; #Deltat (# BC)", isec), 50, -1.0f, 1.0f, 500, -200, 200);
    getObjectsManager()->startPublishing(mDTimeTrk[isec]);
    mDTimeTrkTPC[isec] = new TH2F(Form("DTimeTrkTPC_sec%02d", isec), Form("Sector %d: TPC track-tof #Deltat vs #eta; #eta; #Deltat (# BC)", isec), 50, -1.0f, 1.0f, 500, -200, 200);
    if ((mSrc & o2::dataformats::GlobalTrackID::getSourcesMask("TPC")).any()) {
      getObjectsManager()->startPublishing(mDTimeTrkTPC[isec]);
    }
    mDTimeTrkTRD[isec] = new TH2F(Form("DTimeTrkTRD_sec%02d", isec), Form("Sector %d: ITS-TPC-TRD track-tof #Deltat vs #eta; #eta; #Deltat (# BC)", isec), 100, -1.0f, 1.0f, 200, -5, 5);
    if ((mSrc & o2::dataformats::GlobalTrackID::getSourcesMask("ITS-TPC-TRD")).any()) {
      getObjectsManager()->startPublishing(mDTimeTrkTRD[isec]);
    }
  }

  if (mSrc[GID::Source::TPCTOF] == 1) {
    getObjectsManager()->startPublishing(mInTracksPt[matchType::TPC]);
    getObjectsManager()->startPublishing(mInTracksEta[matchType::TPC]);
    getObjectsManager()->startPublishing(mInTracks2DPtEta[matchType::TPC]);
    getObjectsManager()->startPublishing(mMatchedTracksPt[matchType::TPC]);
    getObjectsManager()->startPublishing(mMatchedTracksEta[matchType::TPC]);
    getObjectsManager()->startPublishing(mMatchedTracks2DPtEta[matchType::TPC]);
    if (mUseMC) {
      getObjectsManager()->startPublishing(mFakeMatchedTracksPt[matchType::TPC]);
      getObjectsManager()->startPublishing(mFakeMatchedTracksEta[matchType::TPC]);
      getObjectsManager()->startPublishing(mFakeFractionTracksPt[matchType::TPC]);
      getObjectsManager()->startPublishing(mFakeFractionTracksEta[matchType::TPC]);
    }
    getObjectsManager()->startPublishing(mEffPt[matchType::TPC]);
    getObjectsManager()->startPublishing(mEffEta[matchType::TPC]);
    getObjectsManager()->startPublishing(mEff2DPtEta[matchType::TPC]);
    getObjectsManager()->startPublishing(mDeltaZEta[matchType::TPC]);
    getObjectsManager()->startPublishing(mDeltaZPhi[matchType::TPC]);
    getObjectsManager()->startPublishing(mDeltaZPt[matchType::TPC]);
    getObjectsManager()->startPublishing(mDeltaXEta[matchType::TPC]);
    getObjectsManager()->startPublishing(mDeltaXPhi[matchType::TPC]);
    getObjectsManager()->startPublishing(mDeltaXPt[matchType::TPC]);
    getObjectsManager()->startPublishing(mTOFChi2[matchType::TPC]);
    getObjectsManager()->startPublishing(mTOFChi2Pt[matchType::TPC]);
  }

  if (mSrc[GID::Source::TPCTRDTOF] == 1) {
    getObjectsManager()->startPublishing(mInTracksPt[matchType::TPCTRD]);
    getObjectsManager()->startPublishing(mInTracksEta[matchType::TPCTRD]);
    getObjectsManager()->startPublishing(mInTracks2DPtEta[matchType::TPCTRD]);
    getObjectsManager()->startPublishing(mMatchedTracksPt[matchType::TPCTRD]);
    getObjectsManager()->startPublishing(mMatchedTracksEta[matchType::TPCTRD]);
    getObjectsManager()->startPublishing(mMatchedTracks2DPtEta[matchType::TPCTRD]);
    if (mUseMC) {
      getObjectsManager()->startPublishing(mFakeMatchedTracksPt[matchType::TPCTRD]);
      getObjectsManager()->startPublishing(mFakeMatchedTracksEta[matchType::TPCTRD]);
      getObjectsManager()->startPublishing(mFakeFractionTracksPt[matchType::TPCTRD]);
      getObjectsManager()->startPublishing(mFakeFractionTracksEta[matchType::TPCTRD]);
    }
    getObjectsManager()->startPublishing(mEffPt[matchType::TPCTRD]);
    getObjectsManager()->startPublishing(mEffEta[matchType::TPCTRD]);
    getObjectsManager()->startPublishing(mEff2DPtEta[matchType::TPCTRD]);
    getObjectsManager()->startPublishing(mDeltaZEta[matchType::TPCTRD]);
    getObjectsManager()->startPublishing(mDeltaZPhi[matchType::TPCTRD]);
    getObjectsManager()->startPublishing(mDeltaZPt[matchType::TPCTRD]);
    getObjectsManager()->startPublishing(mDeltaXEta[matchType::TPCTRD]);
    getObjectsManager()->startPublishing(mDeltaXPhi[matchType::TPCTRD]);
    getObjectsManager()->startPublishing(mDeltaXPt[matchType::TPCTRD]);
    getObjectsManager()->startPublishing(mTOFChi2[matchType::TPCTRD]);
    getObjectsManager()->startPublishing(mTOFChi2Pt[matchType::TPCTRD]);
  }

  if (mSrc[GID::Source::ITSTPCTOF] == 1 || mSrc[GID::Source::ITSTPCTRDTOF] == 1) {
    getObjectsManager()->startPublishing(mInTracksPt[matchType::ITSTPC_ITSTPCTRD]);
    getObjectsManager()->startPublishing(mInTracksEta[matchType::ITSTPC_ITSTPCTRD]);
    getObjectsManager()->startPublishing(mInTracks2DPtEta[matchType::ITSTPC_ITSTPCTRD]);
    getObjectsManager()->startPublishing(mMatchedTracksPt[matchType::ITSTPC_ITSTPCTRD]);
    getObjectsManager()->startPublishing(mMatchedTracksEta[matchType::ITSTPC_ITSTPCTRD]);
    getObjectsManager()->startPublishing(mMatchedTracks2DPtEta[matchType::ITSTPC_ITSTPCTRD]);
    if (mUseMC) {
      getObjectsManager()->startPublishing(mFakeMatchedTracksPt[matchType::ITSTPC_ITSTPCTRD]);
      getObjectsManager()->startPublishing(mFakeMatchedTracksEta[matchType::ITSTPC_ITSTPCTRD]);
      getObjectsManager()->startPublishing(mFakeFractionTracksPt[matchType::ITSTPC_ITSTPCTRD]);
      getObjectsManager()->startPublishing(mFakeFractionTracksEta[matchType::ITSTPC_ITSTPCTRD]);
    }
    getObjectsManager()->startPublishing(mEffPt[matchType::ITSTPC_ITSTPCTRD]);
    getObjectsManager()->startPublishing(mEffEta[matchType::ITSTPC_ITSTPCTRD]);
    getObjectsManager()->startPublishing(mEff2DPtEta[matchType::ITSTPC_ITSTPCTRD]);
    getObjectsManager()->startPublishing(mDeltaZEta[matchType::ITSTPC_ITSTPCTRD]);
    getObjectsManager()->startPublishing(mDeltaZPhi[matchType::ITSTPC_ITSTPCTRD]);
    getObjectsManager()->startPublishing(mDeltaZPt[matchType::ITSTPC_ITSTPCTRD]);
    getObjectsManager()->startPublishing(mDeltaXEta[matchType::ITSTPC_ITSTPCTRD]);
    getObjectsManager()->startPublishing(mDeltaXPhi[matchType::ITSTPC_ITSTPCTRD]);
    getObjectsManager()->startPublishing(mDeltaXPt[matchType::ITSTPC_ITSTPCTRD]);
    getObjectsManager()->startPublishing(mTOFChi2[matchType::ITSTPC_ITSTPCTRD]);
    getObjectsManager()->startPublishing(mTOFChi2Pt[matchType::ITSTPC_ITSTPCTRD]);
  }
}

void TOFMatchedTracks::startOfActivity(const Activity& activity)
{
  ILOG(Debug, Devel) << "startOfActivity " << activity.mId << ENDM;
  reset();
}

void TOFMatchedTracks::startOfCycle()
{
  ILOG(Debug, Devel) << "startOfCycle" << ENDM;
}

void TOFMatchedTracks::monitorData(o2::framework::ProcessingContext& ctx)
{

  ++mTF;
  LOG(debug) << " ************************ ";
  LOG(debug) << " *** Processing TF " << mTF << " *** ";
  LOG(debug) << " ************************ ";
  mRecoCont.collectData(ctx, *mDataRequest.get());

  // Getting the B field
  mBz = o2::base::Propagator::Instance()->getNominalBz();

  // TOF
  gsl::span<const o2::tof::Cluster> tofClusArray = mRecoCont.getTOFClusters();

  // TPC
  if (mRecoCont.isTrackSourceLoaded(GID::TPC) || mRecoCont.isTrackSourceLoaded(GID::TPCTOF)) { // this is enough to know that also TPC was loades, see "initialize"
    mTPCTracks = mRecoCont.getTPCTracks();
  }

  // TPC-TOF
  if (mRecoCont.isTrackSourceLoaded(GID::TPCTOF)) { // this is enough to know that also TPC was loades, see "initialize"
    mTPCTOFMatches = mRecoCont.getTPCTOFMatches();
    LOG(debug) << "We found " << mTPCTracks.size() << " TPC-only tracks";
    LOG(debug) << "We found " << mRecoCont.getTPCTOFTracks().size() << " TPC-TOF tracks";
    LOG(debug) << "We found " << mTPCTOFMatches.size() << " TPC-only tracks matched to TOF";
    if (mRecoCont.getTPCTOFTracks().size() != mTPCTOFMatches.size()) {
      ILOG(Fatal, Support) << "Number of TPCTOF tracks (" << mRecoCont.getTPCTOFTracks().size() << ") differs from number of TPCTOF matches (" << mTPCTOFMatches.size() << ")" << ENDM;
    }
    // loop over TOF MatchInfo
    for (const auto& matchTOF : mTPCTOFMatches) {
      GTrackID gTrackId = matchTOF.getTrackRef();
      const auto& trk = mTPCTracks[gTrackId.getIndex()];
      const auto& trkDz = matchTOF.getDZatTOF();
      const auto& trkDx = matchTOF.getDXatTOF();
      const auto& trkchi2 = matchTOF.getChi2();
      if (!selectTrack(trk)) {
        LOG(debug) << "NUM UNCONS: track with eta " << trk.getEta() << " and pt " << trk.getPt() << " DISCARDED for numerator, UNCONS";
        continue;
      }
      LOG(debug) << "NUM UNCONS: track with eta " << trk.getEta() << " and pt " << trk.getPt() << " ACCEPTED for numerator, UNCONS";
      mMatchedTracksPt[matchType::TPC]->Fill(trk.getPt());
      mMatchedTracksEta[matchType::TPC]->Fill(trk.getEta());
      mMatchedTracks2DPtEta[matchType::TPC]->Fill(trk.getPt(), trk.getEta());
      mDeltaZEta[matchType::TPC]->Fill(trk.getEta(), trkDz);
      mDeltaZPhi[matchType::TPC]->Fill(trk.getPhi(), trkDz);
      mDeltaZPt[matchType::TPC]->Fill(trk.getPt(), trkDz);
      mDeltaXEta[matchType::TPC]->Fill(trk.getEta(), trkDx);
      mDeltaXPhi[matchType::TPC]->Fill(trk.getPhi(), trkDx);
      mDeltaXPt[matchType::TPC]->Fill(trk.getPt(), trkDx);
      mTOFChi2[matchType::TPC]->Fill(trkchi2);
      mTOFChi2Pt[matchType::TPC]->Fill(trk.getPt(), trkchi2);

      if (trk.getPt() > 1.0) {
        const double bcTimeInvInMus = o2::tof::Geo::BC_TIME_INV * 1E3;
        float deltaTrackTimeInBC = -matchTOF.getDeltaT() * bcTimeInvInMus; // track time - tof time in number of BC
        auto& tofCl = tofClusArray[matchTOF.getTOFClIndex()];
        int isec = tofCl.getMainContributingChannel() / 8736;
        if (isec >= 0 && isec < 18) {
          mDTimeTrkTPC[isec]->Fill(trk.getEta(), deltaTrackTimeInBC);
        }
      }

      if (mUseMC) {
        auto lbl = mRecoCont.getTrackMCLabel(gTrackId);
        if (lbl.isFake()) {
          mFakeMatchedTracksPt[matchType::TPC]->Fill(trk.getPt());
          mFakeMatchedTracksEta[matchType::TPC]->Fill(trk.getEta());
        }
      }
    }
  }

  // ITS-TPC-TOF
  if (mRecoCont.isTrackSourceLoaded(GID::ITSTPCTOF)) { // this is enough to know that also ITSTPC was loades, see "initialize"
    mITSTPCTracks = mRecoCont.getTPCITSTracks();
    mITSTPCTOFMatches = mRecoCont.getITSTPCTOFMatches();
    LOG(debug) << "We found " << mITSTPCTracks.size() << " ITS-TPC tracks";
    LOG(debug) << "We found " << mITSTPCTOFMatches.size() << " ITS-TPC tracks matched to TOF";
    // loop over TOF MatchInfo
    for (const auto& matchTOF : mITSTPCTOFMatches) {
      GTrackID gTrackId = matchTOF.getTrackRef();
      const auto& trk = mITSTPCTracks[gTrackId.getIndex()];
      const auto& trkTPC = mTPCTracks[trk.getRefTPC()];
      const auto& trkDz = matchTOF.getDZatTOF();
      const auto& trkDx = matchTOF.getDXatTOF();
      const auto& trkchi2 = matchTOF.getChi2();
      if (!selectTrack(trkTPC)) {
        LOG(debug) << "NUM ITSTPC CONSTR: track with eta " << trkTPC.getEta() << " and pT " << trkTPC.getPt() << " DISCARDED for numerator, ITSTPC CONSTR";
        continue;
      }
      LOG(debug) << "NUM ITSTPC CONSTR: track with eta " << trkTPC.getEta() << " and pT " << trkTPC.getPt() << " ACCEPTED for numerator, ITSTPC CONSTR"
                 << " gid: " << gTrackId << " TPC gid =" << trk.getRefTPC();
      mMatchedTracksPt[matchType::ITSTPC_ITSTPCTRD]->Fill(trkTPC.getPt());

      if (trkTPC.getPt() > 1.0) {
        const double bcTimeInvInMus = o2::tof::Geo::BC_TIME_INV * 1E3;
        float deltaTrackTimeInBC = -matchTOF.getDeltaT() * bcTimeInvInMus; // track time - tof time in number of BC
        auto& tofCl = tofClusArray[matchTOF.getTOFClIndex()];
        int isec = tofCl.getMainContributingChannel() / 8736;
        if (isec >= 0 && isec < 18) {
          mDTimeTrk[isec]->Fill(trkTPC.getEta(), deltaTrackTimeInBC);
        }
      }

      mMatchedTracksEta[matchType::ITSTPC_ITSTPCTRD]->Fill(trkTPC.getEta());
      mMatchedTracks2DPtEta[matchType::ITSTPC_ITSTPCTRD]->Fill(trkTPC.getPt(), trkTPC.getEta());
      mDeltaZEta[matchType::ITSTPC_ITSTPCTRD]->Fill(trkTPC.getEta(), trkDz);
      mDeltaZPhi[matchType::ITSTPC_ITSTPCTRD]->Fill(trkTPC.getPhi(), trkDz);
      mDeltaZPt[matchType::ITSTPC_ITSTPCTRD]->Fill(trkTPC.getPt(), trkDz);
      mDeltaXEta[matchType::ITSTPC_ITSTPCTRD]->Fill(trkTPC.getEta(), trkDx);
      mDeltaXPhi[matchType::ITSTPC_ITSTPCTRD]->Fill(trkTPC.getPhi(), trkDx);
      mDeltaXPt[matchType::ITSTPC_ITSTPCTRD]->Fill(trkTPC.getPt(), trkDx);
      mTOFChi2[matchType::ITSTPC_ITSTPCTRD]->Fill(trkchi2);
      mTOFChi2Pt[matchType::ITSTPC_ITSTPCTRD]->Fill(trkTPC.getPt(), trkchi2);
      if (mUseMC) {
        auto lbl = mRecoCont.getTrackMCLabel(gTrackId);
        if (lbl.isFake()) {
          mFakeMatchedTracksPt[matchType::ITSTPC_ITSTPCTRD]->Fill(trkTPC.getPt());
          mFakeMatchedTracksEta[matchType::ITSTPC_ITSTPCTRD]->Fill(trkTPC.getEta());
        }
      }
    }
  }

  // TPC-TRD-TOF
  if (mRecoCont.isTrackSourceLoaded(GID::TPCTRDTOF)) { // this is enough to know that also TPCTRD was loades, see "initialize"
    mTPCTRDTracks = mRecoCont.getTPCTRDTracks<o2::trd::TrackTRD>();
    mTPCTRDTOFMatches = mRecoCont.getTPCTRDTOFMatches();
    LOG(debug) << "We found " << mTPCTRDTracks.size() << " TPC-TRD tracks";
    LOG(debug) << "We found " << mTPCTRDTOFMatches.size() << " TPC-TRD tracks matched to TOF";
    // loop over TOF MatchInfo
    for (const auto& matchTOF : mTPCTRDTOFMatches) {
      GTrackID gTrackId = matchTOF.getTrackRef();
      const auto& trktpctrd = mTPCTRDTracks[gTrackId.getIndex()];
      const auto& trk = mTPCTracks[trktpctrd.getRefGlobalTrackId()];
      const auto& trkDz = matchTOF.getDZatTOF();
      const auto& trkDx = matchTOF.getDXatTOF();
      const auto& trkchi2 = matchTOF.getChi2();
      if (!selectTrack(trk)) {
        LOG(debug) << "NUM TPCTRD CONSTR: track with eta " << trk.getEta() << " and pt " << trk.getPt() << " DISCARDED for numerator, TPCTRD CONSTR";
        continue;
      }
      LOG(debug) << "NUM TPCTRD CONSTR: track with eta " << trk.getEta() << " and pt " << trk.getPt() << " ACCEPTED for numerator, TPCTRD CONSTR";
      mMatchedTracksPt[matchType::TPCTRD]->Fill(trk.getPt());
      mMatchedTracksEta[matchType::TPCTRD]->Fill(trk.getEta());
      mMatchedTracks2DPtEta[matchType::TPCTRD]->Fill(trk.getPt(), trk.getEta());
      mDeltaZEta[matchType::TPCTRD]->Fill(trk.getEta(), trkDz);
      mDeltaZPhi[matchType::TPCTRD]->Fill(trk.getPhi(), trkDz);
      mDeltaZPt[matchType::TPCTRD]->Fill(trk.getPt(), trkDz);
      mDeltaXEta[matchType::TPCTRD]->Fill(trk.getEta(), trkDx);
      mDeltaXPhi[matchType::TPCTRD]->Fill(trk.getPhi(), trkDx);
      mDeltaXPt[matchType::TPCTRD]->Fill(trk.getPt(), trkDx);
      mTOFChi2[matchType::TPCTRD]->Fill(trkchi2);
      mTOFChi2Pt[matchType::TPCTRD]->Fill(trk.getPt(), trkchi2);
      if (mUseMC) {
        auto lbl = mRecoCont.getTrackMCLabel(gTrackId);
        if (lbl.isFake()) {
          mFakeMatchedTracksPt[matchType::TPCTRD]->Fill(trk.getPt());
          mFakeMatchedTracksEta[matchType::TPCTRD]->Fill(trk.getEta());
        }
      }
    }
  }

  // ITS-TPC-TRD-TOF
  if (mRecoCont.isTrackSourceLoaded(GID::ITSTPCTRDTOF)) { // this is enough to know that also ITSTPC was loades, see "initialize"
    mITSTPCTRDTracks = mRecoCont.getITSTPCTRDTracks<o2::trd::TrackTRD>();
    mITSTPCTRDTOFMatches = mRecoCont.getITSTPCTRDTOFMatches();
    LOG(debug) << "We found " << mITSTPCTRDTracks.size() << " ITS-TPC-TRD tracks";
    LOG(debug) << "We found " << mITSTPCTRDTOFMatches.size() << " ITS-TPC-TRD tracks matched to TOF";
    // loop over TOF MatchInfo
    for (const auto& matchTOF : mITSTPCTRDTOFMatches) {
      GTrackID gTrackId = matchTOF.getTrackRef();
      const auto& trk = mITSTPCTRDTracks[gTrackId.getIndex()];
      const auto& trkITSTPC = mITSTPCTracks[trk.getRefGlobalTrackId()];
      const auto& trkTPC = mTPCTracks[trkITSTPC.getRefTPC()]; // check!
      const auto& trkDz = matchTOF.getDZatTOF();
      const auto& trkDx = matchTOF.getDXatTOF();
      const auto& trkchi2 = matchTOF.getChi2();
      if (!selectTrack(trkTPC)) {
        LOG(debug) << "NUM ITSTPCTRD CONSTR: track with eta " << trkTPC.getEta() << " and pT " << trkTPC.getPt() << " DISCARDED for numerator, ITSTPCTRD CONSTR";
        continue;
      }
      LOG(debug) << "NUM ITSTPCTRD CONSTR: track with eta " << trkTPC.getEta() << " and pT " << trkTPC.getPt() << " ACCEPTED for numerator, ITSTPCTRD CONSTR"
                 << " gid: " << gTrackId << " TPC gid =" << trkITSTPC.getRefTPC();
      mMatchedTracksPt[matchType::ITSTPC_ITSTPCTRD]->Fill(trkTPC.getPt());
      mMatchedTracksEta[matchType::ITSTPC_ITSTPCTRD]->Fill(trkTPC.getEta());
      mMatchedTracks2DPtEta[matchType::ITSTPC_ITSTPCTRD]->Fill(trkTPC.getPt(), trkTPC.getEta());
      mDeltaZEta[matchType::ITSTPC_ITSTPCTRD]->Fill(trkTPC.getEta(), trkDz);
      mDeltaZPhi[matchType::ITSTPC_ITSTPCTRD]->Fill(trkTPC.getPhi(), trkDz);
      mDeltaZPt[matchType::ITSTPC_ITSTPCTRD]->Fill(trkTPC.getPt(), trkDz);
      mDeltaXEta[matchType::ITSTPC_ITSTPCTRD]->Fill(trkTPC.getEta(), trkDx);
      mDeltaXPhi[matchType::ITSTPC_ITSTPCTRD]->Fill(trkTPC.getPhi(), trkDx);
      mDeltaXPt[matchType::ITSTPC_ITSTPCTRD]->Fill(trkTPC.getPt(), trkDx);
      mTOFChi2[matchType::ITSTPC_ITSTPCTRD]->Fill(trkchi2);
      mTOFChi2Pt[matchType::ITSTPC_ITSTPCTRD]->Fill(trkTPC.getPt(), trkchi2);

      if (trkTPC.getPt() > 1.0) {
        const double bcTimeInvInMus = o2::tof::Geo::BC_TIME_INV * 1E3;
        float deltaTrackTimeInBC = -matchTOF.getDeltaT() * bcTimeInvInMus; // track time - tof time in number of BC
        auto& tofCl = tofClusArray[matchTOF.getTOFClIndex()];
        int isec = tofCl.getMainContributingChannel() / 8736;
        if (isec >= 0 && isec < 18) {
          mDTimeTrkTRD[isec]->Fill(trkTPC.getEta(), deltaTrackTimeInBC);
        }
      }

      if (mUseMC) {
        auto lbl = mRecoCont.getTrackMCLabel(gTrackId);
        if (lbl.isFake()) {
          mFakeMatchedTracksPt[matchType::ITSTPC_ITSTPCTRD]->Fill(trkTPC.getPt());
          mFakeMatchedTracksEta[matchType::ITSTPC_ITSTPCTRD]->Fill(trkTPC.getEta());
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
          LOG(debug) << "DEN UNCONS: track with eta " << trk.getEta() << " and pT " << trk.getPt() << " DISCARDED for denominator UNCONS, TPC track";
          return true;
        }
        LOG(debug) << "DEN UNCONS: track with eta " << trk.getEta() << " and pT " << trk.getPt() << " ACCEPTED for denominator UNCONS, TPC track"
                   << " gid: " << gid;
        this->mInTracksPt[matchType::TPC]->Fill(trk.getPt());
        this->mInTracksEta[matchType::TPC]->Fill(trk.getEta());
        this->mInTracks2DPtEta[matchType::TPC]->Fill(trk.getPt(), trk.getEta());
      } else if constexpr (isTPCTOFTrack<decltype(trk)>()) {
        const auto& tpcTrack = mTPCTracks[mTPCTOFMatches[trk.getRefMatch()].getTrackRef().getIndex()];
        if (!selectTrack(tpcTrack)) {
          LOG(debug) << "DEN UNCONS: track with eta " << tpcTrack.getEta() << " and pT " << tpcTrack.getPt() << " DISCARDED for denominator UNCONS, TPCTOF track";
          return true;
        }
        LOG(debug) << "DEN UNCONS: track with eta " << tpcTrack.getEta() << " and pT " << tpcTrack.getPt() << " ACCEPTED for denominator UNCONS, TPCTOF track";
        this->mInTracksPt[matchType::TPC]->Fill(tpcTrack.getPt());
        this->mInTracksEta[matchType::TPC]->Fill(tpcTrack.getEta());
        this->mInTracks2DPtEta[matchType::TPC]->Fill(tpcTrack.getPt(), tpcTrack.getEta());
      }
    }
    // In case of ITS-TPC-TOF, the ITS-TPC tracks contain also the ITS-TPC-TOF
    if constexpr (isTPCITSTrack<decltype(trk)>()) {
      const auto& tpcTrack = mTPCTracks[trk.getRefTPC().getIndex()];
      if (!selectTrack(tpcTrack)) {
        LOG(debug) << "DEN CONSTR: track with eta " << tpcTrack.getEta() << " and pT " << tpcTrack.getPt() << " DISCARDED for denominator CONSTR, ITSTPC track";
        return true;
      }
      LOG(debug) << "DEN CONSTR: track with eta " << tpcTrack.getEta() << " and pT " << tpcTrack.getPt() << " ACCEPTED for denominator CONSTR, ITSTPC track";
      this->mInTracksPt[matchType::ITSTPC_ITSTPCTRD]->Fill(tpcTrack.getPt());
      this->mInTracksEta[matchType::ITSTPC_ITSTPCTRD]->Fill(tpcTrack.getEta());
      this->mInTracks2DPtEta[matchType::ITSTPC_ITSTPCTRD]->Fill(tpcTrack.getPt(), tpcTrack.getEta());
    }
    // In case of ITS-TPC-TRD-TOF, TPC-TRD, ITS-TPC-TRD-TOF, TPC-TRD-TOF, it is always a TRD track
    if constexpr (isTRDTrack<decltype(trk)>()) {
      // now we need to check if this is a track with or without the ITS
      if (gid.includesDet(GID::DetID::ITS)) {
        const auto& trkITSTPC = mITSTPCTracks[trk.getRefGlobalTrackId()];
        const auto& tpcTrack = mTPCTracks[trkITSTPC.getRefTPC()]; // check!
        if (!selectTrack(tpcTrack)) {
          LOG(debug) << "DEN CONSTR: track with eta " << tpcTrack.getEta() << " and pT " << tpcTrack.getPt() << " DISCARDED for denominator CONSTR, ITSTPC track";
          return true;
        }
        LOG(debug) << "DEN CONSTR: track with eta " << tpcTrack.getEta() << " and pT " << tpcTrack.getPt() << " ACCEPTED for denominator CONSTR, ITSTPC track";
        this->mInTracksPt[matchType::ITSTPC_ITSTPCTRD]->Fill(tpcTrack.getPt());
        this->mInTracksEta[matchType::ITSTPC_ITSTPCTRD]->Fill(tpcTrack.getEta());
        this->mInTracks2DPtEta[matchType::ITSTPC_ITSTPCTRD]->Fill(tpcTrack.getPt(), tpcTrack.getEta());
      } else {
        const auto& tpcTrack = mTPCTracks[trk.getRefGlobalTrackId()];
        if (!selectTrack(tpcTrack)) {
          LOG(debug) << "DEN CONSTR: track with eta " << tpcTrack.getEta() << " and pT " << tpcTrack.getPt() << " DISCARDED for denominator CONSTR, ITSTPC track";
          return true;
        }
        LOG(debug) << "DEN CONSTR: track with eta " << tpcTrack.getEta() << " and pT " << tpcTrack.getPt() << " ACCEPTED for denominator CONSTR, ITSTPC track";
        this->mInTracksPt[matchType::TPCTRD]->Fill(tpcTrack.getPt());
        this->mInTracksEta[matchType::TPCTRD]->Fill(tpcTrack.getEta());
        this->mInTracks2DPtEta[matchType::TPCTRD]->Fill(tpcTrack.getPt(), tpcTrack.getEta());
      }
    }
    return true;
  };
  mRecoCont.createTracksVariadic(creator);

  int nTPCOnlytracks = mRecoCont.getTPCTracks().size() > 0 ? (mRecoCont.getTPCTracks().size() - mITSTPCTracks.size()) : 0;

  LOG(debug) << "We have " << mInTracksPt[matchType::TPC]->GetEntries() << " unconstrained tracks at denominator (should be " << nTPCOnlytracks << "), and " << mInTracksPt[matchType::ITSTPC_ITSTPCTRD]->GetEntries() << " constrained tracks at denominator (should be " << mRecoCont.getTPCITSTracks().size() << " but *before any quality cut!!*)";
  LOG(debug) << "We have " << mMatchedTracksPt[matchType::TPC]->GetEntries() << " TOF matches from unconstrained tracks (should be " << mTPCTOFMatches.size() << "), and " << mMatchedTracksPt[matchType::ITSTPC_ITSTPCTRD]->GetEntries() << " TOF matches from constrained tracks (should be " << mITSTPCTOFMatches.size() << " but *before any quality cut!!*))";

  LOG(debug) << "We have " << mInTracksPt[matchType::TPC]->GetEntries() << " unconstrained tracks at denominator, and " << mInTracksPt[matchType::ITSTPC_ITSTPCTRD]->GetEntries() << " ITS-constrained (ITSTPC + ITSTPCTRD) tracks at denominator, and " << mInTracksPt[matchType::TPCTRD]->GetEntries() << " TRD-constrained tracks at denominator";
  LOG(debug) << "We have " << mMatchedTracksPt[matchType::TPC]->GetEntries() << " TOF matches from unconstrained tracks and " << mMatchedTracksPt[matchType::ITSTPC_ITSTPCTRD]->GetEntries() << " TOF matches from ITS-constrained (ITSTPC + ITSTPCTRD) tracks, and" << mMatchedTracksPt[matchType::TPCTRD]->GetEntries() << " TOF matches from TRD-constrained tracks";

  // logging in case denominator has less tracks than numerator
  for (int i = 0; i < matchType::SIZE; ++i) {
    for (int ibin = 1; ibin <= mMatchedTracksPt[i]->GetNbinsX(); ++ibin) {
      LOG(debug) << "check: ibin " << ibin << ": mInTracksPt[" << i << "]->GetBinContent(" << ibin << ") = " << mInTracksPt[i]->GetBinContent(ibin) << ", mMatchedTracksPt[" << i << "]->GetBinContent(" << ibin << ") = " << mMatchedTracksPt[i]->GetBinContent(ibin);
      if (mInTracksPt[i]->GetBinContent(ibin) < mMatchedTracksPt[i]->GetBinContent(ibin)) {
        LOG(error) << "issue spotted: ibin " << ibin << ": mInTracksPt[" << i << "]->GetBinContent(" << ibin << ") = " << mInTracksPt[i]->GetBinContent(ibin) << ", mMatchedTracksPt[" << i << "]->GetBinContent(" << ibin << ") = " << mMatchedTracksPt[i]->GetBinContent(ibin);
      }
      if (mUseMC) {
        if (mMatchedTracksPt[i]->GetBinContent(ibin) < mFakeMatchedTracksPt[i]->GetBinContent(ibin)) {
          LOG(error) << "issue spotted: ibin " << ibin << ": mMatchedTracksPt[" << i << "]->GetBinContent(" << ibin << ") = " << mMatchedTracksPt[i]->GetBinContent(ibin) << ", mFakeMatchedTracksPt[" << i << "]->GetBinContent(" << ibin << ") = " << mFakeMatchedTracksPt[i]->GetBinContent(ibin);
        }
      }
    }
    for (int ibin = 1; ibin <= mMatchedTracksEta[i]->GetNbinsX(); ++ibin) {
      LOG(debug) << "check: ibin " << ibin << ": mInTracksEta[" << i << "]->GetBinContent(" << ibin << ") = " << mInTracksEta[i]->GetBinContent(ibin) << ", mMatchedTracksEta[" << i << "]->GetBinContent(" << ibin << ") = " << mMatchedTracksEta[i]->GetBinContent(ibin);
      if (mInTracksEta[i]->GetBinContent(ibin) < mMatchedTracksEta[i]->GetBinContent(ibin)) {
        LOG(error) << "issue spotted: ibin " << ibin << ": mInTracksEta[" << i << "]->GetBinContent(" << ibin << ") = " << mInTracksEta[i]->GetBinContent(ibin) << ", mMatchedTracksEta[" << i << "]->GetBinContent(" << ibin << ") = " << mMatchedTracksEta[i]->GetBinContent(ibin);
      }
      if (mUseMC) {
        if (mMatchedTracksEta[i]->GetBinContent(ibin) < mFakeMatchedTracksEta[i]->GetBinContent(ibin)) {
          LOG(error) << "issue spotted: ibin " << ibin << ": mMatchedTracksEta[" << i << "]->GetBinContent(" << ibin << ") = " << mMatchedTracksEta[i]->GetBinContent(ibin) << ", mFakeMatchedTracksEta[" << i << "]->GetBinContent(" << ibin << ") = " << mFakeMatchedTracksEta[i]->GetBinContent(ibin);
        }
      }
    }
  }
}

void TOFMatchedTracks::endOfCycle()
{

  ILOG(Debug, Devel) << "endOfCycle" << ENDM;

  // Logging in case any denominator has less entries than the corresponding numerator
  for (int i = 0; i < matchType::SIZE; ++i) {
    for (int ibin = 1; ibin <= mMatchedTracksPt[i]->GetNbinsX(); ++ibin) {
      if (mInTracksPt[i]->GetBinContent(ibin) < mMatchedTracksPt[i]->GetBinContent(ibin)) {
        LOG(error) << "End Of Cycle issue spotted: ibin " << ibin << ": mInTracksPt[" << i << "]->GetBinContent(" << ibin << ") = " << mInTracksPt[i]->GetBinContent(ibin) << ", mMatchedTracksPt[" << i << "]->GetBinContent(" << ibin << ") = " << mMatchedTracksPt[i]->GetBinContent(ibin);
      }
      if (mInTracksPt[i]->GetBinContent(ibin) != 0 || mMatchedTracksPt[i]->GetBinContent(ibin) != 0) {
        LOG(debug) << "End Of Cycle - histo filled : ibin " << ibin << ": mInTracksPt[" << i << "]->GetBinContent(" << ibin << ") = " << mInTracksPt[i]->GetBinContent(ibin) << ", mMatchedTracksPt[" << i << "]->GetBinContent(" << ibin << ") = " << mMatchedTracksPt[i]->GetBinContent(ibin);
      }
      if (mUseMC) {
        if (mMatchedTracksPt[i]->GetBinContent(ibin) < mFakeMatchedTracksPt[i]->GetBinContent(ibin)) {
          LOG(error) << "End Of Cycle issue spotted: ibin " << ibin << ": mMatchedTracksPt[" << i << "]->GetBinContent(" << ibin << ") = " << mMatchedTracksPt[i]->GetBinContent(ibin) << ", mFakeMatchedTracksPt[" << i << "]->GetBinContent(" << ibin << ") = " << mFakeMatchedTracksPt[i]->GetBinContent(ibin);
        }
      }
    }
    for (int ibin = 1; ibin <= mMatchedTracksEta[i]->GetNbinsX(); ++ibin) {
      if (mInTracksEta[i]->GetBinContent(ibin) < mMatchedTracksEta[i]->GetBinContent(ibin)) {
        LOG(error) << "End Of Cycle issue spotted: ibin " << ibin << ": mInTracksEta[" << i << "]->GetBinContent(" << ibin << ") = " << mInTracksEta[i]->GetBinContent(ibin) << ", mMatchedTracksEta[" << i << "]->GetBinContent(" << ibin << ") = " << mMatchedTracksEta[i]->GetBinContent(ibin);
      }
      if (mInTracksEta[i]->GetBinContent(ibin) != 0 || mMatchedTracksEta[i]->GetBinContent(ibin) != 0) {
        LOG(debug) << "End Of Cycle - histo filled: ibin " << ibin << ": mInTracksEta[" << i << "]->GetBinContent(" << ibin << ") = " << mInTracksEta[i]->GetBinContent(ibin) << ", mMatchedTracksEta[" << i << "]->GetBinContent(" << ibin << ") = " << mMatchedTracksEta[i]->GetBinContent(ibin);
      }
      if (mUseMC) {
        if (mMatchedTracksEta[i]->GetBinContent(ibin) < mFakeMatchedTracksEta[i]->GetBinContent(ibin)) {
          LOG(error) << "End Of Cycle issue spotted: ibin " << ibin << ": mMatchedTracksEta[" << i << "]->GetBinContent(" << ibin << ") = " << mMatchedTracksEta[i]->GetBinContent(ibin) << ", mFakeMatchedTracksEta[" << i << "]->GetBinContent(" << ibin << ") = " << mFakeMatchedTracksEta[i]->GetBinContent(ibin);
        }
      }
    }
    for (int ibinx = 1; ibinx <= mMatchedTracks2DPtEta[i]->GetNbinsX(); ++ibinx) {
      for (int ibiny = 1; ibiny <= mMatchedTracks2DPtEta[i]->GetNbinsY(); ++ibiny) {
        if (mInTracks2DPtEta[i]->GetBinContent(ibinx, ibiny) < mMatchedTracks2DPtEta[i]->GetBinContent(ibinx, ibiny)) {
          LOG(error) << "End Of Cycle issue spotted: ibinx " << ibinx << ", ibiny " << ibiny << ": mInTracks2DPtEta[" << i << "]->GetBinContent(" << ibinx << ", " << ibiny << ") = " << mInTracks2DPtEta[i]->GetBinContent(ibinx, ibiny) << ", mMatchedTracks2DPtEta[" << i << "]->GetBinContent(" << ibinx << ", " << ibiny << ") = " << mMatchedTracks2DPtEta[i]->GetBinContent(ibinx, ibiny);
        }
      }
    }
  }

  if (mRecoCont.isTrackSourceLoaded(GID::TPCTOF)) {
    if (!mEffPt[matchType::TPC]->SetTotalHistogram(*mInTracksPt[matchType::TPC], "f") || // all total have to be forcely replaced, or the passed from previous processing may have incompatible number of entries
        !mEffPt[matchType::TPC]->SetPassedHistogram(*mMatchedTracksPt[matchType::TPC], "") ||
        !mEffEta[matchType::TPC]->SetTotalHistogram(*mInTracksEta[matchType::TPC], "f") ||
        !mEffEta[matchType::TPC]->SetPassedHistogram(*mMatchedTracksEta[matchType::TPC], "") ||
        !mEff2DPtEta[matchType::TPC]->SetTotalHistogram(*mInTracks2DPtEta[matchType::TPC], "f") ||
        !mEff2DPtEta[matchType::TPC]->SetPassedHistogram(*mMatchedTracks2DPtEta[matchType::TPC], "")) {
      ILOG(Fatal, Support) << "Something went wrong in defining the efficiency histograms, UNCONS!!" << ENDM;
    }
    if (mUseMC) {
      if (!mFakeFractionTracksPt[matchType::TPC]->SetTotalHistogram(*mMatchedTracksPt[matchType::TPC], "f") ||
          !mFakeFractionTracksPt[matchType::TPC]->SetPassedHistogram(*mFakeMatchedTracksPt[matchType::TPC], "") ||
          !mFakeFractionTracksEta[matchType::TPC]->SetTotalHistogram(*mMatchedTracksEta[matchType::TPC], "f") ||
          !mFakeFractionTracksEta[matchType::TPC]->SetPassedHistogram(*mFakeMatchedTracksEta[matchType::TPC], "")) {
        ILOG(Fatal, Support) << "Something went wrong in defining the efficiency histograms for MC, UNCONS!!" << ENDM;
      }
    }
  }
  if (mRecoCont.isTrackSourceLoaded(GID::ITSTPCTOF) || mRecoCont.isTrackSourceLoaded(GID::ITSTPCTRDTOF)) {
    if (!mEffPt[matchType::ITSTPC_ITSTPCTRD]->SetTotalHistogram(*mInTracksPt[matchType::ITSTPC_ITSTPCTRD], "f") ||
        !mEffPt[matchType::ITSTPC_ITSTPCTRD]->SetPassedHistogram(*mMatchedTracksPt[matchType::ITSTPC_ITSTPCTRD], "") ||
        !mEffEta[matchType::ITSTPC_ITSTPCTRD]->SetTotalHistogram(*mInTracksEta[matchType::ITSTPC_ITSTPCTRD], "f") ||
        !mEffEta[matchType::ITSTPC_ITSTPCTRD]->SetPassedHistogram(*mMatchedTracksEta[matchType::ITSTPC_ITSTPCTRD], "") ||
        !mEff2DPtEta[matchType::ITSTPC_ITSTPCTRD]->SetTotalHistogram(*mInTracks2DPtEta[matchType::ITSTPC_ITSTPCTRD], "f") ||
        !mEff2DPtEta[matchType::ITSTPC_ITSTPCTRD]->SetPassedHistogram(*mMatchedTracks2DPtEta[matchType::ITSTPC_ITSTPCTRD], "")) {
      ILOG(Fatal, Support) << "Something went wrong in defining the efficiency histograms, ITS-CONSTR (ITSTPC + ITSTPCTRD)!!" << ENDM;
    }
    if (mUseMC) {
      if (!mFakeFractionTracksPt[matchType::ITSTPC_ITSTPCTRD]->SetTotalHistogram(*mMatchedTracksPt[matchType::ITSTPC_ITSTPCTRD], "f") ||
          !mFakeFractionTracksPt[matchType::ITSTPC_ITSTPCTRD]->SetPassedHistogram(*mFakeMatchedTracksPt[matchType::ITSTPC_ITSTPCTRD], "") ||
          !mFakeFractionTracksEta[matchType::ITSTPC_ITSTPCTRD]->SetTotalHistogram(*mMatchedTracksEta[matchType::ITSTPC_ITSTPCTRD], "f") ||
          !mFakeFractionTracksEta[matchType::ITSTPC_ITSTPCTRD]->SetPassedHistogram(*mFakeMatchedTracksEta[matchType::ITSTPC_ITSTPCTRD], "")) {
        ILOG(Fatal, Support) << "Something went wrong in defining the efficiency histograms for MC, ITS-CONSTR (ITSTPC + ITSTPCTRD)!!" << ENDM;
      }
    }
  }
  if (mRecoCont.isTrackSourceLoaded(GID::TPCTRDTOF)) {
    if (!mEffPt[matchType::TPCTRD]->SetTotalHistogram(*mInTracksPt[matchType::TPCTRD], "f") ||
        !mEffPt[matchType::TPCTRD]->SetPassedHistogram(*mMatchedTracksPt[matchType::TPCTRD], "") ||
        !mEffEta[matchType::TPCTRD]->SetTotalHistogram(*mInTracksEta[matchType::TPCTRD], "f") ||
        !mEffEta[matchType::TPCTRD]->SetPassedHistogram(*mMatchedTracksEta[matchType::TPCTRD], "") ||
        !mEff2DPtEta[matchType::TPCTRD]->SetTotalHistogram(*mInTracks2DPtEta[matchType::TPCTRD], "f") ||
        !mEff2DPtEta[matchType::TPCTRD]->SetPassedHistogram(*mMatchedTracks2DPtEta[matchType::TPCTRD], "")) {
      ILOG(Fatal, Support) << "Something went wrong in defining the efficiency histograms, TRD-CONSTR (TPCTRD)!!" << ENDM;
    }
    if (mUseMC) {
      if (!mFakeFractionTracksPt[matchType::TPCTRD]->SetTotalHistogram(*mMatchedTracksPt[matchType::TPCTRD], "f") ||
          !mFakeFractionTracksPt[matchType::TPCTRD]->SetPassedHistogram(*mFakeMatchedTracksPt[matchType::TPCTRD], "") ||
          !mFakeFractionTracksEta[matchType::TPCTRD]->SetTotalHistogram(*mMatchedTracksEta[matchType::TPCTRD], "f") ||
          !mFakeFractionTracksEta[matchType::TPCTRD]->SetPassedHistogram(*mFakeMatchedTracksEta[matchType::TPCTRD], "")) {
        ILOG(Fatal, Support) << "Something went wrong in defining the efficiency histograms for MC, TRD-CONSTR (TPCTRD)!!" << ENDM;
      }
    }
  }

  // Printing, for checks
  if (mVerbose) {
    for (int i = 0; i < matchType::SIZE; ++i) {
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

void TOFMatchedTracks::endOfActivity(const Activity& /*activity*/)
{
  ILOG(Debug, Devel) << "endOfActivity" << ENDM;
}

void TOFMatchedTracks::reset()
{
  // clean all the monitor objects here

  ILOG(Debug, Devel) << "Resetting the histograms" << ENDM;
  for (int i = 0; i < matchType::SIZE; ++i) {
    mMatchedTracksPt[i]->Reset();
    mMatchedTracksEta[i]->Reset();
    mMatchedTracks2DPtEta[i]->Reset();
    if (mUseMC) {
      mFakeMatchedTracksPt[i]->Reset();
      mFakeMatchedTracksEta[i]->Reset();
    }
    mInTracksPt[i]->Reset();
    mInTracksEta[i]->Reset();
    mInTracks2DPtEta[i]->Reset();
    mDeltaZEta[i]->Reset();
    mDeltaZPhi[i]->Reset();
    mDeltaZPt[i]->Reset();
    mDeltaXEta[i]->Reset();
    mDeltaXPhi[i]->Reset();
    mDeltaXPt[i]->Reset();
    mTOFChi2[i]->Reset();
    mTOFChi2Pt[i]->Reset();
  }

  for (int isec = 0; isec < 18; isec++) {
    mDTimeTrk[isec]->Reset();
    mDTimeTrkTPC[isec]->Reset();
    mDTimeTrkTRD[isec]->Reset();
  }
}

//__________________________________________________________

bool TOFMatchedTracks::selectTrack(o2::tpc::TrackTPC const& track)
{

  if (track.getPt() <= mPtCut) {
    return false;
  }
  if (std::abs(track.getEta()) >= mEtaCut) {
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
