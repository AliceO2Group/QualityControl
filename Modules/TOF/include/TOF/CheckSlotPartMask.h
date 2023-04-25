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
/// \file   CheckSlotPartMask.h
/// \author Francesca Ercolessi
/// \brief  Checker for slot partecipating
///

#ifndef QC_MODULE_TOF_CHECKSLOTPARTMASK_H
#define QC_MODULE_TOF_CHECKSLOTPARTMASK_H

#include "QualityControl/CheckInterface.h"
#include "Base/MessagePad.h"

namespace o2::quality_control_modules::tof
{

class CheckSlotPartMask : public o2::quality_control::checker::CheckInterface
{
 public:
  /// Default constructor
  CheckSlotPartMask() = default;
  /// Destructor
  ~CheckSlotPartMask() override = default;

  // Override interface
  void configure() override;
  Quality check(std::map<std::string, std::shared_ptr<MonitorObject>>* moMap) override;
  void beautify(std::shared_ptr<MonitorObject> mo, Quality checkResult) override;
  std::string getAcceptedType() override;

 private:
  // Threshold number of crates missing
  int mNCrates = 36;
  // Threshold number of inefficient links
  int mNCrateIneff = 36;
  // Fraction of entries w.r.t. mean of all crates to decide if a link is inefficient
  double mIneffThreshold = 0.8;
  /// Messages to print on the output PAD
  MessagePad mShifterMessages{ "", 60.f, 13.f, 72.f, 14.f };
  /// To select to check link inefficiencies (if recovery does not work)
  int mCheckLinkInefficiency = 0;

  ClassDefOverride(CheckSlotPartMask, 2);
};

} // namespace o2::quality_control_modules::tof

#endif // QC_MODULE_TOF_CHECKDIAGNOSTICS_H
