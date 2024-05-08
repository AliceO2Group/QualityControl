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

#include "MUONCommon/MuonTrack.h"
#include "QualityControl/QcInfoLogger.h"

#include <DataFormatsITSMFT/ROFRecord.h>
#include <DataFormatsMCH/Cluster.h>
#include <DataFormatsMCH/ROFRecord.h>
#include <DataFormatsMFT/TrackMFT.h>
#include <DataFormatsMCH/TrackMCH.h>
#include <DataFormatsMID/ROFRecord.h>
#include <DataFormatsMID/Track.h>
#include <ReconstructionDataFormats/TrackMCHMID.h>
#include <ReconstructionDataFormats/GlobalFwdTrack.h>
#include <DetectorsBase/GeometryManager.h>
#include <Framework/DataRefUtils.h>
#include <Framework/InputRecord.h>
#include <MCHMappingInterface/Segmentation.h>
#include <MCHTracking/TrackExtrap.h>
#include <Math/Vector4D.h>
#include <Math/Vector4Dfwd.h>
#include <TH1F.h>
#include <TMath.h>
#include <gsl/span>
#include <set>

using namespace o2::dataformats;
using TrackMFT = o2::mft::TrackMFT;
using TrackMCH = o2::mch::TrackMCH;
using TrackMID = o2::mid::Track;
using InteractionRecord = o2::InteractionRecord;

