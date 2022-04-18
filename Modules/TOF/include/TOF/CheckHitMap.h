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
/// \file   CheckHitMap.h
/// \author Nicolò Jacazio nicolo.jacazio@cern.ch
/// \brief  Checker for the hit map hit obtained with the TaskDigits
///

#ifndef QC_MODULE_TOF_TOFCHECKHITMAP_H
#define QC_MODULE_TOF_TOFCHECKHITMAP_H

#include "QualityControl/CheckInterface.h"
#include "Base/MessagePad.h"
#include "TPad.h"

namespace o2::quality_control_modules::tof
{

class CheckHitMap : public o2::quality_control::checker::CheckInterface
{
 public:
  /// Default constructor
  CheckHitMap() = default;
  /// Destructor
  ~CheckHitMap() override = default;

  // Override interface
  void configure() override;
  Quality check(std::map<std::string, std::shared_ptr<MonitorObject>>* moMap) override;
  void beautify(std::shared_ptr<MonitorObject> mo, Quality checkResult) override;
  std::string getAcceptedType() override { return "TH2F"; }

 private:
  /// Messages to print on the output PAD
  MessagePad mShifterMessages;
  /// Message regarding the PHOS module (hole)
  MessagePad mPhosModuleMessage{ "PHOS", 13.f, 38.f, 16.f, 53.f }; // Values corresponding to the PHOS hole

  ClassDefOverride(CheckHitMap, 2);
};

} // namespace o2::quality_control_modules::tof

#endif // QC_MODULE_TOF_TOFCHECKHITMAP_H
