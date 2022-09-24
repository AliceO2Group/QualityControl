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
/// \file   TaskFT0TOF.cxx
/// \author Francesca Ercolessi
/// \brief  Task to monitor TOF PID performance
/// \since  13/01/2022
///

#include <TCanvas.h>
#include <TH1.h>
#include <TH2.h>

#include "QualityControl/QcInfoLogger.h"
#include "PID/TaskFT0TOF.h"
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
#include "DetectorsRaw/HBFUtils.h"

// from TOF
#include "TOFBase/Geo.h"
#include "TOFBase/Utils.h"
#include "DataFormatsTOF/Cluster.h"
#include "TOFBase/EventTimeMaker.h"

using GTrackID = o2::dataformats::GlobalTrackID;
using namespace o2::tof;

namespace o2::quality_control_modules::pid
{

bool MyFilter(const MyTrack& tr)
{
  return (tr.getP() < 2.0);
}

TaskFT0TOF::~TaskFT0TOF()
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
  delete mHistDeltatPiEvTimeRes;
  delete mHistDeltatPiEvTimeMult;
  delete mHistEvTimeResEvTimeMult;
  delete mHistEvTimeTOF;
}

void TaskFT0TOF::initialize(o2::framework::InitContext& /*ctx*/)
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
  if (auto param = mCustomParameters.find("useFT0"); param != mCustomParameters.end()) {
    ILOG(Info, Devel) << "Custom parameter - useFT0: " << param->second << ENDM;
    if (param->second == "true" || param->second == "True" || param->second == "TRUE") {
      mUseFT0 = true;
    }
  }

  // for track type selection
  if (auto param = mCustomParameters.find("GID"); param != mCustomParameters.end()) {
    ILOG(Info, Devel) << "Custom parameter - GID (= sources by user): " << param->second << ENDM;
    ILOG(Info, Devel) << "Allowed Sources = " << mAllowedSources << ENDM;
    mSrc = mAllowedSources & GID::getSourcesMask(param->second);
    ILOG(Info, Devel) << "Final requested sources = " << mSrc << ENDM;
  }

  // TPC tracks
  if ((mSrc[GID::Source::TPCTOF] == 1 && mSrc[GID::Source::TPC] == 0) || (mSrc[GID::Source::TPCTOF] == 0 && mSrc[GID::Source::TPC] == 1)) {
    ILOG(Fatal, Support) << "Check the requested sources: TPCTOF = " << mSrc[GID::Source::TPCTOF] << ", TPC = " << mSrc[GID::Source::TPC] << ENDM;
  }

  // ITS-TPC tracks
  if ((mSrc[GID::Source::ITSTPCTOF] == 1 && mSrc[GID::Source::ITSTPC] == 0) || (mSrc[GID::Source::ITSTPCTOF] == 0 && mSrc[GID::Source::ITSTPC] == 1)) {
    ILOG(Fatal, Support) << "Check the requested sources: ITSTPCTOF = " << mSrc[GID::Source::ITSTPCTOF] << ", ITSTPC = " << mSrc[GID::Source::ITSTPC] << ENDM;
  }

  // TPC-TRD tracks
  if ((mSrc[GID::Source::TPCTRDTOF] == 1 && mSrc[GID::Source::TPCTRD] == 0) || (mSrc[GID::Source::TPCTRDTOF] == 0 && mSrc[GID::Source::TPCTRD] == 1)) {
    ILOG(Fatal, Support) << "Check the requested sources: TPCTRDTOF = " << mSrc[GID::Source::TPCTRDTOF] << ", TPCTRD = " << mSrc[GID::Source::TPCTRD] << ENDM;
  }

  // ITS-TPC-TRD tracks
  if ((mSrc[GID::Source::ITSTPCTRDTOF] == 1 && mSrc[GID::Source::ITSTPCTRD] == 0) || (mSrc[GID::Source::ITSTPCTRDTOF] == 0 && mSrc[GID::Source::ITSTPCTRD] == 1)) {
    ILOG(Fatal, Support) << "Check the requested sources: ITSTPCTRDTOF = " << mSrc[GID::Source::ITSTPCTRDTOF] << ", ITSTPCTRD = " << mSrc[GID::Source::ITSTPCTRD] << ENDM;
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
  mHistDeltatPiEvTimeRes = new TH2F("DeltatPiEvtimeRes", "0.7 < p < 1.1 GeV/#it{c};TOF event time resolution (ps);t_{TOF} - t_{exp}^{#pi} (ps)", 200, 0., 200, 500, -5000, 5000);
  mHistDeltatPiEvTimeMult = new TH2F("DeltatPiEvTimeMult", "0.7 < p < 1.1 GeV/#it{c};TOF multiplicity; t_{TOF} - t_{exp}^{#pi} (ps)", 100, 0., 100, 500, -5000, 5000);
  mHistEvTimeResEvTimeMult = new TH2F("EvTimeResEvTimeMult", "0.7 < p < 1.1 GeV/#it{c};TOF multiplicity;TOF event time resolution (ps)", 100, 0., 100, 200, 0, 200);
  mHistEvTimeTOF = new TH1F("EvTimeTOF", "t_{0}^{TOF};t_{0}^{TOF} (ps);Counts", 1000, -5000., +5000);

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
    getObjectsManager()->startPublishing(mHistDeltatPiEvTimeRes);
    getObjectsManager()->startPublishing(mHistDeltatPiEvTimeMult);
    getObjectsManager()->startPublishing(mHistEvTimeResEvTimeMult);
    getObjectsManager()->startPublishing(mHistEvTimeTOF);
  }
  ILOG(Info, Support) << " Initialized!!!! " << ENDM;

  mDataRequest = std::make_shared<o2::globaltracking::DataRequest>();
  mDataRequest->requestTracks(mSrc, 0 /*mUseMC*/);
}