namespace
{

using namespace o2::quality_control_modules::muon;

constexpr double muonMass = 0.1056584;
constexpr double muonMass2 = muonMass * muonMass;

static InteractionRecord getMFTTrackIR(int iTrack, const o2::globaltracking::RecoContainer& recoCont)
{
  // if the MID track is present, use the time from MID
  auto rofs = recoCont.getMFTTracksROFRecords();
  for (const auto& rof : rofs) {
    if (iTrack < rof.getFirstEntry() || iTrack >= (rof.getFirstEntry() + rof.getNEntries())) {
      continue;
    }
    return rof.getBCData();
  }

  return InteractionRecord{};
}

static MuonTrack::Time getMFTTrackTime(int iTrack, const o2::globaltracking::RecoContainer& recoCont, uint32_t firstTForbit)
{
  // if the MID track is present, use the time from MID
  auto rofs = recoCont.getMFTTracksROFRecords();
  for (const auto& rof : rofs) {
    if (iTrack < rof.getFirstEntry() || iTrack >= (rof.getFirstEntry() + rof.getNEntries())) {
      continue;
    }
    auto bcDiff = rof.getBCData().differenceInBC(InteractionRecord{ 0, firstTForbit });
    float tMean = (bcDiff + 0.5) * o2::constants::lhc::LHCBunchSpacingMUS;
    float tErr = 1.0 * o2::constants::lhc::LHCBunchSpacingMUS;
    return { tMean, tErr };
  }

  return MuonTrack::Time{};
}

static InteractionRecord getMIDTrackIR(int iTrack, const o2::globaltracking::RecoContainer& recoCont)
{
  // if the MID track is present, use the time from MID
  auto rofs = recoCont.getMIDTracksROFRecords();
  for (const auto& rof : rofs) {
    if (iTrack < rof.firstEntry || iTrack >= (rof.firstEntry + rof.nEntries)) {
      continue;
    }
    return rof.interactionRecord;
  }

  return InteractionRecord{};
}

static MuonTrack::Time getMIDTrackTime(int iTrack, const o2::globaltracking::RecoContainer& recoCont, uint32_t firstTForbit)
{
  // if the MID track is present, use the time from MID
  auto rofs = recoCont.getMIDTracksROFRecords();
  for (const auto& rof : rofs) {
    if (iTrack < rof.firstEntry || iTrack >= (rof.firstEntry + rof.nEntries)) {
      continue;
    }
    auto time = rof.getTimeMUS(InteractionRecord{ 0, firstTForbit });
    if (time.second) {
      return time.first;
    } else {
      return MuonTrack::Time{};
    }
  }

  return MuonTrack::Time{};
}

static o2::mch::ROFRecord getMCHTrackROF(int iTrack, const o2::globaltracking::RecoContainer& recoCont)
{
  // if the MID track is present, use the time from MID
  auto rofs = recoCont.getMCHTracksROFRecords();
  for (const auto& rof : rofs) {
    if (iTrack < rof.getFirstIdx() || iTrack > rof.getLastIdx()) {
      continue;
    }
    return rof;
  }

  return o2::mch::ROFRecord{};
}

static InteractionRecord getMCHTrackIR(int iTrack, const o2::globaltracking::RecoContainer& recoCont, uint32_t firstTForbit)
{
  auto tracksMCH = recoCont.getMCHTracks();
  if (iTrack < 0 || iTrack >= tracksMCH.size()) {
    return InteractionRecord{};
  }
  return tracksMCH[iTrack].getMeanIR(firstTForbit);

  // if the MID track is present, use the time from MID
  auto rofs = recoCont.getMCHTracksROFRecords();
  for (const auto& rof : rofs) {
    if (iTrack < rof.getFirstIdx() || iTrack > rof.getLastIdx()) {
      continue;
    }
    return rof.getBCData();
  }

  return InteractionRecord{};
}

static InteractionRecord getMCHTrackIR(const o2::mch::TrackMCH* track, const o2::globaltracking::RecoContainer& recoCont, uint32_t firstTForbit)
{
  return track->getMeanIR(firstTForbit);

  // if the MID track is present, use the time from MID
  auto tracksMCH = recoCont.getMCHTracks();
  int iMCH{ -1 };
  for (int i = 0; i < tracksMCH.size(); i++) {
    if (track == (&(tracksMCH[i]))) {
      iMCH = i;
      break;
    }
  }
  if (iMCH >= 0) {
    return getMCHTrackIR(iMCH, recoCont, firstTForbit);
  }

  return InteractionRecord{};
}

static MuonTrack::Time getMCHTrackTime(int iTrack, const o2::globaltracking::RecoContainer& recoCont, uint32_t firstTForbit)
{
  auto tracksMCH = recoCont.getMCHTracks();
  if (iTrack < 0 || iTrack >= tracksMCH.size()) {
    return MuonTrack::Time{};
  }
  return tracksMCH[iTrack].getTimeMUS();
}

static MuonTrack::Time getMCHTrackTime(const o2::mch::TrackMCH* track, const o2::globaltracking::RecoContainer& recoCont, uint32_t firstTForbit)
{
  return track->getTimeMUS();
}

static InteractionRecord getGlobalFwdTrackIR(const GlobalFwdTrack* track, const o2::globaltracking::RecoContainer& recoCont, uint32_t firstTForbit)
{
  auto iMID = track->getMIDTrackID();
  auto tracksMID = recoCont.getMIDTracks();
  if (iMID >= 0 && iMID < tracksMID.size()) {
    // if the MID track is present, use the time from MID
    return getMIDTrackIR(iMID, recoCont);
  }

  auto iMCH = track->getMCHTrackID();
  auto tracksMCH = recoCont.getMCHTracks();
  if (iMCH >= 0 && iMCH < tracksMCH.size()) {
    // if the MID track is present, use the time from MID
    return getMCHTrackIR(iMCH, recoCont, firstTForbit);
  }

  return InteractionRecord{};
}

static MuonTrack::Time getGlobalFwdTrackTime(const GlobalFwdTrack* track, const o2::globaltracking::RecoContainer& recoCont, uint32_t firstTForbit)
{
  auto iMID = track->getMIDTrackID();
  auto tracksMID = recoCont.getMIDTracks();
  if (iMID >= 0 && iMID < tracksMID.size()) {
    // if the MID track is present, use the time from MID
    return getMIDTrackTime(iMID, recoCont, firstTForbit);
  }

  auto iMCH = track->getMCHTrackID();
  auto tracksMCH = recoCont.getMCHTracks();
  if (iMCH >= 0 && iMCH < tracksMCH.size()) {
    // if the MID track is present, use the time from MID
    return getMCHTrackTime(iMCH, recoCont, firstTForbit);
  }

  return MuonTrack::Time{};
}

static bool getParametersAtVertex(o2::mch::TrackParam& trackParamAtVertex, bool correctForMCS = true)
{
  bool result = false;
  if (trackParamAtVertex.getZ() > o2::quality_control_modules::muon::MuonTrack::sAbsZBeg) {
    result = o2::mch::TrackExtrap::extrapToZ(trackParamAtVertex, 0);
  } else {
    if (correctForMCS) {
      result = o2::mch::TrackExtrap::extrapToVertex(trackParamAtVertex, 0.0, 0.0, 0.0, 0.0, 0.0);
    } else {
      result = o2::mch::TrackExtrap::extrapToVertexWithoutBranson(trackParamAtVertex, 0.0);
    }
  }
  return result;
}

static ROOT::Math::PxPyPzMVector getMuonMomentum(const o2::mch::TrackParam& par)
{
  return { par.px(), par.py(), par.pz(), muonMass };
}

static ROOT::Math::PxPyPzMVector getMuonMomentumAtVertex(const o2::mch::TrackParam& par)
{
  o2::mch::TrackParam trackParamAtVertex = par;
  bool result = getParametersAtVertex(trackParamAtVertex);
  if (!result) {
    ILOG(Error, Devel) << "Track extrap failed" << ENDM;
    return ROOT::Math::PxPyPzMVector();
  }

  double px = trackParamAtVertex.px();
  double py = trackParamAtVertex.py();
  double pz = trackParamAtVertex.pz();
  return { px, py, pz, muonMass };
}

static float getDCA(const o2::mch::TrackParam& par)
{
  o2::mch::TrackParam trackParamAtDCA = par;
  bool result = getParametersAtVertex(trackParamAtDCA, false);
  if (!result) {
    ILOG(Error, Devel) << "Track extrap failed" << ENDM;
    return 0;
  }

  double dcaX = trackParamAtDCA.getBendingCoor();
  double dcaY = trackParamAtDCA.getNonBendingCoor();
  return std::sqrt(dcaX * dcaX + dcaY * dcaY);
}

static float getPDCA(const o2::mch::TrackParam& par)
{
  auto p = par.p();
  auto dca = getDCA(par);
  return (p * dca);
}

static float getRAbsMCH(const o2::mch::TrackParam& par)
{
  o2::mch::TrackParam trackParamAtAbs = par;
  if (!o2::mch::TrackExtrap::extrapToZ(trackParamAtAbs, o2::quality_control_modules::muon::MuonTrack::sAbsZEnd)) {
    return 0;
  }
  double xAbs = trackParamAtAbs.getNonBendingCoor();
  double yAbs = trackParamAtAbs.getBendingCoor();
  return std::sqrt(xAbs * xAbs + yAbs * yAbs);
}

static o2::mch::TrackParam forwardTrackToMCHTrack(const o2::track::TrackParFwd& track)
{
  const auto phi = track.getPhi();
  const auto sinPhi = std::sin(phi);
  const auto tgL = track.getTgl();

  const auto SlopeX = std::cos(phi) / tgL;
  const auto SlopeY = sinPhi / tgL;
  const auto InvP_yz = track.getInvQPt() / std::sqrt(sinPhi * sinPhi + tgL * tgL);

  const std::array<Double_t, 5> params{ track.getX(), SlopeX, track.getY(), SlopeY, InvP_yz };
  const std::array<Double_t, 15> cov{
    1,
    0, 1,
    0, 0, 1,
    0, 0, 0, 1,
    0, 0, 0, 0, 1
  };

  return { track.getZ(), params.data(), cov.data() };
}

static o2::mch::TrackParam forwardTrackToMCHTrack(const o2::track::TrackParCovFwd& track)
{
  const auto phi = track.getPhi();
  const auto sinPhi = std::sin(phi);
  const auto tgL = track.getTgl();

  const auto SlopeX = std::cos(phi) / tgL;
  const auto SlopeY = sinPhi / tgL;
  const auto InvP_yz = track.getInvQPt() / std::sqrt(sinPhi * sinPhi + tgL * tgL);

  const std::array<Double_t, 5> params{ track.getX(), SlopeX, track.getY(), SlopeY, InvP_yz };
  const std::array<Double_t, 15> cov{
    1,
    0, 1,
    0, 0, 1,
    0, 0, 0, 1,
    0, 0, 0, 0, 1
  };

  return { track.getZ(), params.data(), cov.data() };
}

} // namespace

