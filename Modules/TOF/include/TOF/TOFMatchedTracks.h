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

  void fillDenominatorForExperts(float eta, float phi);
  void fillNumeratorForExperts(float eta, float phi, int channel, int nhit, float dx, float dz, float chi2);
  float mThresholdPtForExperts = 2.0;
  static constexpr Float_t ETAFROMSTRIP[91] = { 0.852408, 0.833555, 0.814697, 0.795648, 0.777002, 0.758214, 0.739368, 0.720569, 0.701740, 0.682949, 0.664247, 0.645404, 0.626585, 0.607850, 0.589007, 0.570191, 0.551303, 0.532452, 0.514311, 0.493487, 0.474700, 0.455910, 0.437032, 0.418148, 0.399490, 0.380784, 0.362024, 0.343193, 0.324490, 0.305825, 0.286971, 0.268118, 0.249384, 0.230666, 0.211854, 0.193103, 0.174354, 0.155913, 0.131256, 0.113258, 0.096227, 0.077911, 0.059520, 0.041240, 0.022849, 0.004604, -0.013771, -0.032039, -0.050439, -0.068710, -0.087143, -0.104045, -0.122299, -0.146704, -0.165218, -0.184033, -0.202654, -0.221536, -0.240324, -0.258915, -0.277828, -0.296733, -0.315451, -0.334002, -0.352892, -0.371696, -0.390449, -0.408962, -0.427876, -0.446766, -0.465565, -0.484362, -0.505619, -0.523295, -0.542173, -0.561072, -0.579892, -0.598741, -0.617481, -0.636304, -0.655142, -0.673841, -0.692634, -0.711465, -0.730266, -0.749117, -0.767894, -0.786554, -0.805589, -0.824465, -0.843304 };

  TH1F* mHistoExpTrackedStrip = nullptr;
  TH1F* mHistoExpMatchedStrip = nullptr;
  TH1F* mHistoExpMatchedStripFromCh = nullptr;
  TH2F* mHistoExpMatchedStripFromChDx = nullptr;
  TH2F* mHistoExpMatchedStripFromChDz = nullptr;
  TH2F* mHistoExpMatchedStripFromChR = nullptr;
  TH2F* mHistoExpMatchedNhit = nullptr;

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
