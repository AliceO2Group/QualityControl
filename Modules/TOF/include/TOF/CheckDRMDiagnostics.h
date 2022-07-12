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
/// \file   CheckDRMDiagnostics.h
/// \author Nicolo' Jacazio, Pranjal Sarma
/// \brief  Checker dedicated to the study of low level raw data diagnostics words
///

#ifndef QC_MODULE_TOF_CHECKDRMDIAGNOSTICS_H
#define QC_MODULE_TOF_CHECKDRMDIAGNOSTICS_H

#include "QualityControl/CheckInterface.h"
#include "Base/MessagePad.h"

namespace o2::quality_control_modules::tof
{

/// \brief  Checker for diagnostic histogram of TOF Raw data
///
/// \author Nicolo' Jacazio, Pranjal Sarma
class CheckDRMDiagnostics : public o2::quality_control::checker::CheckInterface
{
 public:
  /// Default constructor
  CheckDRMDiagnostics() = default;
  /// Destructor
  ~CheckDRMDiagnostics() override = default;

  // Override interface
  void configure() override;
  Quality check(std::map<std::string, std::shared_ptr<MonitorObject>>* moMap) override;
  void beautify(std::shared_ptr<MonitorObject> mo, Quality checkResult) override;
  std::string getAcceptedType() override;

 private:
  /// Messages to print on the output PAD
  MessagePad mShifterMessages;

  ClassDefOverride(CheckDRMDiagnostics, 2);
};

} // namespace o2::quality_control_modules::tof

#endif // QC_MODULE_TOF_CHECKDRMDIAGNOSTICS_H
