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
/// \author My Name
///

#ifndef QC_MODULE_TOF_TOFTOFMATCHEDTRACKS_H
#define QC_MODULE_TOF_TOFTOFMATCHEDTRACKS_H

#include "QualityControl/TaskInterface.h"

#include "DataFormatsGlobalTracking/RecoContainer.h"
#include "ReconstructionDataFormats/MatchInfoTOFReco.h"

class TH1D;

using namespace o2::quality_control::core;
using GID = o2::dataformats::GlobalTrackID;
using trkType = o2::dataformats::MatchInfoTOFReco::TrackType;

namespace o2::quality_control_modules::tof
{

/// \brief Example Quality Control DPL Task
/// \author My Name
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
  TH1D* mInTracksPt[trkType::SIZE];
  TH1D* mInTracksEta[trkType::SIZE];
  TH1D* mMatchedTracksPt[trkType::SIZE];
  TH1D* mMatchedTracksEta[trkType::SIZE];
  TH1D* mEffPt[trkType::SIZE];
  TH1D* mEffEta[trkType::SIZE];
};

} // namespace o2::quality_control_modules::tof

#endif // QC_MODULE_TOF_TOFTOFMATCHEDTRACKS_H
