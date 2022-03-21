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
/// \file   TaskPID.cxx
/// \author Francesca Ercolessi
/// \brief  Task to monitor TOF PID performance
/// \since  13/01/2022
///

#include <TCanvas.h>
#include <TH1.h>
#include <TH2.h>

#include "QualityControl/QcInfoLogger.h"
#include "TOF/TaskPID.h"
#include <Framework/InputRecord.h>
#include <Framework/InputRecordWalker.h>
#include <Framework/InputSpec.h>
#include "GlobalTrackingWorkflowHelpers/InputHelper.h"
#include "ReconstructionDataFormats/TrackTPCITS.h"
#include "DataFormatsTPC/TrackTPC.h"
#include "DataFormatsGlobalTracking/RecoContainerCreateTracksVariadic.h"
#include "ReconstructionDataFormats/TrackParametrization.h"
#include "DetectorsBase/Propagator.h"
#include "DetectorsBase/GeometryManager.h"
#include "TOFBase/EventTimeMaker.h"
#include "GlobalTrackingWorkflow/TOFEventTimeChecker.h"

// from TOF
#include "TOFBase/Geo.h"
#include "TOFBase/Utils.h"
#include "DataFormatsTOF/Cluster.h"
#include "TOFBase/EventTimeMaker.h"

using GTrackID = o2::dataformats::GlobalTrackID;

