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
/// \file   CFDEffCheck.h
/// \author Sebastian Bysiak sbysiak@cern.ch
///

#ifndef QC_MODULE_FV0_FV0CFDEFFCHECK_H
#define QC_MODULE_FV0_FV0CFDEFFCHECK_H

#include "QualityControl/CheckInterface.h"

namespace o2::quality_control_modules::fv0
{

/// \brief checks if CFD efficiency is below threshold
/// \author Sebastian Bysiak sbysiak@cern.ch
class CFDEffCheck : public o2::quality_control::checker::CheckInterface
{
 public:
  CFDEffCheck() = default;
  ~CFDEffCheck() override = default;

  void configure(std::string name) override;
  Quality check(std::map<std::string, std::shared_ptr<MonitorObject>>* moMap) override;
  void beautify(std::shared_ptr<MonitorObject> mo, Quality checkResult = Quality::Null) override;
  std::string getAcceptedType() override;

  constexpr static int sNCHANNELS_PM = 48; //48(for PM), nothing more

  ClassDefOverride(CFDEffCheck, 1);

 private:
  float mThreshWarning;
  float mThreshError;
  int mNumWarnings;
  int mNumErrors;
};

} // namespace o2::quality_control_modules::fv0

#endif // QC_MODULE_FV0_FV0CFDEFFCHECK_H
