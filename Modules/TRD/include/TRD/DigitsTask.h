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
/// \file   TRDDigitQcTask.h
/// \author My Name
///

#ifndef QC_MODULE_TRD_DIGITSTASK_H
#define QC_MODULE_TRD_DIGITSTASK_H

#include "QualityControl/TaskInterface.h"
#include <array>
#include "DataFormatsTRD/NoiseCalibration.h"
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
class DigitsTask final : public TaskInterface
{

 public:
  /// \brief Constructor
  DigitsTask() = default;
  /// Destructor
  ~DigitsTask() override;

  // Definition of the methods for the template method pattern
  void initialize(o2::framework::InitContext& ctx) override;
  void startOfActivity(const Activity& activity) override;
  void startOfCycle() override;
  void monitorData(o2::framework::ProcessingContext& ctx) override;
  void endOfCycle() override;
  void endOfActivity(const Activity& activity) override;
  void reset() override;
  void buildHistograms();
  void drawLinesMCM(TH2F* histo);
  void drawTrdLayersGrid(TH2F* hist);
  void retrieveCCDBSettings();
  void drawLinesOnPulseHeight(TH1F* h);
  void fillLinesOnHistsPerLayer(int iLayer);
  void drawHashOnLayers(int layer, int hcid, int col, int rowstart, int rowend);
  void buildChamberIgnoreBP();
  bool isChamberToBeIgnored(unsigned int sm, unsigned int stack, unsigned int layer);

 private:
  // limits
  bool mSkipSharedDigits;
  bool mLayerLabelsIgnore = false;
  unsigned int mPulseHeightThreshold;
  std::pair<float, float> mDriftRegion;
  std::pair<float, float> mPulseHeightPeakRegion;
  long int mTimestamp;

  std::shared_ptr<TH1F> mDigitsPerEvent;
  std::shared_ptr<TH1F> mEventswDigitsPerTimeFrame;
  std::shared_ptr<TH2F> mDigitsSizevsTrackletSize;
  std::shared_ptr<TH1F> mDigitHCID = nullptr;
  std::shared_ptr<TH2F> mClusterAmplitudeChamber;
  std::array<std::shared_ptr<TH2F>, 6> mNClsLayer;
  std::shared_ptr<TH1D> mADCvalue;
  std::array<std::shared_ptr<TH1F>, 18> mADC;
  std::array<std::shared_ptr<TH2F>, 18> mADCTB;
  std::array<std::shared_ptr<TH2F>, 18> mADCTBfull;
  std::shared_ptr<TH1F> mNCls;
  std::array<std::shared_ptr<TH2F>, 18> mHCMCM;
  std::array<std::shared_ptr<TH1F>, 18> mClsSM;
  std::shared_ptr<TH2F> mClsTb;
  std::shared_ptr<TH2F> mClsChargeFirst;
  std::shared_ptr<TH1F> mClsChargeTb;
  std::shared_ptr<TH1F> mClsChargeTbCycle;
  std::shared_ptr<TH1F> mClsNTb;
  std::shared_ptr<TH1F> mClsAmp;
  std::shared_ptr<TH1F> mNClsAmp;
  std::shared_ptr<TH1F> mClsAmpDrift;
  std::shared_ptr<TH1F> mClsAmpTb;
  std::shared_ptr<TH1F> mClsAmpCh;
  std::array<std::shared_ptr<TH2F>, 18> mClsDetAmp;
  std::shared_ptr<TH2F> mClsSector;
  std::shared_ptr<TH2F> mClsStack;
  std::shared_ptr<TH1F> mClsChargeTbTigg;
  std::shared_ptr<TH2F> mClsChargeTbTrigHM;
  std::shared_ptr<TH2F> mClsChargeTbTrigMinBias;
  std::shared_ptr<TH2F> mClsChargeTbTrigTRDL1;
  std::array<std::shared_ptr<TH2F>, 18> mClsTbSM;
  std::shared_ptr<TH1F> mPulseHeight = nullptr;
  std::shared_ptr<TH1F> mPulseHeightScaled = nullptr;
  std::shared_ptr<TH2F> mTotalPulseHeight2D = nullptr;
  std::array<std::shared_ptr<TH1F>, 18> mPulseHeight2DperSM;
  std::shared_ptr<TH1F> mPulseHeightn = nullptr;
  std::shared_ptr<TProfile> mPulseHeightpro = nullptr;
  std::shared_ptr<TProfile2D> mPulseHeightperchamber = nullptr;
  //  std::array<std::shared_ptr<TH1F>, 540> mPulseHeightPerChamber_1D; // ph2DSM;
  std::vector<TH2F*> mLayers;
  // information pulled from ccdb
  o2::trd::NoiseStatusMCM* mNoiseMap = nullptr;
  o2::trd::HalfChamberStatusQC* mChamberStatus = nullptr;
  std::string mChambersToIgnore;
  std::bitset<o2::trd::constants::MAXCHAMBER> mChambersToIgnoreBP;
};

} // namespace o2::quality_control_modules::trd

#endif // QC_MODULE_TRD_DIGITSTASK_H
