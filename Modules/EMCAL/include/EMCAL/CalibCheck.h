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
/// \file   CalibCheck.h
/// \author Sierra Cantway
///

#ifndef QC_MODULE_EMCAL_EMCALCALIBCHECK_H
#define QC_MODULE_EMCAL_EMCALCALIBCHECK_H

#include "QualityControl/CheckInterface.h"

namespace o2::quality_control_modules::emcal
{

/// \brief  Check whether a plot is good or not.
///
/// \author Sierra Cantway
class CalibCheck : public o2::quality_control::checker::CheckInterface
{
 public:
  /// Default constructor
  CalibCheck() = default;
  /// Destructor
  ~CalibCheck() override = default;

  // Override interface
  void configure() override;
  Quality check(std::map<std::string, std::shared_ptr<MonitorObject>>* moMap) override;
  void beautify(std::shared_ptr<MonitorObject> mo, Quality checkResult = Quality::Null) override;

 private:
  /************************************************
   * threshold cuts                               *
   ************************************************/

  float mBadThresholdMaskStatsAll = 10.;                      ///< Bad Threshold used in the Max Stats All bad and dead channels check
  float mBadThresholdTimeCalibCoeff = 10.;                    ///< Bad Threshold used in the time Calib Coeff points outside of mean check
  float mBadThresholdFractionGoodCellsEvent = 0.;             ///< Bad Threshold used in the fraction Good Cells per Event check
  float mBadThresholdFractionGoodCellsSupermodule = 0.;       ///< Bad Threshold used in the fraction Good Cells per Supermodule check
  float mBadThresholdCellAmplitudeSupermoduleCalibPHYS = 10.; ///< Bad Threshold used in the PHYS Cell amplitude (Calib) vs. supermodule ID check
  float mBadThresholdCellTimeSupermoduleCalibPHYS = 10.;      ///< Bad Threshold used in the PHYS Cell Time (Calib) vs. supermodule ID (High gain) check
  float mBadThresholdChannelsFEC = 200.;                      ///< Bad Threshold used in the Number of Dead/Bad/Dead+Bad per FEC check

  float mMedThresholdMaskStatsAll = 10.;                      ///< Medium Threshold used in the Max Stats All bad and dead channels check
  float mMedThresholdTimeCalibCoeff = 10.;                    ///< Medium Threshold used in the time Calib Coeff points outside of mean check
  float mMedThresholdFractionGoodCellsEvent = 10.;            ///< Medium Threshold used in the fraction Good Cells per Event check
  float mMedThresholdFractionGoodCellsSupermodule = 10.;      ///< Medium Threshold used in the fraction Good Cells per Supermodule check
  float mMedThresholdCellAmplitudeSupermoduleCalibPHYS = 10.; ///< Medium Threshold used in the PHYS Cell amplitude (Calib) vs. supermodule ID check
  float mMedThresholdCellTimeSupermoduleCalibPHYS = 10.;      ///< Medium Threshold used in the PHYS Cell Time (Calib) vs. supermodule ID (High gain) check
  float mMedThresholdChannelsFEC = 100.;                      ///< Bad Threshold used in the Number of Dead/Bad/Dead+Bad per FEC check

  float mSigmaTimeCalibCoeff = 2.; ///< Number of sigmas used in the timeCalibCoeff points outside of mean check

  ClassDefOverride(CalibCheck, 1);
};

} // namespace o2::quality_control_modules::emcal

#endif // QC_MODULE_EMCAL_EMCALCALIBCHECK_H
