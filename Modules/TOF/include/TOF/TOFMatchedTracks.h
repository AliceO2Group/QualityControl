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

class TH1F;
class TEfficiency;

namespace o2::quality_control_modules::tof
{

using namespace o2::quality_control::core;
using GID = o2::dataformats::GlobalTrackID;
using trkType = o2::dataformats::MatchInfoTOFReco::TrackType;

/// \brief  Task for the control of the TOF matching efficiency
/// \author Chiara Zampolli
class TOFMatchedTracks final : public TaskInterface
{
 public:
  /// \brief Constructor
  TOFMatchedTracks() = default;
  /// Destructor
  ~TOFMatchedTracks() override;

  // Definition of the methods for the template method pattern
  void initialize(o2::framework::InitContext& ctx) override;
  void startOfActivity(Activity& activity) override;
  void startOfCycle() override;
  void monitorData(o2::framework::ProcessingContext& ctx) override;
  void endOfCycle() override;
  void endOfActivity(Activity& activity) override;
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
  GID::mask_t mAllowedSources = GID::getSourcesMask("TPC,ITS-TPC,TPC-TOF,ITS-TPC-TOF");
  // TPC-TOF
  gsl::span<const o2::tpc::TrackTPC> mTPCTracks;
  gsl::span<const o2::dataformats::MatchInfoTOF> mTPCTOFMatches;
  // ITS-TPC-TOF
  gsl::span<const o2::dataformats::TrackTPCITS> mITSTPCTracks;
  gsl::span<const o2::dataformats::MatchInfoTOF> mITSTPCTOFMatches;

  bool mUseMC = false;
  bool mVerbose = false;
  TH1F* mInTracksPt[trkType::SIZE] = {};
  TH1F* mInTracksEta[trkType::SIZE] = {};
  TH1F* mMatchedTracksPt[trkType::SIZE] = {};
  TH1F* mMatchedTracksEta[trkType::SIZE] = {};
  TH1F* mFakeMatchedTracksPt[trkType::SIZE] = {};
  TH1F* mFakeMatchedTracksEta[trkType::SIZE] = {};
  TEfficiency* mEffPt[trkType::SIZE] = {};
  TEfficiency* mEffEta[trkType::SIZE] = {};
  TEfficiency* mFakeFractionTracksPt[trkType::SIZE] = {};  // fraction of fakes among the matched tracks vs pT
  TEfficiency* mFakeFractionTracksEta[trkType::SIZE] = {}; // fraction of fakes among the matched tracks vs Eta

  // for track selection
  float mPtCut = 0.1f;
  float mEtaCut = 1.4f;
  int32_t mNTPCClustersCut = 40;
  float mDCACut = 100.f;
  float mDCACutY = 10.f;
  std::string mGRPFileName = "o2sim_grp.root";
  std::string mGeomFileName = "o2sim_geometry.root";
  float mBz = 0; ///< nominal Bz
};

} // namespace o2::quality_control_modules::tof

#endif // QC_MODULE_TOF_TOFTOFMATCHEDTRACKS_H
