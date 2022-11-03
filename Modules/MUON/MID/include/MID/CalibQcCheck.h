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
/// \file   CalibQcCheck.h
/// \author Valerie Ramillien
///

#ifndef QC_MODULE_MID_MIDCALIBQCCHECK_H
#define QC_MODULE_MID_MIDCALIBQCCHECK_H

#include "QualityControl/CheckInterface.h"

namespace o2::quality_control_modules::mid
{

class CalibQcCheck : public o2::quality_control::checker::CheckInterface
{
 public:
  /// Default constructor
  CalibQcCheck() = default;
  /// Destructor
  ~CalibQcCheck() override = default;

  // Override interface
  Quality check(std::map<std::string, std::shared_ptr<MonitorObject>>* moMap) override;
  void beautify(std::shared_ptr<MonitorObject> mo, Quality checkResult = Quality::Null) override;
  std::string getAcceptedType() override;

 private:
  ///////////////////////////
  float nTF = 0;
  float nNoiseRof = 0;
  float nDeadRof = 0;
  float scaleTime = 0.0114048; // 128 orb/TF * 3564 BC/orb * 25ns

  ClassDefOverride(CalibQcCheck, 2);
};

} // namespace o2::quality_control_modules::mid

#endif // QC_MODULE_MID_MIDCALIBQCCHECK_H
