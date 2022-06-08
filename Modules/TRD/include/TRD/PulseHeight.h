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
/// \file   PulseHeight.h
/// \author My Name
///

#ifndef QC_MODULE_TRD_TRDPULSEHEIGHT_H
#define QC_MODULE_TRD_TRDPULSEHEIGHT_H

#include "QualityControl/TaskInterface.h"
#include <array>
#include "DataFormatsTRD/NoiseCalibration.h"

class TH1F;
class TH1D;
class TH2F;
class TH2D;
class TProfile;
class TProfile2D;

using namespace o2::quality_control::core;

namespace o2::quality_control_modules::trd
{

/// \brief Example Quality Control DPL Task
/// \author My Name
class PulseHeight final : public TaskInterface
{
 public:
  /// \brief Constructor
  PulseHeight() = default;
  /// Destructor
  ~PulseHeight() override;

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

 private:
  std::shared_ptr<TH1F> mPulseHeight = nullptr;
  std::shared_ptr<TH1F> mPulseHeightScaled = nullptr;
  std::shared_ptr<TH2F> mTotalPulseHeight2D = nullptr;
  std::array<std::shared_ptr<TH1F>, 18> mPulseHeight2DperSM; // ph2DSM;
  std::shared_ptr<TH1F> mPulseHeight2 = nullptr;
  std::shared_ptr<TH1F> mPulseHeight2n = nullptr;
  std::shared_ptr<TH1F> mPulseHeightScaled2 = nullptr;
  std::shared_ptr<TH2F> mTotalPulseHeight2D2 = nullptr;
  std::array<std::shared_ptr<TH1F>, 18> mPulseHeight2DperSM2; // ph2DSM;
  std::pair<float, float> mDriftRegion;
  std::pair<float, float> mPulseHeightPeakRegion;
  std::shared_ptr<TH1F> mPulseHeightDuration;
  std::shared_ptr<TH1F> mPulseHeightDuration1;
  std::shared_ptr<TH1F> mPulseHeightDurationDiff;
  o2::trd::NoiseStatusMCM* mNoiseMap = nullptr;
  std::shared_ptr<TProfile> mPulseHeightpro = nullptr;
  std::shared_ptr<TProfile2D> mPulseHeightperchamber = nullptr;
};

} // namespace o2::quality_control_modules::trd

#endif // QC_MODULE_TRD_TRDPULSEHEIGHT_H
