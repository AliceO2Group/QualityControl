// Copyright 2023 CERN and copyright holders of ALICE O2.
// See https://alice-o2.web.cern.ch/copyright for details of the copyright holders.
// All rights not expressly granted are reserved.
//
// This software is distributed under the terms of the GNU General Public
// License v3 (GPL Version 3), copied verbatim in the file "COPYING".
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.

#ifndef QC_MODULE_FDD_OUTOFBUNCHCOLLFEEMODULESCHECK_H
#define QC_MODULE_FDD_OUTOFBUNCHCOLLFEEMODULESCHECK_H

#include "QualityControl/CheckInterface.h"

#include "FDDBase/Constants.h"
#include "CommonConstants/LHCConstants.h"

#include <string>

namespace o2::quality_control_modules::fdd
{

class OutOfBunchCollFeeModulesCheck : public o2::quality_control::checker::CheckInterface
{
 public:
  OutOfBunchCollFeeModulesCheck() = default;
  ~OutOfBunchCollFeeModulesCheck() override = default;

  void configure() override;
  Quality check(std::map<std::string, std::shared_ptr<MonitorObject>>* moMap) override;
  void beautify(std::shared_ptr<MonitorObject> mo, Quality checkResult = Quality::Null) override;  ClassDefOverride(OutOfBunchCollFeeModulesCheck, 1);

 private:
  float mThreshWarning;
  float mThreshError;
  float mFractionOutOfBunchColl = 0;
  constexpr static std::size_t sBCperOrbit = o2::constants::lhc::LHCMaxBunches;
};

} // namespace o2::quality_control_modules::fdd

#endif // QC_MODULE_FDD_OUTOFBUNCHCOLLFEEMODULESCHECK_H