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
/// \file   PedestalTask.h
/// \author Sergey Evdokimov
///

#ifndef QC_MODULE_CPV_CPVPEDESTALTASK_H
#define QC_MODULE_CPV_CPVPEDESTALTASK_H

#include "QualityControl/TaskInterface.h"
#include <memory>
#include <array>
#include <gsl/span>
#include <CPVBase/Geometry.h>

class TH1F;
class TH2F;

using namespace o2::quality_control::core;

namespace o2::quality_control_modules::cpv
{

/// \brief CPV Pedestal Task which processes uncalibrated digits from pedestal runs and produces pedestal monitor objects
/// \author Sergey Evdokimov
class PedestalTask final : public TaskInterface
{
 public:
  /// \brief Constructor
  PedestalTask();
  /// Destructor
  ~PedestalTask() override;

  // Definition of the methods for the template method pattern
  void initialize(o2::framework::InitContext& ctx) override;
  void startOfActivity(const Activity& activity) override;
  void startOfCycle() override;
  void monitorData(o2::framework::ProcessingContext& ctx) override;
  void endOfCycle() override;
  void endOfActivity(const Activity& activity) override;
  void reset() override;

 private:
  void initHistograms();
  void fillDigitsHistograms();
  void resetHistograms();

  static constexpr short kNHist1D = 27;
  enum Histos1D { H1DRawErrors,
                  H1DInputPayloadSize,
                  H1DNInputs,
                  H1DNValidInputs,
                  H1DNDigitsPerInput,
                  H1DDigitIds,
                  H1DPedestalValueM2,
                  H1DPedestalValueM3,
                  H1DPedestalValueM4,
                  H1DPedestalSigmaM2,
                  H1DPedestalSigmaM3,
                  H1DPedestalSigmaM4,
                  H1DPedestalEfficiencyM2,
                  H1DPedestalEfficiencyM3,
                  H1DPedestalEfficiencyM4,
                  H1DPedestalValueInDigitsM2,
                  H1DPedestalValueInDigitsM3,
                  H1DPedestalValueInDigitsM4,
                  H1DPedestalSigmaInDigitsM2,
                  H1DPedestalSigmaInDigitsM3,
                  H1DPedestalSigmaInDigitsM4,
                  H1DPedestalEfficiencyInDigitsM2,
                  H1DPedestalEfficiencyInDigitsM3,
                  H1DPedestalEfficiencyInDigitsM4
  };

  static constexpr short kNHist2D = 34;
  enum Histos2D { H2DErrorType,
                  H2DDigitMapM2,
                  H2DDigitMapM3,
                  H2DDigitMapM4,
                  H2DPedestalValueMapM2,
                  H2DPedestalValueMapM3,
                  H2DPedestalValueMapM4,
                  H2DPedestalSigmaMapM2,
                  H2DPedestalSigmaMapM3,
                  H2DPedestalSigmaMapM4,
                  H2DPedestalEfficiencyMapM2,
                  H2DPedestalEfficiencyMapM3,
                  H2DPedestalEfficiencyMapM4,
                  H2DFEEThresholdsMapM2,
                  H2DFEEThresholdsMapM3,
                  H2DFEEThresholdsMapM4,
                  H2DHighThresholdMapM2,
                  H2DHighThresholdMapM3,
                  H2DHighThresholdMapM4,
                  H2DDeadChanelsMapM2,
                  H2DDeadChanelsMapM3,
                  H2DDeadChanelsMapM4,
                  H2DPedestalNPeaksMapInDigitsM2,
                  H2DPedestalNPeaksMapInDigitsM3,
                  H2DPedestalNPeaksMapInDigitsM4,
                  H2DPedestalValueMapInDigitsM2,
                  H2DPedestalValueMapInDigitsM3,
                  H2DPedestalValueMapInDigitsM4,
                  H2DPedestalSigmaMapInDigitsM2,
                  H2DPedestalSigmaMapInDigitsM3,
                  H2DPedestalSigmaMapInDigitsM4,
                  H2DPedestalEfficiencyMapInDigitsM2,
                  H2DPedestalEfficiencyMapInDigitsM3,
                  H2DPedestalEfficiencyMapInDigitsM4
  };

  int mNEventsTotal = 0;
  int mNEventsFromLastFillHistogramsCall;
  int mMinNEventsToUpdatePedestals = 1000; ///< min number of events needed to update pedestals
  int mRunNumber = 0;                      ///< Run number of current activity
  bool mMonitorPedestalCalibrator = true;  ///< monitor results of pedestal calibrator
  int mNtimesCCDBPayloadFetched = 0;       ///< how many times non-empty CCDB payload fetched
  bool mMonitorDigits = false;             ///< monitor digits

  std::array<TH1F*, kNHist1D> mHist1D = { nullptr }; ///< Array of 1D histograms
  std::array<TH2F*, kNHist2D> mHist2D = { nullptr }; ///< Array of 2D histograms

  std::array<TH1F*, o2::cpv::Geometry::kNCHANNELS> mHistAmplitudes = { nullptr };  ///< Array of amplitude spectra
  std::array<bool, o2::cpv::Geometry::kNCHANNELS> mIsUpdatedAmplitude = { false }; ///< Array of isUpdatedAmplitude bools
};

} // namespace o2::quality_control_modules::cpv

#endif // QC_MODULE_CPV_CPVPEDESTALTASK_H
