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
/// \file   CheckRawTime.h
/// \author Nicolo' Jacazio
/// \brief  Checker for TOF raw times
///

#ifndef QC_MODULE_TOF_TOFCHECKRAWSTIME_H
#define QC_MODULE_TOF_TOFCHECKRAWSTIME_H

#include "QualityControl/CheckInterface.h"
#include "QualityControl/MonitorObject.h"
#include "QualityControl/Quality.h"

namespace o2::quality_control_modules::tof
{

class CheckRawTime : public o2::quality_control::checker::CheckInterface
{
 public:
  /// Default constructor
  CheckRawTime() = default;
  /// Destructor
  ~CheckRawTime() override = default;

  // Override interface
  void configure(std::string name) override;
  Quality check(std::map<std::string, std::shared_ptr<MonitorObject>>* moMap) override;
  void beautify(std::shared_ptr<MonitorObject> mo, Quality checkResult) override;
  std::string getAcceptedType() override;

 private:
  /// Minimum value for TOF average raw time
  float mMinRawTime;
  /// Maximum value for TOF average raw time
  float mMaxRawTime;
  /// Mean of the TOF raw time distribution
  float mRawTimeMean = 0.f;
  /// Integral of the TOF raw time distribution in the peak region i.e. within minTOFrawTime and maxTOFrawTime
  float mRawTimePeakIntegral = 0.f;
  /// Integral of the TOF raw time distribution in the whole histogram range
  float mRawTimeIntegral = 0.f;

  ClassDefOverride(CheckRawTime, 1);
};

} // namespace o2::quality_control_modules::tof

#endif // QC_MODULE_TOF_TOFCHECKRAWSTIME_H
