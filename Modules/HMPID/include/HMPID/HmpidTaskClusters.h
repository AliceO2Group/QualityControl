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
/// \file   HmpidTaskClusters.h
/// \author Annalisa Mastroserio
///

#ifndef QC_MODULE_HMPID_HMPIDTASKCLUSTERS_H
#define QC_MODULE_HMPID_HMPIDTASKCLUSTERS_H
#include <Framework/InputRecord.h>
#include "QualityControl/TaskInterface.h"
#include <TH1.h>
#include <TProfile.h>
#include <TH2.h>
#include <THnSparse.h>

class TH1F;

using namespace o2::quality_control::core;

namespace o2::quality_control_modules::hmpid
{

class HmpidTaskClusters : public TaskInterface
{

 public:
  HmpidTaskClusters();
  ~HmpidTaskClusters() override;

  void initialize(o2::framework::InitContext& ctx) override;
  void startOfActivity(const Activity& activity) override;
  void startOfCycle() override;
  void monitorData(o2::framework::ProcessingContext& ctx) override;
  void endOfCycle() override;
  void endOfActivity(const Activity& activity) override;
  void reset() override;

 private:
  void BookHistograms();
  void getJsonParameters();

  // monitoring histos
  TProfile* ThClusMult;
  TH1F* hClusMultEv;
  TH1F* hHMPIDchargeClus[7];
  TH2F* hHMPIDpositionClus[7];

  std::vector<TObject*> mPublishedObjects;
};
} //  namespace o2::quality_control_modules::hmpid

#endif
