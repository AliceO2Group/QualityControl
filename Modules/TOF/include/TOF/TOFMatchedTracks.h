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
/// \file   TOFMatchedTracks.h
/// \author Chiara Zampolli
/// \brief  Task to monitor TOF matching efficiency
/// \since  03/08/2021
///

#ifndef QC_MODULE_TOF_TOFTOFMATCHEDTRACKS_H
#define QC_MODULE_TOF_TOFTOFMATCHEDTRACKS_H

#include "QualityControl/TaskInterface.h"

#include "DataFormatsGlobalTracking/RecoContainer.h"
#include "ReconstructionDataFormats/MatchInfoTOFReco.h"
#include "SimulationDataFormat/MCCompLabel.h"
#include "DataFormatsTRD/TrackTRD.h"

class TH1F;
class TH2F;
class TEfficiency;

namespace o2::quality_control_modules::tof
{

using namespace o2::quality_control::core;
using GID = o2::dataformats::GlobalTrackID;

/// \brief  Task for the control of the TOF matching efficiency
/// \author Chiara Zampolli
class TOFMatchedTracks final : public TaskInterface
{
 public:
  enum matchType : int8_t { TPC = 0,
                            ITSTPC_ITSTPCTRD,
                            TPCTRD,
                            SIZE };

  /// \brief Constructor
  TOFMatchedTracks() = default;
  /// Destructor
  ~TOFMatchedTracks() override;

  // Definition of the methods for the template method pattern
  void initialize(o2::framework::InitContext& ctx) override;
  void startOfActivity(const Activity& activity) override;
  void startOfCycle() override;
  void monitorData(o2::framework::ProcessingContext& ctx) override;
  void endOfCycle() override;
  void endOfActivity(const Activity& activity) override;
  void reset() override;

  // track selection
  bool selectTrack(o2::tpc::TrackTPC const& track);
  void setPtCut(float v) { mPtCut = v; }
  void setEtaCut(float v) { mEtaCut = v; }
  void setMinNTPCClustersCut(float v) { mNTPCClustersCut = v; }
  void setMinDCAtoBeamPipeCut(std::array<float, 2> v)
  {
    setMinDCAtoBeamPipeDistanceCut(v[0]);
    setMinDCAtoBeamPipeYCut(v[1]);
  }
  void setMinDCAtoBeamPipeDistanceCut(float v) { mDCACut = v; }
  void setMinDCAtoBeamPipeYCut(float v) { mDCACutY = v; }

 private:
  std::shared_ptr<o2::globaltracking::DataRequest> mDataRequest;
  o2::globaltracking::RecoContainer mRecoCont;
  GID::mask_t mSrc = GID::getSourcesMask("ITS-TPC");
  GID::mask_t mAllowedSources = GID::getSourcesMask("TPC,ITS-TPC,TPC-TOF,ITS-TPC-TOF,TPC-TRD,ITS-TPC-TRD-TOF,TPC-TRD-TOF,ITS-TPC-TRD");
  // TPC-TOF
  gsl::span<const o2::tpc::TrackTPC> mTPCTracks;
  gsl::span<const o2::dataformats::MatchInfoTOF> mTPCTOFMatches;
  // ITS-TPC-TOF
  gsl::span<const o2::dataformats::TrackTPCITS> mITSTPCTracks;
  gsl::span<const o2::dataformats::MatchInfoTOF> mITSTPCTOFMatches;
  // TPC-TRD-TOF
  gsl::span<const o2::trd::TrackTRD> mTPCTRDTracks;
  gsl::span<const o2::dataformats::MatchInfoTOF> mTPCTRDTOFMatches;
  // TPC-TRD-TOF
  gsl::span<const o2::trd::TrackTRD> mITSTPCTRDTracks;
  gsl::span<const o2::dataformats::MatchInfoTOF> mITSTPCTRDTOFMatches;

  bool mUseMC = false;
  bool mVerbose = false;
  TH1F* mInTracksPt[matchType::SIZE] = {};
  TH1F* mInTracksEta[matchType::SIZE] = {};
  TH2F* mInTracks2DPtEta[matchType::SIZE] = {};
  TH1F* mMatchedTracksPt[matchType::SIZE] = {};
  TH1F* mMatchedTracksEta[matchType::SIZE] = {};
  TH2F* mMatchedTracks2DPtEta[matchType::SIZE] = {};
  TH1F* mFakeMatchedTracksPt[matchType::SIZE] = {};
  TH1F* mFakeMatchedTracksEta[matchType::SIZE] = {};
  TH2F* mDeltaZEta[matchType::SIZE] = {};
  TH2F* mDeltaZPhi[matchType::SIZE] = {};
  TH2F* mDeltaZPt[matchType::SIZE] = {};
  TH2F* mDeltaXEta[matchType::SIZE] = {};
  TH2F* mDeltaXPhi[matchType::SIZE] = {};
  TH2F* mDeltaXPt[matchType::SIZE] = {};
  TH1F* mTOFChi2[matchType::SIZE] = {};
  TH2F* mTOFChi2Pt[matchType::SIZE] = {};
  TH2F* mDTimeTrk[18] = {};
  TH2F* mDTimeTrkTPC[18] = {};
  TH2F* mDTimeTrkTRD[18] = {};

  TEfficiency* mEffPt[matchType::SIZE] = {};
  TEfficiency* mEffEta[matchType::SIZE] = {};
  TEfficiency* mEff2DPtEta[matchType::SIZE] = {};
  TEfficiency* mFakeFractionTracksPt[matchType::SIZE] = {};  // fraction of fakes among the matched tracks vs pT
  TEfficiency* mFakeFractionTracksEta[matchType::SIZE] = {}; // fraction of fakes among the matched tracks vs Eta

  // for track selection
  float mPtCut = 0.1f;
  float mEtaCut = 0.8f;
  int32_t mNTPCClustersCut = 40;
  float mDCACut = 100.f;
  float mDCACutY = 10.f;
  float mBz = 0; ///< nominal Bz

  int mTF = -1; // to count the number of processed TFs
};

} // namespace o2::quality_control_modules::tof

#endif // QC_MODULE_TOF_TOFTOFMATCHEDTRACKS_H
