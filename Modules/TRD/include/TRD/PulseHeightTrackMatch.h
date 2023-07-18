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
/// \file   PulseHeightTrackMatch.h
/// \author Vikash Sumberia
///

#ifndef QC_MODULE_TRD_TRDPULSEHEIGHTTRACKMATCH_H
#define QC_MODULE_TRD_TRDPULSEHEIGHTTRACKMATCH_H

#include "QualityControl/TaskInterface.h"
#include <array>
#include "DataFormatsTRD/NoiseCalibration.h"
#include "DataFormatsTRD/Digit.h"
#include "DataFormatsTRD/Constants.h"
#include "TRDQC/StatusHelper.h"

class TH1F;
class TH2F;
class TH1D;
class TH2D;
class TLine;
class TProfile;
class TProfile2D;

using namespace o2::quality_control::core;

namespace o2::quality_control_modules::trd
{

/// \brief Example Quality Control DPL Task
/// \author My Name
class PulseHeightTrackMatch final : public TaskInterface
{
 public:
  /// \brief Constructor
  PulseHeightTrackMatch() = default;
  /// Destructor
  ~PulseHeightTrackMatch() override;

  // Definition of the methods for the template method pattern
  void initialize(o2::framework::InitContext& ctx) override;
  void startOfActivity(const Activity& activity) override;
  void startOfCycle() override;
  void monitorData(o2::framework::ProcessingContext& ctx) override;
  void endOfCycle() override;
  void endOfActivity(const Activity& activity) override;
  void reset() override;
  void buildHistograms();
  void retrieveCCDBSettings();
  void drawLinesOnPulseHeight(TProfile* h);

 private:
  // json configurable parameters
  std::pair<float, float> mDriftRegion;
  std::pair<float, float> mPulseHeightPeakRegion;
  std::bitset<4> mTrackType = 0xf; // bitset to select one or a combination of track types 0: ITSTPCTRD, 1: TPCTRD, 2: TRACKLET, 3: OTHERS. Default is 0xf: all tracks
  long int mTimestamp;
  std::shared_ptr<TProfile> mPulseHeightpro = nullptr;
  std::shared_ptr<TProfile2D> mPulseHeightperchamber = nullptr;
};

} // namespace o2::quality_control_modules::trd

#endif // QC_MODULE_TRD_TRDPULSEHEIGHTTRACKMATCH_H
