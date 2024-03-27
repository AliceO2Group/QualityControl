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
/// \file   DigitsTask.h
///

#ifndef QC_MODULE_TRD_DIGITSTASK_H
#define QC_MODULE_TRD_DIGITSTASK_H

#include "QualityControl/TaskInterface.h"
#include <array>
#include "DataFormatsTRD/NoiseCalibration.h"
#include "TRDQC/StatusHelper.h"

class TH1F;
class TH2F;
class TProfile;
class TProfile2D;
using namespace o2::quality_control::core;

namespace o2::quality_control_modules::trd
{

/// \brief Example Quality Control DPL Task
/// \author My Name
class DigitsTask final : public TaskInterface
{
 public:
  DigitsTask() = default;
  ~DigitsTask() override = default;

  // Definition of the methods for the template method pattern
  void initialize(o2::framework::InitContext& ctx) override;
  void startOfActivity(const Activity& activity) override;
  void startOfCycle() override;
  void monitorData(o2::framework::ProcessingContext& ctx) override;
  void endOfCycle() override;
  void endOfActivity(const Activity& activity) override;
  void finaliseCCDB(o2::framework::ConcreteDataMatcher& matcher, void* obj) override;
  void reset() override;
  void buildHistograms();
  void drawLinesOnPulseHeight(TH1F* h);
  void buildChamberIgnoreBP();

 private:
  // user settings
  unsigned int mPulseHeightThreshold;
  bool mDoClusterize{ false };
  std::pair<float, float> mPulseHeightPeakRegion;
  std::string mChambersToIgnore;
  std::bitset<o2::trd::constants::MAXCHAMBER> mChambersToIgnoreBP;
  int mClsCutoff = 1000;
  int mAdcBaseline = 10;

  // histograms
  std::shared_ptr<TH1F> mDigitsPerEvent;
  std::shared_ptr<TH1F> mDigitsSizevsTrackletSize;
  std::shared_ptr<TH1F> mDigitHCID;
  std::shared_ptr<TH1F> mADCvalue;

  // histograms for clusterizer are not published by default
  std::shared_ptr<TH1F> mNCls;
  std::shared_ptr<TH2F> mClsTb;
  std::shared_ptr<TH1F> mClsAmp;
  std::shared_ptr<TH1F> mClsChargeTb;
  std::shared_ptr<TH1F> mClsNTb;

  std::array<std::shared_ptr<TH2F>, o2::trd::constants::NSECTOR> mHCMCM;

  std::shared_ptr<TH1F> mPulseHeight = nullptr;
  std::shared_ptr<TH2F> mTotalPulseHeight2D = nullptr;
  std::array<std::shared_ptr<TH1F>, o2::trd::constants::NSECTOR> mPulseHeight2DperSM;
  std::shared_ptr<TProfile> mPulseHeightpro = nullptr;
  std::shared_ptr<TProfile2D> mPulseHeightperchamber = nullptr;
  std::array<std::shared_ptr<TH2F>, o2::trd::constants::NLAYER> mLayers;

  // CCDB objects
  const o2::trd::NoiseStatusMCM* mNoiseMap = nullptr;
};

} // namespace o2::quality_control_modules::trd

#endif // QC_MODULE_TRD_DIGITSTASK_H