namespace o2::quality_control_modules::tof
{

bool MyFilter(const MyTrack& tr)
{
  return (tr.getP() < 2.0);
}

TaskPID::~TaskPID()
{
  // delete
  delete mHistDeltatPi;
  delete mHistDeltatKa;
  delete mHistDeltatPr;
  delete mHistDeltatPiPt;
  delete mHistDeltatKaPt;
  delete mHistDeltatPrPt;
  delete mHistMass;
  delete mHistBetavsP;
}

void TaskPID::initialize(o2::framework::InitContext& /*ctx*/)
{
  ILOG(Info, Support) << " Initializing... " << ENDM;
  // track selection
  if (auto param = mCustomParameters.find("minPtCut"); param != mCustomParameters.end()) {
    ILOG(Info, Devel) << "Custom parameter - minPtCut (for track selection): " << param->second << ENDM;
    setMinPtCut(atof(param->second.c_str()));
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
    setMinDCAtoBeamPipeCut(atof(param->second.c_str()));
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

  // For now only ITS-TPC tracks can be used
  // if ((mSrc[GID::Source::TPCTOF] == 1 && mSrc[GID::Source::TPC] == 0) || (mSrc[GID::Source::TPCTOF] == 0 && mSrc[GID::Source::TPC] == 1)) {
  //  ILOG(Fatal, Support) << "Check the requested sources: TPCTOF = " << mSrc[GID::Source::TPCTOF] << ", TPC = " << mSrc[GID::Source::TPC] << ENDM;
  //}
  //
  if ((mSrc[GID::Source::ITSTPCTOF] == 1 && mSrc[GID::Source::ITSTPC] == 0) || (mSrc[GID::Source::ITSTPCTOF] == 0 && mSrc[GID::Source::ITSTPC] == 1)) {
    ILOG(Fatal, Support) << "Check the requested sources: ITSTPCTOF = " << mSrc[GID::Source::ITSTPCTOF] << ", ITSTPC = " << mSrc[GID::Source::ITSTPC] << ENDM;
  }

  // initialize histgrams
  mHistDeltatPi = new TH1F("DeltatPi", ";t_{TOF} - t_{exp}^{#pi} (ps)", 500, -5000, 5000);
  mHistDeltatKa = new TH1F("DeltatKa", ";t_{TOF} - t_{exp}^{K} (ps)", 500, -5000, 5000);
  mHistDeltatPr = new TH1F("DeltatPr", ";t_{TOF} - t_{exp}^{p} (ps)", 500, -5000, 5000);
  mHistDeltatPiPt = new TH2F("DeltatPi_Pt", ";#it{p}_{T} (GeV/#it{c});t_{TOF} - t_{exp}^{#pi} (ps)", 1000, 0., 20., 500, -5000, 5000);
  mHistDeltatKaPt = new TH2F("DeltatKa_Pt", ";#it{p}_{T} (GeV/#it{c});t_{TOF} - t_{exp}^{#pi} (ps)", 1000, 0., 20., 500, -5000, 5000);
  mHistDeltatPrPt = new TH2F("DeltatPr_Pt", ";#it{p}_{T} (GeV/#it{c});t_{TOF} - t_{exp}^{#pi} (ps)", 1000, 0., 20., 500, -5000, 5000);
  mHistMass = new TH1F("HadronMasses", ";M (GeV/#it{c}^{2})", 1000, 0, 3.);
  mHistBetavsP = new TH2F("BetavsP", ";#it{p} (GeV/#it{c});TOF #beta", 1000, 0., 5, 1000, 0., 1.5);

  // initialize B field and geometry for track selection
  o2::base::GeometryManager::loadGeometry(mGeomFileName);
  o2::base::Propagator::initFieldFromGRP(mGRPFileName);
  mBz = o2::base::Propagator::Instance()->getNominalBz();

  // publish histgrams
  if (mSrc[GID::Source::ITSTPCTOF] == 1) {
    getObjectsManager()->startPublishing(mHistDeltatPi);
    getObjectsManager()->startPublishing(mHistDeltatKa);
    getObjectsManager()->startPublishing(mHistDeltatPr);
    getObjectsManager()->startPublishing(mHistDeltatPiPt);
    getObjectsManager()->startPublishing(mHistDeltatKaPt);
    getObjectsManager()->startPublishing(mHistDeltatPrPt);
    getObjectsManager()->startPublishing(mHistMass);
    getObjectsManager()->startPublishing(mHistBetavsP);
  }
  ILOG(Info, Support) << " Initialized!!!! " << ENDM;

  mDataRequest = std::make_shared<o2::globaltracking::DataRequest>();
  mDataRequest->requestTracks(mSrc, 0 /*mUseMC*/);
}

void TaskPID::startOfActivity(Activity& activity)
{
  ILOG(Info, Support) << "startOfActivity " << activity.mId << ENDM;
  reset();
}

void TaskPID::startOfCycle()
{
  ILOG(Info, Support) << "startOfCycle" << ENDM;
}

void TaskPID::monitorData(o2::framework::ProcessingContext& ctx)
{
  ++mTF;
  ILOG(Info, Support) << " Processing TF: " << mTF << ENDM;

  mRecoCont.collectData(ctx, *mDataRequest.get());

  if (mRecoCont.isTrackSourceLoaded(GID::ITSTPCTOF)) { // if track is ITS+TPC

    mITSTPCTracks = mRecoCont.getTPCITSTracks();
    mITSTPCTOFMatches = mRecoCont.getITSTPCTOFMatches();
    mTPCTracks = mRecoCont.getTPCTracks();

    mMyTracks.clear();

    // loop over TOF MatchInfo
    for (const auto& matchTOF : mITSTPCTOFMatches) {

      GTrackID gTrackId = matchTOF.getTrackRef();
      const auto& trk = mITSTPCTracks[gTrackId.getIndex()];
      const auto& trkTPC = mTPCTracks[trk.getRefTPC()];

      const o2::track::TrackLTIntegral& info = matchTOF.getLTIntegralOut();

      if (!selectTrack(trkTPC)) { // Discard tracks according to configurable cuts
        continue;
      }

      mMyTracks.push_back(MyTrack(matchTOF, trk));

    } // END loop on TOF matches

    std::vector<MyTrack> tracks;

    // sorting matching in time
    std::sort(mMyTracks.begin(), mMyTracks.end(),
              [](MyTrack a, MyTrack b) { return a.tofSignalDouble() < b.tofSignalDouble(); });

    for (int i = 0; i < mMyTracks.size(); i++) { // loop looking for interaction candidates
      tracks.clear();
      int ntrk = 1;
      double time = mMyTracks[i].tofSignalDouble();
      tracks.emplace_back(mMyTracks[i]);
      for (; i < mMyTracks.size(); i++) {
        double timeCurrent = mMyTracks[i].tofSignalDouble();
        if (timeCurrent - time > 100E3) {
          i--;
          break;
        }
        tracks.emplace_back(mMyTracks[i]);
        ntrk++;
      }
      if (ntrk > 0) { // good candidate with time
        processEvent(tracks);
      }
    }
  } // END if track is ITS-TPC

  ILOG(Info, Support) << " Processed! " << ENDM;
  return;
}

void TaskPID::endOfCycle()
{

  ILOG(Info, Support) << "endOfCycle" << ENDM;
}

void TaskPID::endOfActivity(Activity& /*activity*/)
{
  ILOG(Info, Support) << "endOfActivity" << ENDM;
}

void TaskPID::reset()
{
  // clean all the monitor objects here
  mHistDeltatPi->Reset();
  mHistDeltatKa->Reset();
  mHistDeltatPr->Reset();
  mHistDeltatPiPt->Reset();
  mHistDeltatKaPt->Reset();
  mHistDeltatPrPt->Reset();
  mHistMass->Reset();
  mHistBetavsP->Reset();
}

void TaskPID::processEvent(const std::vector<MyTrack>& tracks)
{

  auto evtime = o2::tof::evTimeMaker<std::vector<MyTrack>, MyTrack, MyFilter>(tracks);

  int nt = 0;
  for (auto& track : tracks) {

    float mT0 = evtime.eventTime;
    float mT0Res = evtime.eventTimeError;
    //
    float sumw = 1. / (mT0Res * mT0Res);
    mT0 *= sumw;
    mT0 -= evtime.weights[nt] * evtime.tracktime[nt];
    sumw -= evtime.weights[nt];
    mT0 /= sumw;
    mT0Res = sqrt(1. / sumw);

    nt++;

    // Delta t Pion
    const float deltatpi = track.tofSignal() - mT0 - track.tofExpSignalPi();
    // Delta t Kaon
    const float deltatka = track.tofSignal() - mT0 - track.tofExpSignalKa();
    // Delta t Proton
    const float deltatpr = track.tofSignal() - mT0 - track.tofExpSignalPr();
    // Beta
    const float beta = track.getL() / (track.tofSignal() - mT0) * cinv;
    // Mass
    const float mass = track.getP() / beta * TMath::Sqrt(TMath::Abs(1 - beta * beta));

    mHistDeltatPi->Fill(deltatpi);
    mHistDeltatKa->Fill(deltatka);
    mHistDeltatPr->Fill(deltatpr);
    mHistDeltatPiPt->Fill(track.getPt(), deltatpi);
    mHistDeltatKaPt->Fill(track.getPt(), deltatka);
    mHistDeltatPrPt->Fill(track.getPt(), deltatpr);
    mHistMass->Fill(mass);
    mHistBetavsP->Fill(track.getP(), beta);
  }
}

//__________________________________________________________

bool TaskPID::selectTrack(o2::tpc::TrackTPC const& track)
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