void TaskFT0TOF::startOfActivity(Activity& activity)
{
  ILOG(Info, Support) << "startOfActivity " << activity.mId << ENDM;
  reset();
}

void TaskFT0TOF::startOfCycle()
{
  ILOG(Info, Support) << "startOfCycle" << ENDM;
}

void TaskFT0TOF::monitorData(o2::framework::ProcessingContext& ctx)
{
  ++mTF;
  ILOG(Info, Support) << " Processing TF: " << mTF << ENDM;

  mRecoCont.collectData(ctx, *mDataRequest.get());

  mMyTracks.clear();

  // Get FT0 RecPointss
//  const gsl::span<o2::ft0::RecPoints>* ft0rec = nullptr;
  const std::vector<o2::ft0::RecPoints>* ft0rec = nullptr;
  if(mUseFT0){
    auto& obj = ctx.inputs().get<const std::vector<o2::ft0::RecPoints>>("recpoints");
//    const auto& obj = mRecoCont.getFT0RecPoints();
    ft0rec = &obj;
  }

  if(mUseFT0){
    ILOG(Info, Support) << "FT0 rec points loaded, size = " << ft0rec->size() << ENDM;
  } else {
    ILOG(Info, Support) << "FT0 rec points NOT available" << ENDM;
  }

  // TPC-TOF
  if (mRecoCont.isTrackSourceLoaded(GID::TPCTOF)) { // if track is TPC

    mTPCTracks = mRecoCont.getTPCTracks();
    mTPCTOFMatches = mRecoCont.getTPCTOFMatches();

    if (mRecoCont.getTPCTOFTracks().size() != mTPCTOFMatches.size()) {
      ILOG(Fatal, Support) << "Number of TPCTOF tracks (" << mRecoCont.getTPCTOFTracks().size() << ") differs from number of TPCTOF matches (" << mTPCTOFMatches.size() << ")" << ENDM;
    }

    // loop over TOF MatchInfo
    for (const auto& matchTOF : mTPCTOFMatches) {

      GTrackID gTrackId = matchTOF.getTrackRef();
      const auto& trk = mTPCTracks[gTrackId.getIndex()];

      if (!selectTrack(trk)) { // Discard tracks according to configurable cuts
        continue;
      }

      mMyTracks.push_back(MyTrack(matchTOF, trk));
    } // END loop on TOF matches
  }   // END if track is TPC-TOF

  // ITS-TPC-TOF
  if (mRecoCont.isTrackSourceLoaded(GID::ITSTPCTOF)) { // if track is ITS+TPC

    mITSTPCTracks = mRecoCont.getTPCITSTracks();
    mITSTPCTOFMatches = mRecoCont.getITSTPCTOFMatches();
    mTPCTracks = mRecoCont.getTPCTracks();

    // loop over TOF MatchInfo
    for (const auto& matchTOF : mITSTPCTOFMatches) {

      GTrackID gTrackId = matchTOF.getTrackRef();
      const auto& trk = mITSTPCTracks[gTrackId.getIndex()];
      const auto& trkTPC = mTPCTracks[trk.getRefTPC()];

      if (!selectTrack(trkTPC)) { // Discard tracks according to configurable cuts
        continue;
      }

      mMyTracks.push_back(MyTrack(matchTOF, trk));

    } // END loop on TOF matches
  }   // END if track is ITS-TPC-TOF

  // TPC-TRD-TOF
  if (mRecoCont.isTrackSourceLoaded(GID::TPCTRDTOF)) { // this is enough to know that also TPCTRD was loades, see "initialize"

    mTPCTRDTracks = mRecoCont.getTPCTRDTracks<o2::trd::TrackTRD>();
    mTPCTRDTOFMatches = mRecoCont.getTPCTRDTOFMatches();

    // loop over TOF MatchInfo
    for (const auto& matchTOF : mTPCTRDTOFMatches) {

      GTrackID gTrackId = matchTOF.getTrackRef();
      const auto& trk = mTPCTRDTracks[gTrackId.getIndex()];
      const auto& trkTPC = mTPCTracks[trk.getRefGlobalTrackId()];

      if (!selectTrack(trkTPC)) { // Discard tracks according to configurable cuts
        continue;
      }

      mMyTracks.push_back(MyTrack(matchTOF, trk));

    } // END loop on TOF matches
  }   // END if track is TPC-TRD-TOF

  // ITS-TPC-TRD-TOF
  if (mRecoCont.isTrackSourceLoaded(GID::ITSTPCTRDTOF)) { // this is enough to know that also ITSTPC was loades, see "initialize"

    mITSTPCTRDTracks = mRecoCont.getITSTPCTRDTracks<o2::trd::TrackTRD>();
    mITSTPCTRDTOFMatches = mRecoCont.getITSTPCTRDTOFMatches();

    // loop over TOF MatchInfo
    for (const auto& matchTOF : mITSTPCTRDTOFMatches) {

      GTrackID gTrackId = matchTOF.getTrackRef();
      const auto& trk = mITSTPCTRDTracks[gTrackId.getIndex()];
      const auto& trkITSTPC = mITSTPCTracks[trk.getRefGlobalTrackId()];
      const auto& trkTPC = mTPCTracks[trkITSTPC.getRefTPC()];

      if (!selectTrack(trkTPC)) { // Discard tracks according to configurable cuts
        continue;
      }

      mMyTracks.push_back(MyTrack(matchTOF, trk));

    } // END loop on TOF matches
  }   // END if track is TPC-TRD-TOF

  std::vector<MyTrack> tracks;
  std::vector<o2::ft0::RecPoints> ft0Sorted = *ft0rec;
  std::vector<o2::ft0::RecPoints> ft0Cand;

  // sorting matching in time
  std::sort(mMyTracks.begin(), mMyTracks.end(),
            [](MyTrack a, MyTrack b) { return a.tofSignalDouble() < b.tofSignalDouble(); });

  std::sort(ft0Sorted.begin(), ft0Sorted.end(),
            [](o2::ft0::RecPoints a, o2::ft0::RecPoints b) { return a.getInteractionRecord().bc2ns() < b.getInteractionRecord().bc2ns(); });

  int ift0 = 0;

  uint32_t ft0firstOrbit = ctx.services().get<o2::framework::TimingInfo>().firstTForbit;

//  ILOG(Info, Support) << "ft0firstOrbit = " << ft0firstOrbit << " - BC = " <<  mRecoCont.startIR.bc <<ENDM;

  for (int i = 0; i < mMyTracks.size(); i++) { // loop looking for interaction candidates
    tracks.clear();
    ft0Cand.clear();

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

      // select FT0 candidates within 8 BCs
      if(mUseFT0){
	double firstTime = tracks[0].tofSignalDouble() - 8*o2::tof::Geo::BC_TIME_INPS;
	double lastTime = tracks[ntrk - 1].tofSignalDouble() + 8*o2::tof::Geo::BC_TIME_INPS;
	for(int j=ift0; j < ft0Sorted.size(); j++){
	  auto& obj = ft0Sorted[j];
	  uint32_t orbit = obj.getInteractionRecord().orbit - ft0firstOrbit;
	  double BCtimeFT0 = ((orbit)*o2::constants::lhc::LHCMaxBunches + obj.getInteractionRecord().bc)  * o2::tof::Geo::BC_TIME_INPS;

	  if(BCtimeFT0 < firstTime){
	    ift0 = j+1;
	    continue;
	  }
	  if(BCtimeFT0 > lastTime){
	    break;
	  }
//          ILOG(Info, Support) << " TOF first=" << firstTime << " < " << BCtimeFT0 << " < TOF last=" << lastTime << ENDM;
//          ILOG(Info, Support) << " orbit=" << obj.getInteractionRecord().orbit << " " << obj.getInteractionRecord().orbit - ft0firstOrbit << " BC=" << obj.getInteractionRecord().bc << ENDM;

          std::array<short, 4> collTimes = {obj.getCollisionTime(0), obj.getCollisionTime(1), obj.getCollisionTime(2), obj.getCollisionTime(3)};

          int pos = ft0Cand.size();
	  ft0Cand.emplace_back(collTimes, 0, 0, o2::InteractionRecord{obj.getInteractionRecord().bc, orbit}, obj.getTrigger());
	}
      }

      processEvent(tracks, ft0Cand);
    }
  }

  ILOG(Info, Support) << " Processed! " << ENDM;
  return;
}

