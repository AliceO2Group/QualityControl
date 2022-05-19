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

class TH1F;
class TH2F;
class TH1D;
class TH2D;
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
  void startOfActivity(Activity& activity) override;
  void startOfCycle() override;
  void monitorData(o2::framework::ProcessingContext& ctx) override;
  void endOfCycle() override;
  void endOfActivity(Activity& activity) override;
  void reset() override;
  void buildHistograms();
  void buildHistogramsPH();
  void drawLinesMCM(TH2F* histo);
  void retrieveCCDBSettings();

 private:
  // limits
  std::pair<float, float> mDriftRegion;
  std::pair<float, float> mPulseHeightPeakRegion;

  std::shared_ptr<TH1F> mDigitsPerEvent = nullptr;
  std::shared_ptr<TH1F> mTotalChargevsTimeBin = nullptr; //
  std::shared_ptr<TH1F> mDigitHCID = nullptr;
  std::shared_ptr<TH1F> mParsingErrors = nullptr;

  std::shared_ptr<TH2F> mClusterAmplitudeChamber;
  std::array<std::shared_ptr<TH2F>, 6> mNClsLayer;        ///[layer]->Fill(sm - 0.5 + col / 144., startRow[istack]+row);
  std::shared_ptr<TH1D> mADCvalue;                        //->Fill(value);
  std::array<std::shared_ptr<TH1F>, 18> mADC;             //[sm]->Fill(value);
  std::array<std::shared_ptr<TH2F>, 18> mADCTB;           //[sm]->Fill(time, value);
  std::array<std::shared_ptr<TH2F>, 18> mADCTBfull;       //[sm]->Fill(time, value);
  std::shared_ptr<TH1F> mNCls;                            //->Fill(sm);
  std::array<std::shared_ptr<TH2F>, 18> mHCMCM;           //[sm]->Fill(sum);
  std::array<std::shared_ptr<TH1F>, 18> mClsSM;           //[sm]->Fill(sum);
  std::array<std::shared_ptr<TH2F>, 18> mcLStBsm;         //[SM]->fILL(TIme, sum);
  std::shared_ptr<TH2F> mClsTb;                           //->Fill(time, sum);
  std::shared_ptr<TH2F> mClsChargeFirst;                  //->Fill(sum, (1.*sum/sumU) -1.);
  std::shared_ptr<TH1F> mClsChargeTb;                     //->Fill(time, sum);
  std::shared_ptr<TH1F> mClsChargeTbCycle;                //->Fill(time, sum);
  std::shared_ptr<TH1F> mClsNTb;                          //->Fill(time);
  std::shared_ptr<TH1F> mClsAmp;                          //->Fill(sum);
  std::shared_ptr<TH1F> mClsAmpDrift;                     //->Fill(sum);
  std::shared_ptr<TH1F> mClsAmpTb;                        //, "ClsAmpTb", "ClsAmpTb", 30, -0.5, 29.5);
  std::shared_ptr<TH1F> mClsAmpCh;                        //
  std::array<std::shared_ptr<TH2F>, 18> mClsDetAmp;       //[sm]->Fill(detLoc, sum);
  std::shared_ptr<TH2F> mClsSector;                       //, "ClsSector", "ClsSector", nSMs, -0.5, 17.5, 500, -0.5, 999.5);
  std::shared_ptr<TH2F> mClsStack;                        //, "ClsStack", "ClsStack", 5, -0.5, 4.5, 500, -0.5, 999.5);
  std::array<std::shared_ptr<TH2F>, 18> mClsDetTime;      //[sm]->Fill(detLoc, time, sum);
  std::array<std::shared_ptr<TH1F>, 10> mClsChargeTbTigg; //[trgg]->Fill(time, sum);
  std::shared_ptr<TH2F> mClsChargeTbTrigHM;               //->Fill(time, sum);
  std::shared_ptr<TH2F> mClsChargeTbTrigMinBias;          //->Fill(time, sum);
  std::shared_ptr<TH2F> mClsChargeTbTrigTRDL1;            //->Fill(time, sum);
  std::array<std::shared_ptr<TH2F>, 18> mClsTbSM;

  std::shared_ptr<TH1F> mPulseHeight = nullptr;
  std::shared_ptr<TH1F> mPulseHeightScaled = nullptr;
  std::shared_ptr<TH2F> mTotalPulseHeight2D = nullptr;
  std::array<std::shared_ptr<TH1F>, 18> mPulseHeight2DperSM; // ph2DSM;
  std::shared_ptr<TH1F> mPulseHeight2 = nullptr;
  std::shared_ptr<TH1F> mPulseHeight2n = nullptr;
  std::shared_ptr<TH1F> mPulseHeightScaled2 = nullptr;
  std::shared_ptr<TH2F> mTotalPulseHeight2D2 = nullptr;
  std::array<std::shared_ptr<TH1F>, 18> mPulseHeight2DperSM2; // ph2DSM;
  std::shared_ptr<TH1F> mPulseHeightDuration;
  std::shared_ptr<TH1F> mPulseHeightDuration1;
  std::shared_ptr<TH1F> mPulseHeightDurationDiff;
  o2::trd::NoiseStatusMCM* mNoiseMap = nullptr;
  std::shared_ptr<TProfile> mPulseHeightpro = nullptr;
  std::shared_ptr<TProfile2D> mPulseHeightperchamber = nullptr;
};

} // namespace o2::quality_control_modules::trd

#endif // QC_MODULE_TRD_DIGITSTASK_H
