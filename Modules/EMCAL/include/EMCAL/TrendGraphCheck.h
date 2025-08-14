// Copyright 2019-2020 CERN and copyright holders of ALICE O2.
// See https://alice-o2.web.cern.ch/copyright for details of the copyright
// holders. All rights not expressly granted are reserved.
//
// This software is distributed under the terms of the GNU General Public
// License v3 (GPL Version 3), copied verbatim in the file "COPYING".
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.

///
/// \file   TrendGraphCheck.h
/// \author Ananya Rai
///

#ifndef QC_MODULE_EMCAL_EMCALTRENDGRAPHCHECK_H
#define QC_MODULE_EMCAL_EMCALTRENDGRAPHCHECK_H

#include "QualityControl/CheckInterface.h"

namespace o2::quality_control_modules::emcal
{

/// \brief  Check if the trend rate is as expected.
///
/// \author Ananya Rai
class TrendGraphCheck : public o2::quality_control::checker::CheckInterface
{
 public:
  /// Default constructor
  TrendGraphCheck() = default;
  /// Destructor
  ~TrendGraphCheck() override = default;

  // Override interface
  void configure() override;
  Quality
    check(std::map<std::string, std::shared_ptr<MonitorObject>>* moMap) override;
  void beautify(std::shared_ptr<MonitorObject> mo,
                Quality checkResult = Quality::Null) override;

 private:
  /************************************************
   * threshold cuts                               *
   ************************************************/
  int mPeriodMovAvg = 5; ///< Period for moving average
  double mBadThresholdLow =
    10; ///< If rate lower than this threshold - medium, keep checking
  double mBadThresholdHigh =
    1000; ///< If rate higher than this thereshold - medium, keep checking
  double mBadDiff =
    20; ///< If difference is higher than this between consecutive rates - bad

  ClassDefOverride(TrendGraphCheck, 5);
};

} // namespace o2::quality_control_modules::emcal

#endif // QC_MODULE_EMCAL_EMCALTRENDGRAPHCHECK_H
