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
/// \file   HmpidTaskMatchInfo.h
/// \author Nicola Nicassio, Giacomo Volpe
/// \brief  Task to produce HMPID matchingInfo QC plots
/// \since  20/10/2023
///

#ifndef QC_MODULE_HMPID_HMPIDTASKMATCHINFO_H
#define QC_MODULE_HMPID_HMPIDTASKMATCHINFO_H

#include "QualityControl/TaskInterface.h"

#include "DataFormatsGlobalTracking/RecoContainer.h"
#include "ReconstructionDataFormats/MatchInfoHMP.h"
#include "SimulationDataFormat/MCCompLabel.h"
#include "DataFormatsTRD/TrackTRD.h"

class TH1F;
class TH2F;
// class TEfficiency;

namespace o2::quality_control_modules::hmpid
{

using namespace o2::quality_control::core;
using GID = o2::dataformats::GlobalTrackID;

class HmpidTaskMatchInfo final : public TaskInterface
{
 public:
  /// Constructor
  HmpidTaskMatchInfo();
  /// Destructor
  ~HmpidTaskMatchInfo() override;

  // Definition of the methods for the template method pattern
  void initialize(o2::framework::InitContext& ctx) override;
  void startOfActivity(const Activity& activity) override;
  void startOfCycle() override;
  void monitorData(o2::framework::ProcessingContext& ctx) override;
  void endOfCycle() override;
  void endOfActivity(const Activity& activity) override;
  void reset() override;

 private:
  std::shared_ptr<o2::globaltracking::DataRequest> mDataRequest;
  o2::globaltracking::RecoContainer mRecoCont;
  GID::mask_t mSrc = GID::getSourcesMask("HMP");
  GID::mask_t mAllowedSources = GID::getSourcesMask("HMP");
  // HMPID Maching Info
  gsl::span<const o2::dataformats::MatchInfoHMP> mHMPMatches;

  bool mUseMC = false;
  bool mVerbose = false;
  TH1F* mMatchInfoResidualsXTrackMIP[7];
  TH1F* mMatchInfoResidualsYTrackMIP[7];
  TH1F* mMatchInfoChargeClusterMIP[7];
  TH1F* mMatchInfoChargeClusterPhotons[7];
  TH2F* mMatchInfoThetaCherenkovVsMom[7];

  // int mTF = -1; // to count the number of processed TFs
};

} // namespace o2::quality_control_modules::hmpid

#endif // QC_MODULE_HMPID_HMPIDTASKMATCHINFO_H