//_________________________________________________________________________________________________________

namespace o2::quality_control_modules::muon
{

MuonTrack::MuonTrack(const o2::mch::TrackMCH* track, int trackID, const o2::globaltracking::RecoContainer& recoCont, uint32_t firstTForbit)
{
  mTrackMCH = track;

  mTrackIdMCH = trackID;

  mChi2OverNDF = track->getChi2OverNDF();

  mIR = getMCHTrackIR(track, recoCont, firstTForbit);
  mIRMCH = mIR;

  mTime = track->getTimeMUS();
  mTimeMCH = mTime;

  o2::InteractionRecord startIR{ 0, firstTForbit };
  mRofMCH = getMCHTrackROF(mTrackIdMCH, recoCont);
  mRofTimeMCH = mRofMCH.getTimeMUS(startIR, 32).first;

  mTrackParameters.setZ(track->getZ());
  mTrackParameters.setParameters(track->getParameters());

  mTrackParametersMCH.setZ(track->getZ());
  mTrackParametersMCH.setParameters(track->getParameters());

  mTrackParametersAtMID.setZ(track->getZAtMID());
  mTrackParametersAtMID.setParameters(track->getParametersAtMID());

  mChi2OverNDFMCH = mTrackMCH->getChi2OverNDF();

  init();
}

MuonTrack::MuonTrack(const TrackMCHMID* track, const o2::globaltracking::RecoContainer& recoCont, uint32_t firstTForbit)
{
  auto tracksMCH = recoCont.getMCHTracks();
  auto tracksMID = recoCont.getMIDTracks();

  auto iMCH = track->getMCHRef().getIndex();
  auto iMID = track->getMIDRef().getIndex();

  if (iMCH >= 0 && iMCH >= tracksMCH.size()) {
    throw std::length_error(fmt::format("[MuonTrackImpl<GlobalFwdTrack>] bad MCH track index: iMCH={}  tracksMCH.size()={}", iMCH, tracksMCH.size()));
  }
  if (iMID >= 0 && iMID >= tracksMID.size()) {
    throw std::length_error(fmt::format("[MuonTrackImpl<GlobalFwdTrack>] bad MID track index: iMID={}  tracksMID.size()={}", iMID, tracksMID.size()));
  }

  mTrackIdMCH = iMCH;
  mTrackIdMID = iMID;

  mTrackMCH = &(tracksMCH[iMCH]);
  mTrackMID = &(tracksMID[iMID]);

  mIR = track->getIR();
  auto trackTimeMUS = track->getTimeMUS(InteractionRecord{ 0, firstTForbit }, 128, true);
  if (trackTimeMUS.second) {
    mTime = trackTimeMUS.first;
  }

  auto& trackMCH = tracksMCH[iMCH];

  if (iMCH >= 0) {
    // mTrackMCH = &(tracksMCH[iMCH]);
    mIRMCH = getMCHTrackIR(iMCH, recoCont, firstTForbit);
    mTimeMCH = trackMCH.getTimeMUS();

    o2::InteractionRecord startIR{ 0, firstTForbit };
    mRofMCH = getMCHTrackROF(mTrackIdMCH, recoCont);
    mRofTimeMCH = mRofMCH.getTimeMUS(startIR, 32).first;

    mChi2OverNDFMCH = mTrackMCH->getChi2OverNDF();
  }
  if (iMID >= 0) {
    // mTrackMID = &(tracksMID[iMID]);
    mIRMID = getMIDTrackIR(iMID, recoCont);
    if (trackTimeMUS.second) {
      mTimeMID = trackTimeMUS.first;
    }

    mChi2OverNDFMID = mTrackMID->getChi2OverNDF();
  }

  mChi2OverNDF = trackMCH.getChi2OverNDF();

  mTrackParameters.setZ(trackMCH.getZ());
  mTrackParameters.setParameters(trackMCH.getParameters());

  mTrackParametersMCH.setZ(trackMCH.getZ());
  mTrackParametersMCH.setParameters(trackMCH.getParameters());

  mTrackParametersAtMID.setZ(trackMCH.getZAtMID());
  mTrackParametersAtMID.setParameters(trackMCH.getParametersAtMID());

  init();
}

MuonTrack::MuonTrack(const GlobalFwdTrack* track, const o2::globaltracking::RecoContainer& recoCont, uint32_t firstTForbit) : mTrackParameters(forwardTrackToMCHTrack(*track))
{
  auto tracksMFT = recoCont.getMFTTracks();
  auto tracksMCH = recoCont.getMCHTracks();
  auto tracksMID = recoCont.getMIDTracks();

  auto iMFT = track->getMFTTrackID();
  auto iMCH = track->getMCHTrackID();
  auto iMID = track->getMIDTrackID();

  if (iMFT >= 0 && iMFT >= tracksMFT.size()) {
    throw std::length_error(fmt::format("[MuonTrack(GlobalFwdTrack)] bad MFT track index: iMFT={}  tracksMFT.size()={}", iMFT, tracksMFT.size()));
  }
  if (iMCH >= 0 && iMCH >= tracksMCH.size()) {
    throw std::length_error(fmt::format("[MuonTrack(GlobalFwdTrack)] bad MCH track index: iMCH={}  tracksMCH.size()={}", iMCH, tracksMCH.size()));
  }
  if (iMID >= 0 && iMID >= tracksMID.size()) {
    throw std::length_error(fmt::format("[MuonTrack(GlobalFwdTrack)] bad MID track index: iMID={}  tracksMID.size()={}", iMID, tracksMID.size()));
  }

  mMatchInfoFwd = *track;

  mTrackIdMFT = iMFT;
  mTrackIdMCH = iMCH;
  mTrackIdMID = iMID;

  if (iMFT >= 0) {
    mTrackMFT = &(tracksMFT[iMFT]);
    mIRMFT = getMFTTrackIR(iMFT, recoCont);
    mTimeMFT = getMFTTrackTime(iMFT, recoCont, firstTForbit);
    mTrackParametersMFT.setZ(tracksMFT[iMFT].getOutParam().getZ());
    mTrackParametersMFT.setParameters(forwardTrackToMCHTrack(tracksMFT[iMFT].getOutParam()).getParameters());
  }
  if (iMCH >= 0) {
    mTrackMCH = &(tracksMCH[iMCH]);
    auto& trackMCH = tracksMCH[iMCH];
    mTrackParametersMCH.setZ(trackMCH.getZ());
    mTrackParametersMCH.setParameters(trackMCH.getParameters());

    mTrackParametersAtMID.setZ(trackMCH.getZAtMID());
    mTrackParametersAtMID.setParameters(trackMCH.getParametersAtMID());

    mIRMCH = getMCHTrackIR(iMCH, recoCont, firstTForbit);
    mTimeMCH = getMCHTrackTime(iMCH, recoCont, firstTForbit);

    o2::InteractionRecord startIR{ 0, firstTForbit };
    mRofMCH = getMCHTrackROF(mTrackIdMCH, recoCont);
    mRofTimeMCH = mRofMCH.getTimeMUS(startIR, 32).first;

    mChi2OverNDFMCH = mTrackMCH->getChi2OverNDF();
  }
  if (iMID >= 0) {
    mTrackMID = &(tracksMID[iMID]);
    mIRMID = getMIDTrackIR(iMID, recoCont);
    mTimeMID = getMIDTrackTime(iMID, recoCont, firstTForbit);

    mChi2OverNDFMID = mTrackMID->getChi2OverNDF();
  }

  mIR = ::getGlobalFwdTrackIR(track, recoCont, firstTForbit);
  mTime = ::getGlobalFwdTrackTime(track, recoCont, firstTForbit);

  // mChi2OverNDF = track->getTrackChi2();
  mChi2OverNDF = mTrackMCH->getChi2OverNDF();

  init();
}

void MuonTrack::init()
{
  mSign = mTrackParameters.getCharge();

  mMuonMomentum = ::getMuonMomentum(mTrackParameters);
  mMuonMomentumAtVertex = ::getMuonMomentumAtVertex(mTrackParameters);

  mMuonMomentumMCH = ::getMuonMomentum(mTrackParametersMCH);
  mMuonMomentumAtVertexMCH = ::getMuonMomentumAtVertex(mTrackParametersMCH);

  mDCA = ::getDCA(mTrackParameters);
  mDCAMCH = ::getDCA(mTrackParametersMCH);
  mRAbs = getRAbsMCH(mTrackParametersMCH);
}

bool MuonTrack::extrapToZMFT(o2::mch::TrackParam& trackParam, float z) const
{
  trackParam = mTrackParametersMFT;
  return o2::mch::TrackExtrap::extrapToZ(trackParam, z);
}

bool MuonTrack::extrapToZMCH(o2::mch::TrackParam& trackParam, float z) const
{
  trackParam = mTrackParametersMCH;
  return o2::mch::TrackExtrap::extrapToZ(trackParam, z);
}

bool MuonTrack::extrapToZMID(o2::mch::TrackParam& trackParam, float z) const
{
  trackParam = mTrackParametersMID;
  return o2::mch::TrackExtrap::extrapToZ(trackParam, z);
}

double MuonTrack::getP() const
{
  return mMuonMomentumAtVertex.P();
}

double MuonTrack::getPt() const
{
  return mMuonMomentumAtVertex.Pt();
}

double MuonTrack::getEta() const
{
  return mMuonMomentumAtVertex.eta();
}

double MuonTrack::getPhi() const
{
  return mMuonMomentumAtVertex.phi() * TMath::RadToDeg();
}

double MuonTrack::getPMCH() const
{
  return mMuonMomentumAtVertexMCH.P();
}

double MuonTrack::getPtMCH() const
{
  return mMuonMomentumAtVertexMCH.Pt();
}

double MuonTrack::getEtaMCH() const
{
  return mMuonMomentumAtVertexMCH.eta();
}

double MuonTrack::getPhiMCH() const
{
  return mMuonMomentumAtVertexMCH.phi() * TMath::RadToDeg();
}

bool MuonTrack::canBeMuon() const
{
  bool inABS = (mRAbs > 17.6) && (mRAbs < 89.5);
  bool inHoleMID = (std::abs(getXMid()) < 50) && (std::abs(getYMid()) < 50);
  bool outOfMID = (std::abs(getXMid()) > 250) || (std::abs(getYMid()) > 300);
  bool inMID = (!inHoleMID) && (!outOfMID);
  // return (inABS);
  return (inABS && inMID);
  // return true;
  // return (mRAbs > 17.6 && mRAbs < 89.5 && std::abs(getXMid()) > 50 && std::abs(getXMid()) < 250 && std::abs(getYMid()) > 50 && std::abs(getYMid()) < 300);
}

} // namespace o2::quality_control_modules::muon
