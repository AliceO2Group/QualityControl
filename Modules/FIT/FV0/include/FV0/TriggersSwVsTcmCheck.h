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

///
/// \file   TriggersSwVsTcmCheck.h
/// \author Dawid Skora dawid.mateusz.skora@cern.ch
///

#ifndef QC_MODULE_FV0_TRIGGERSSWVSTCMCHECK_H
#define QC_MODULE_FV0_TRIGGERSSWVSTCMCHECK_H

#include "QualityControl/CheckInterface.h"

#include "FV0Base/Constants.h"

namespace o2::quality_control_modules::fv0
{

/// \brief This check provide xnor (exclusive nor) operation on SW and HW triggers
/// \author Dawid Skora dawid.mateusz.skora@cern.ch
class TriggersSwVsTcmCheck : public o2::quality_control::checker::CheckInterface
{
 public:
  TriggersSwVsTcmCheck() = default;
  ~TriggersSwVsTcmCheck() override = default;

  void configure() override;
  Quality check(std::map<std::string, std::shared_ptr<MonitorObject>>* moMap) override;
  void beautify(std::shared_ptr<MonitorObject> mo, Quality checkResult = Quality::Null) override;
  std::string getAcceptedType() override;

  ClassDefOverride(TriggersSwVsTcmCheck, 2);

 private:
  std::array<double, 4> mPositionMsgBox;
};

} // namespace o2::quality_control_modules::fv0

#endif // QC_MODULE_FV0_TRIGGERSSWVSTCMCHECK_H
