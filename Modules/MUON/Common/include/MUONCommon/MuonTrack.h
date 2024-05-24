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

#ifndef QC_MODULE_MUON_COMMON_TRACK_H
#define QC_MODULE_MUON_COMMON_TRACK_H

#include <CommonDataFormat/InteractionRecord.h>
#include <DataFormatsGlobalTracking/RecoContainer.h>
#include <DataFormatsMCH/TrackMCH.h>
#include <DataFormatsMCH/ROFRecord.h>
#include <DataFormatsMID/Track.h>
#include <ReconstructionDataFormats/TrackMCHMID.h>
#include <ReconstructionDataFormats/GlobalFwdTrack.h>
#include <CommonDataFormat/TimeStamp.h>
#include <MCHTracking/TrackParam.h>
#include <Math/Vector4D.h>
#include <memory>

namespace o2::quality_control_modules::muon
{

class MuonTrack
{
 public:
  using Time = o2::dataformats::TimeStampWithError<float, float>;

 public:
  MuonTrack() = default;
  MuonTrack(const o2::mch::TrackMCH* track, int trackID, const o2::globaltracking::RecoContainer& recoCont, uint32_t firstTForbit);
  MuonTrack(const o2::dataformats::TrackMCHMID* track, const o2::globaltracking::RecoContainer& recoCont, uint32_t firstTForbit);
  MuonTrack(const o2::dataformats::GlobalFwdTrack* track, const o2::globaltracking::RecoContainer& recoCont, uint32_t firstTForbit);

  void init();

  ROOT::Math::PxPyPzMVector getMuonMomentum() const { return mMuonMomentum; }
  ROOT::Math::PxPyPzMVector getMuonMomentumAtVertex() const { return mMuonMomentumAtVertex; }
  ROOT::Math::PxPyPzMVector getMuonMomentumMCH() const { return mMuonMomentumMCH; }
  ROOT::Math::PxPyPzMVector getMuonMomentumAtVertexMCH() const { return mMuonMomentumAtVertexMCH; }
  double getP() const;
  double getPt() const;
  double getEta() const;
  double getPhi() const;
  double getDCA() const { return mDCA; }
  double getPDCA() const { return getMuonMomentum().P() * mDCA; }
  double getPMCH() const;
  double getPtMCH() const;
  double getEtaMCH() const;
  double getPhiMCH() const;
  double getDCAMCH() const { return mDCAMCH; }
  double getPDCAMCH() const { return getMuonMomentumMCH().P() * mDCAMCH; }
  double getRAbs() const { return mRAbs; }
  double getChi2OverNDF() const { return mChi2OverNDF; }
  double getChi2OverNDFMFT() const { return mChi2OverNDFMFT; }
  double getChi2OverNDFMCH() const { return mChi2OverNDFMCH; }
  double getChi2OverNDFMID() const { return mChi2OverNDFMID; }

  /// get the track x position
  double getXMid() const { return mTrackParametersAtMID.getNonBendingCoor(); }
  /// get the track y position
  double getYMid() const { return mTrackParametersAtMID.getBendingCoor(); }
  /// get the track z position where the parameters are evaluated
  double getZMid() const { return mTrackParametersAtMID.getZ(); }

  const o2::dataformats::MatchInfoFwd& getMatchInfoFwd() const { return mMatchInfoFwd; }

  /// get the interaction record associated to this track
  InteractionRecord getIR() const { return mIR; }
  /// get the interaction record associated to the MFT track
  InteractionRecord getIRMFT() const { return mIRMFT; }
  /// get the interaction record associated to the MCH track
  InteractionRecord getIRMCH() const { return mIRMCH; }
  /// get the interaction record associated to the MID track
  InteractionRecord getIRMID() const { return mIRMID; }

  Time getTime() const { return mTime; }
  Time getTimeMFT() const { return mTimeMFT; }
  Time getTimeMCH() const { return mTimeMCH; }
  Time getTimeMID() const { return mTimeMID; }

