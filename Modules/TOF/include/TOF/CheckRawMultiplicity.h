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
/// \author Nicolò Jacazio <nicolo.jacazio@cern.ch>
/// \brief  Checker for the hit multiplicity obtained with the TaskDigits
///

#ifndef QC_MODULE_TOF_TOFCHECKRAWSMULTI_H
#define QC_MODULE_TOF_TOFCHECKRAWSMULTI_H

#include "QualityControl/CheckInterface.h"
#include "Base/MessagePad.h"

namespace o2::quality_control_modules::tof
{

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
  std::string getAcceptedType() override { return "TH1I"; }

  /// Running modes available
  static constexpr int kModeCollisions = 0; /// Standard running mode with collisions
  static constexpr int kModeCosmics = 1;    /// Running mode with collisions

 private:
  // Running configurable parameters
  /// Minimum number of entries in MO before message can be printed
  double mMinEntriesBeforeMessage = -1.0;
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
  /// Messages to print on the output PAD
  MessagePad mShifterMessages;

  ClassDefOverride(CheckRawMultiplicity, 2);
};

} // namespace o2::quality_control_modules::tof

#endif // QC_MODULE_TOF_TOFCHECKRAWSMULTI_H
