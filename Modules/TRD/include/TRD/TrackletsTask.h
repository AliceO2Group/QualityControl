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
/// \author My Name
///

#ifndef QC_MODULE_TRD_TRDTRACKLETSTASK_H
#define QC_MODULE_TRD_TRDTRACKLETSTASK_H

#include "QualityControl/TaskInterface.h"
#include "QualityControl/DatabaseInterface.h"
#include "DataFormatsTRD/NoiseCalibration.h"

class TH1F;
class TH2F;

using namespace o2::quality_control::core;

namespace o2::quality_control_modules::trd
{

/// \brief Example Quality Control DPL Task
/// \author My Name
class TrackletsTask final : public TaskInterface
{
 public:
  /// \brief Constructor
  TrackletsTask() = default;
  /// Destructor
  ~TrackletsTask() override;

  // Definition of the methods for the template method pattern
  void initialize(o2::framework::InitContext& ctx) override;
  void startOfActivity(Activity& activity) override;
  void startOfCycle() override;
  void monitorData(o2::framework::ProcessingContext& ctx) override;
  void endOfCycle() override;
  void endOfActivity(Activity& activity) override;
  void reset() override;
  void buildHistograms();
  void retrieveCCDBSettings();
  void drawLinesMCM(TH2F* histo);

 private:
  long int mTimestamp;
  std::array<std::shared_ptr<TH2F>, 18> moHCMCM;
  std::shared_ptr<TH1F> mTrackletSlope = nullptr;
  std::shared_ptr<TH1F> mTrackletSlopeRaw = nullptr;
  std::shared_ptr<TH1F> mTrackletHCID = nullptr;
  std::shared_ptr<TH1F> mTrackletPosition = nullptr;
  std::shared_ptr<TH1F> mTrackletPositionRaw = nullptr;
  std::shared_ptr<TH1F> mTrackletsPerEvent = nullptr;
  std::array<std::shared_ptr<TH2F>, 18> moHCMCMn;
  std::shared_ptr<TH1F> mTrackletSlopen = nullptr;
  std::shared_ptr<TH1F> mTrackletSlopeRawn = nullptr;
  std::shared_ptr<TH1F> mTrackletHCIDn = nullptr;
  std::shared_ptr<TH1F> mTrackletPositionn = nullptr;
  std::shared_ptr<TH1F> mTrackletPositionRawn = nullptr;
  std::shared_ptr<TH1F> mTrackletsPerEventn = nullptr;
  o2::trd::NoiseStatusMCM* mNoiseMap = nullptr;
};

} // namespace o2::quality_control_modules::trd

#endif // QC_MODULE_TRD_TRDTRACKLETSTASK_H