  /// get the ROF associated to the MCH track
  o2::mch::ROFRecord getRofMCH() const { return mRofMCH; }

  Time getRofTimeMCH() const { return mRofTimeMCH; }

  /// get the interaction record associated to the MFT track
  int getTrackIdMFT() const { return mTrackIdMFT; }
  /// get the interaction record associated to the MCH track
  int getTrackIdMCH() const { return mTrackIdMCH; }
  /// get the interaction record associated to the MID track
  int getTrackIdMID() const { return mTrackIdMID; }

  /// get the interaction record associated to the MCH track
  const o2::mft::TrackMFT* getTrackMFT() const { return mTrackMFT; }
  /// get the interaction record associated to the MCH track
  const o2::mch::TrackMCH* getTrackMCH() const { return mTrackMCH; }
  /// get the interaction record associated to the MID track
  const o2::mid::Track* getTrackMID() const { return mTrackMID; }

  const o2::mch::TrackParam& getTrackParamMFT() const { return mTrackParametersMFT; }
  const o2::mch::TrackParam& getTrackParamMCH() const { return mTrackParametersMCH; }
  const o2::mch::TrackParam& getTrackParamMID() const { return mTrackParametersMID; }

  bool extrapToZMFT(o2::mch::TrackParam& trackParam, float z) const;
  bool extrapToZMCH(o2::mch::TrackParam& trackParam, float z) const;
  bool extrapToZMID(o2::mch::TrackParam& trackParam, float z) const;

  bool hasMFT() const { return (mTrackIdMFT >= 0); }
  bool hasMCH() const { return (mTrackIdMCH >= 0); }
  bool hasMID() const { return (mTrackIdMID >= 0); }

  /// get the muon sign
  short getSign() const { return mSign; }

  bool canBeMuon() const;

  static constexpr double sAbsZBeg = -90.;  ///< Position of the begining of the absorber (cm)
  static constexpr double sAbsZEnd = -505.; ///< Position of the end of the absorber (cm)

 private:
  o2::dataformats::MatchInfoFwd mMatchInfoFwd;

  o2::mch::TrackParam mTrackParameters;
  o2::mch::TrackParam mTrackParametersMFT;
  o2::mch::TrackParam mTrackParametersMCH;
  o2::mch::TrackParam mTrackParametersMID;
  o2::mch::TrackParam mTrackParametersAtMID;

  ROOT::Math::PxPyPzMVector mMuonMomentum;
  ROOT::Math::PxPyPzMVector mMuonMomentumAtVertex;
  ROOT::Math::PxPyPzMVector mMuonMomentumMCH;
  ROOT::Math::PxPyPzMVector mMuonMomentumAtVertexMCH;

  float mDCA{ 0 };
  float mDCAMCH{ 0 };
  float mRAbs{ 0 };
  float mChi2OverNDF{ 0 };
  float mChi2OverNDFMFT{ 0 };
  float mChi2OverNDFMCH{ 0 };
  float mChi2OverNDFMID{ 0 };

  InteractionRecord mIR{};    ///< associated interaction record
  InteractionRecord mIRMFT{}; ///< MFT interaction record
  InteractionRecord mIRMCH{}; ///< MCH interaction record
  InteractionRecord mIRMID{}; ///< MID interaction record

  Time mTime;
  Time mTimeMFT;
  Time mTimeMCH;
  Time mTimeMID;

  o2::mch::ROFRecord mRofMCH{};

  Time mRofTimeMCH;

  int mTrackIdMFT{ -1 };
  int mTrackIdMCH{ -1 };
  int mTrackIdMID{ -1 };

  const o2::mft::TrackMFT* mTrackMFT{ nullptr };
  const o2::mch::TrackMCH* mTrackMCH{ nullptr };
  const o2::mid::Track* mTrackMID{ nullptr };

  short mSign;
};

} // namespace o2::quality_control_modules::muon

#endif
