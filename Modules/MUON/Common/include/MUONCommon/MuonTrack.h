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
#include <ReconstructionDataFormats/TrackMCHMID.h>
#include <ReconstructionDataFormats/GlobalFwdTrack.h>
#include <MCHTracking/TrackParam.h>
#include <Math/Vector4D.h>
#include <memory>

namespace o2::quality_control_modules::muon
{

class MuonTrack
{
 public:
  MuonTrack(const o2::mch::TrackMCH* track, const o2::globaltracking::RecoContainer& recoCont);
  MuonTrack(const o2::dataformats::TrackMCHMID* track, const o2::globaltracking::RecoContainer& recoCont);
  MuonTrack(const o2::dataformats::GlobalFwdTrack* track, const o2::globaltracking::RecoContainer& recoCont);

  void init(const o2::globaltracking::RecoContainer& recoCont);

  ROOT::Math::PxPyPzMVector getMuonMomentum() const { return mMuonMomentum; }
  ROOT::Math::PxPyPzMVector getMuonMomentumAtVertex() const { return mMuonMomentumAtVertex; }
  double getP() const
  {
    return mMuonMomentum.P();
  }
  double getDCA() const { return mDCA; }
  double getRAbs() const { return mRAbs; }
  double getChi2OverNDF() const { return mChi2OverNDF; }

  /// get the interaction record associated to this track
  InteractionRecord getIR() const { return mIR; }
  /// get the interaction record associated to the MFT track
  InteractionRecord getIRMFT() const { return mIRMFT; }
  /// get the interaction record associated to the MCH track
  InteractionRecord getIRMCH() const { return mIRMCH; }
  /// get the interaction record associated to the MID track
  InteractionRecord getIRMID() const { return mIRMID; }

  /// get the interaction record associated to the MFT track
  int getTrackIdMFT() const { return mTrackIdMFT; }
  /// get the interaction record associated to the MCH track
  int getTrackIdMCH() const { return mTrackIdMCH; }
  /// get the interaction record associated to the MID track
  int getTrackIdMID() const { return mTrackIdMID; }

  bool hasMFT() const { return (mTrackIdMFT >= 0); }
  bool hasMCH() const { return (mTrackIdMCH >= 0); }
  bool hasMID() const { return (mTrackIdMID >= 0); }

  /// get the muon sign
  short getSign() const { return mSign; }

  static constexpr double sAbsZBeg = -90.;  ///< Position of the begining of the absorber (cm)
  static constexpr double sAbsZEnd = -505.; ///< Position of the end of the absorber (cm)

 private:
  o2::mch::TrackParam mTrackParameters;
  o2::mch::TrackParam mTrackParametersMCH;

  ROOT::Math::PxPyPzMVector mMuonMomentum;
  ROOT::Math::PxPyPzMVector mMuonMomentumAtVertex;

  float mDCA{ 0 };
  float mRAbs{ 0 };
  float mChi2OverNDF{ 0 };

  InteractionRecord mIR{};    ///< associated interaction record
  InteractionRecord mIRMFT{}; ///< MFT interaction record
  InteractionRecord mIRMCH{}; ///< MCH interaction record
  InteractionRecord mIRMID{}; ///< MID interaction record

  int mTrackIdMFT{ -1 };
  int mTrackIdMCH{ -1 };
  int mTrackIdMID{ -1 };

  short mSign;
};

} // namespace o2::quality_control_modules::muon

#endif
