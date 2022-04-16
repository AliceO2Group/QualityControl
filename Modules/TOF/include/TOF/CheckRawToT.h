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
/// \file   CheckRawToT.h
/// \author Nicolò Jacazio <nicolo.jacazio@cern.ch>
/// \brief  Checker for TOF Raw data on ToT
///

#ifndef QC_MODULE_TOF_TOFCHECKRAWSTOT_H
#define QC_MODULE_TOF_TOFCHECKRAWSTOT_H

#include "QualityControl/CheckInterface.h"
#include "Base/MessagePad.h"

namespace o2::quality_control_modules::tof
{

class CheckRawToT : public o2::quality_control::checker::CheckInterface
{
 public:
  /// Default constructor
  CheckRawToT() = default;
  /// Destructor
  ~CheckRawToT() override = default;

  // Override interface
  void configure() override;
  Quality check(std::map<std::string, std::shared_ptr<MonitorObject>>* moMap) override;
  void beautify(std::shared_ptr<MonitorObject> mo, Quality checkResult) override;
  std::string getAcceptedType() override { return "TH1F"; }

 private:
  // Running configurable parameters
  /// Minimum ToT allowed for the mean in ns
  float mMinRawToT = 10.; // ns
  /// Maximum ToT allowed for the mean in ns
  float mMaxRawToT = 15.; // ns

  // User variables
  /// Messages to print on the output PAD
  MessagePad mShifterMessages;

  ClassDefOverride(CheckRawToT, 2);
};

} // namespace o2::quality_control_modules::tof

#endif // QC_MODULE_TOF_TOFCHECKRAWSTOT_H