void TaskFT0TOF::endOfCycle()
{

  ILOG(Info, Support) << "endOfCycle" << ENDM;
}

void TaskFT0TOF::endOfActivity(Activity& /*activity*/)
{
  ILOG(Info, Support) << "endOfActivity" << ENDM;
}

void TaskFT0TOF::reset()
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
  mHistDeltatPiEvTimeRes->Reset();
  mHistDeltatPiEvTimeMult->Reset();
  mHistEvTimeResEvTimeMult->Reset();
  mHistEvTimeTOF->Reset();
}

void TaskFT0TOF::processEvent(const std::vector<MyTrack>& tracks, const std::vector<o2::ft0::RecPoints>& ft0Cand)
{

  auto evtime = o2::tof::evTimeMaker<std::vector<MyTrack>, MyTrack, MyFilter>(tracks);
  bool isTOFst = evtime.mEventTimeError < 150;
  int nBC = (evtime.mEventTime + 5000.) * o2::tof::Geo::BC_TIME_INPS_INV;  // 5 ns offset to get the correct number of BC (int)

  for(auto& obj: ft0Cand){ // fill histo for FT0
    bool isAC = obj.isValidTime(0);
    bool isA = obj.isValidTime(1);
    bool isC = obj.isValidTime(2);
    float times[4] = {evtime.mEventTime - nBC*o2::tof::Geo::BC_TIME_INPS, isAC ? obj.getCollisionTime(0) : 0.f, isC ? obj.getCollisionTime(1) : 0.f, isC ? obj.getCollisionTime(2) : 0.f}; // TOF, FT0-AC, FT0-A, FT0-C
    if(isTOFst){
      ILOG(Info, Support) << " TOF Orbit: " << nBC / o2::constants::lhc::LHCMaxBunches << " BC: " << nBC % o2::constants::lhc::LHCMaxBunches << " FTO Orbit: " << obj.getInteractionRecord().orbit<< " BC: " << obj.getInteractionRecord().bc << ENDM;
      ILOG(Info, Support) << " Time: TOF=" << times[0] << ", FT0-AC=" << times[1] << ", FT0-A=" << times[2] << ", FT0-C=" << times[3] << ENDM;
    }
  }

  int nt = 0;
  for (auto& track : tracks) {

    float mEvTime = evtime.mEventTime;
    float mEvTimeRes = evtime.mEventTimeError;
    const auto mMultiplicity = evtime.mEventTimeMultiplicity;
    //
    evtime.removeBias<MyTrack, MyFilter>(track, nt, mEvTime, mEvTimeRes);

    // Delta t Pion
    const float deltatpi = track.tofSignal() - mEvTime - track.tofExpSignalPi();
    // Delta t Kaon
    const float deltatka = track.tofSignal() - mEvTime - track.tofExpSignalKa();
    // Delta t Proton
    const float deltatpr = track.tofSignal() - mEvTime - track.tofExpSignalPr();
    // Beta
    const float beta = track.getL() / (track.tofSignal() - mEvTime) * cinv;
    // Mass
    const float mass = track.getP() / beta * TMath::Sqrt(TMath::Abs(1 - beta * beta));
    // T0 - number of BC
    float mEvTime_BC = mEvTime - nBC * o2::tof::Geo::BC_TIME_INPS; // Event time in ps wrt BC

    mHistDeltatPi->Fill(deltatpi);
    mHistDeltatKa->Fill(deltatka);
    mHistDeltatPr->Fill(deltatpr);
    mHistDeltatPiPt->Fill(track.getPt(), deltatpi);
    mHistDeltatKaPt->Fill(track.getPt(), deltatka);
    mHistDeltatPrPt->Fill(track.getPt(), deltatpr);
    mHistMass->Fill(mass);
    mHistBetavsP->Fill(track.getP(), beta);
    mHistEvTimeTOF->Fill(mEvTime_BC);

    if (track.getP() > 0.7 && track.getP() < 1.1) {
      mHistDeltatPiEvTimeRes->Fill(mEvTimeRes, deltatpi);
      mHistDeltatPiEvTimeMult->Fill(mMultiplicity, deltatpi);
      mHistEvTimeResEvTimeMult->Fill(mMultiplicity, mEvTimeRes);
    }
  }
}

//__________________________________________________________

bool TaskFT0TOF::selectTrack(o2::tpc::TrackTPC const& track)
{
  if (track.getPt() < mMinPtCut) {
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
  if (!(const_cast<o2::tpc::TrackTPC&>(track).propagateParamToDCA(v, mBz, &dca, mMinDCAtoBeamPipeCut)) || std::abs(dca[0]) > mMinDCAtoBeamPipeCutY) {
    return false;
  }

  return true;
}

} // namespace o2::quality_control_modules::pid
