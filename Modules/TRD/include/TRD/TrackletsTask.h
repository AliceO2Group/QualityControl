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
/// \file   TrackletsTask.h
///

#ifndef QC_MODULE_TRD_TRDTRACKLETSTASK_H
#define QC_MODULE_TRD_TRDTRACKLETSTASK_H

#include "QualityControl/TaskInterface.h"
#include "QualityControl/DatabaseInterface.h"
#include "DataFormatsTRD/NoiseCalibration.h"
#include "TRDQC/StatusHelper.h"

class TH1F;
class TH2F;
class TCanvas;

using namespace o2::quality_control::core;

namespace o2::quality_control_modules::trd
{

class TrackletsTask final : public TaskInterface
{
 public:
  TrackletsTask() = default;
  ~TrackletsTask() override = default;

  // Definition of the methods for the template method pattern
  void initialize(o2::framework::InitContext& ctx) override;
  void startOfActivity(const Activity& activity) override;
  void startOfCycle() override;
  void monitorData(o2::framework::ProcessingContext& ctx) override;
  void endOfCycle() override;
  void endOfActivity(const Activity& activity) override;
  void reset() override;
  void buildHistograms();

 private:
  // settings
  bool mRemoveNoise{ false };
  // histograms
  std::array<std::shared_ptr<TH1F>, 3> mTrackletQ;
  std::shared_ptr<TH1F> mTrackletSlope = nullptr;
  std::shared_ptr<TH1F> mTrackletHCID = nullptr;
  std::shared_ptr<TH1F> mTrackletPosition = nullptr;
  std::shared_ptr<TH1F> mTrackletsPerEvent = nullptr;
  std::shared_ptr<TH1F> mTrackletsPerEventPP = nullptr;
  std::shared_ptr<TH1F> mTrackletsPerEventPbPb = nullptr;
  std::shared_ptr<TH2F> mTrackletsPerHC2D = nullptr;
  std::shared_ptr<TH1F> mTrackletsPerTimeFrame = nullptr;
  std::shared_ptr<TH1F> mTriggersPerTimeFrame = nullptr;
  std::array<std::shared_ptr<TH2F>, o2::trd::constants::NLAYER> mLayers;

  // Plotting variables
  //TRDHelpers mTRDHelpers; // Auxiliary functions for TRD
  int mUnitsPerSection;   // Units for each section in layers plots

  // data to pull from CCDB
  const o2::trd::NoiseStatusMCM* mNoiseMap = nullptr;
  const std::array<int, o2::trd::constants::MAXCHAMBER>* mChamberStatus = nullptr;
};

} // namespace o2::quality_control_modules::trd

#endif // QC_MODULE_TRD_TRDTRACKLETSTASK_H
