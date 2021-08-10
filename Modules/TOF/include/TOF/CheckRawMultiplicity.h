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
/// \file   CheckRawMultiplicity.h
/// \author Nicolo' Jacazio
/// \brief  Checker for the raw hit multiplicity obtained with the TaskDigits
///

#ifndef QC_MODULE_TOF_TOFCHECKRAWSMULTI_H
#define QC_MODULE_TOF_TOFCHECKRAWSMULTI_H

#include "QualityControl/CheckInterface.h"
#include "TString.h"

namespace o2::quality_control_modules::tof
{

/// \brief  Check whether a plot is empty or not.
///
/// \author Nicolo' Jacazio
class CheckRawMultiplicity : public o2::quality_control::checker::CheckInterface
{
 public:
  /// Default constructor
  CheckRawMultiplicity() = default;
  /// Destructor
  ~CheckRawMultiplicity() override = default;

  // Override interface
  void configure(std::string name) override;
  Quality check(std::map<std::string, std::shared_ptr<MonitorObject>>* moMap) override;
  void beautify(std::shared_ptr<MonitorObject> mo, Quality checkResult) override;
  std::string getAcceptedType() override;

 private:
  /// Minimum value of TOF raw hit multiplicity
  float mMinRawHits;
  /// Maximum value of TOF raw hit multiplicity
  float mMaxRawHits;
  /// Fraction of the total integral which are considered Ok at 0 mult
  const float mFractAtZeroMult = 0.75;
  /// Fraction of the total integral which are considered Ok at low mult
  const float mFractAtLowMult = 0.75;
  /// Maximum average TOF raw hit multiplicity in Pb-Pb
  const float mMaxTOFRawHitsPbPb = 500;
  /// Mean of the TOF hit multiplicity histogram
  float mRawHitsMean = 0.f;
  /// Number of events with 0 TOF hits
  float mRawHitsZeroMultIntegral = 0.f;
  /// Number of events with low TOF hits multiplicity
  float mRawHitsLowMultIntegral = 0.f;
  /// Number of events with TOF hits multiplicity > 0
  float mRawHitsIntegral = 0.f;
  /// Message to print
  TString shifter_msg = "";

  ClassDefOverride(CheckRawMultiplicity, 1);
};

} // namespace o2::quality_control_modules::tof

#endif // QC_MODULE_TOF_TOFCHECKRAWSMULTI_H
