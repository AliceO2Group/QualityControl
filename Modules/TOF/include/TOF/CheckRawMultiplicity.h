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
#include "Base/MessagePad.h"

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
  void configure() override;
  Quality check(std::map<std::string, std::shared_ptr<MonitorObject>>* moMap) override;
  void beautify(std::shared_ptr<MonitorObject> mo, Quality checkResult) override;
  std::string getAcceptedType() override;

  /// Running modes available
  static constexpr int kModeCollisions = 0; /// Standard running mode with collisions
  static constexpr int kModeCosmics = 1;    /// Running mode with collisions

 private:
  // Running configurable parameters
  /// Running mode, cosmics or collisions
  int mRunningMode = kModeCollisions;
  /// Minimum value of TOF raw hit multiplicity
  float mMinRawHits = 10;
  /// Maximum value of TOF raw hit multiplicity
  float mMaxRawHits = 5000;
  /// Fraction of the total integral which are considered Ok at 0 mult
  float mMaxFractAtZeroMult = 0.75;
  /// Fraction of the total integral which are considered Ok at low mult
  float mMaxFractAtLowMult = 0.75;

  // User variables
  /// Mean of the TOF hit multiplicity histogram
  float mRawHitsMean = 0.f;
  /// Number of events with 0 TOF hits
  float mRawHitsZeroMultIntegral = 0.f;
  /// Number of events with low TOF hits multiplicity
  float mRawHitsLowMultIntegral = 0.f;
  /// Number of events with TOF hits multiplicity > 0
  float mRawHitsIntegral = 0.f;
  /// Messages to print on the output PAD
  MessagePad mShifterMessages;

  ClassDefOverride(CheckRawMultiplicity, 2);
};

} // namespace o2::quality_control_modules::tof

#endif // QC_MODULE_TOF_TOFCHECKRAWSMULTI_H
