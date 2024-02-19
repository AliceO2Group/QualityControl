// Copyright CERN and copyright holders of ALICE O2. This software is
// // distributed under the terms of the GNU General Public License v3 (GPL
// // Version 3), copied verbatim in the file "COPYING".
// //
// // See http://alice-o2.web.cern.ch/license for full licensing information.
// //
// // In applying this license CERN does not waive the privileges and immunities
// // granted to it by virtue of its status as an Intergovernmental Organization
// // or submit itself to any jurisdiction.
//

///
/// \file   HmpidTaskMatches.h
/// \author Nicola Nicassio, Giacomo Volpe
///

#ifndef QC_MODULE_HMPID_HMPIDTASKMATCHES_H
#define QC_MODULE_HMPID_HMPIDTASKMATCHES_H

#include <Framework/InputRecord.h>
#include "QualityControl/TaskInterface.h"

#include "DataFormatsGlobalTracking/RecoContainer.h"
#include "ReconstructionDataFormats/MatchInfoHMP.h"
#include "SimulationDataFormat/MCCompLabel.h"

class TH1F;
class TH2F;

using namespace o2::quality_control::core;
using GID = o2::dataformats::GlobalTrackID;

namespace o2::quality_control_modules::hmpid
{

class HmpidTaskMatches : public TaskInterface
{

 public:
  HmpidTaskMatches();
  ~HmpidTaskMatches() override;

  void initialize(o2::framework::InitContext& ctx) override;
  void startOfActivity(const Activity& activity) override;
  void startOfCycle() override;
  void monitorData(o2::framework::ProcessingContext& ctx) override;
  void endOfCycle() override;
  void endOfActivity(const Activity& activity) override;
  void reset() override;

 private:
  void BookHistograms();

  std::shared_ptr<o2::globaltracking::DataRequest> mDataRequest;
  o2::globaltracking::RecoContainer mRecoCont;
  GID::mask_t mSrc = GID::getSourcesMask("HMP");
  // HMPID Maching Info
  gsl::span<const o2::dataformats::MatchInfoHMP> mHMPMatches;

  TH1F* mMatchInfoResidualsXTrackMIP[7];
  TH1F* mMatchInfoResidualsYTrackMIP[7];
  TH1F* mMatchInfoChargeClusterMIP[7];
  TH1F* mMatchInfoChargeClusterPhotons[7];
  TH1F* mMatchInfoXMip[7];
  TH1F* mMatchInfoYMip[7];
  TH1F* mMatchInfoXTrack[7];
  TH1F* mMatchInfoYTrack[7];
  TH2F* mMatchInfoClusterMIPMap[7];
  TH2F* mMatchInfoThetaCherenkovVsMom[7];

  std::vector<TObject*> mPublishedObjects;
};
} //  namespace o2::quality_control_modules::hmpid

#endif
