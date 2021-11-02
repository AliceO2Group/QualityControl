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
/// \file   ITSFeeCheck.cxx
/// \author LiAng Zhang
/// \author Jian Liu
///

#include "ITS/ITSFeeCheck.h"
#include "QualityControl/MonitorObject.h"
#include "QualityControl/Quality.h"

#include <fairlogger/Logger.h>
#include <TList.h>
#include <TH2.h>
#include <TText.h>

namespace o2::quality_control_modules::its
{

void ITSFeeCheck::configure(std::string) {}

Quality ITSFeeCheck::check(std::map<std::string, std::shared_ptr<MonitorObject>>* moMap)
{
  Quality result = 0;                                                   //Fee Checker will check three plot in Fee Task, and will store the
  std::map<std::string, std::shared_ptr<MonitorObject>>::iterator iter; //quality result in a three digits number:
  for (iter = moMap->begin(); iter != moMap->end(); ++iter) {           //   XXX
    if (iter->second->getName() == "LaneStatus/laneStatusFlagFAULT") {  //   |||-> laneStatusFlagFAULT
      auto* h = dynamic_cast<TH2I*>(iter->second->getObject());         //   ||-> laneStatusFlagERROR
      if (h->GetMaximum() > 0) {                                        //   |-> laneStatusFlagWARNING
        result = result.getLevel() + 1;                                 //the number for each digits is correspond:
      }                                                                 //0: Good, 1: Bad
    } else if (iter->second->getName() == "LaneStatus/laneStatusFlagERROR") {
      auto* h = dynamic_cast<TH2I*>(iter->second->getObject());
      if (h->GetMaximum() > 0) {
        result = result.getLevel() + 10;
      }
    } else if (iter->second->getName() == "LaneStatus/laneStatusFlagWARNING") {
      auto* h = dynamic_cast<TH2I*>(iter->second->getObject());
      if (h->GetMaximum() > 0) {
        result = result.getLevel() + 100;
      }
    }
  }
  return result;
}

std::string ITSFeeCheck::getAcceptedType() { return "TH2I"; }

void ITSFeeCheck::beautify(std::shared_ptr<MonitorObject> mo, Quality checkResult)
{
  auto* h = dynamic_cast<TH2I*>(mo->getObject());
  auto* tInfo = new TText();
  if (mo->getName() == "LaneStatus/laneStatusFlagFAULT") {
    if ((checkResult.getLevel() % 10) == 0) {
      tInfo->SetText(0.1, 0.8, "Quality::GOOD");
      tInfo->SetTextColor(kGreen);
    } else if ((checkResult.getLevel() % 10) == 1) {
      tInfo->SetText(0.1, 0.8, "Quality::BAD(call expert)");
      tInfo->SetTextColor(kRed);
    }
    tInfo->SetTextSize(17);
    tInfo->SetNDC();
    h->GetListOfFunctions()->Add(tInfo);
  } else if (mo->getName() == "LaneStatus/laneStatusFlagERROR") {
    if (((checkResult.getLevel() % 100) / 10) == 0) {
      tInfo->SetText(0.1, 0.8, "Quality::GOOD");
      tInfo->SetTextColor(kGreen);
    } else if (((checkResult.getLevel() % 100) / 10) == 1) {
      tInfo->SetText(0.1, 0.8, "Quality::BAD(call expert)");
      tInfo->SetTextColor(kRed);
    }
    tInfo->SetTextSize(17);
    tInfo->SetNDC();
    h->GetListOfFunctions()->Add(tInfo);
  } else if (mo->getName() == "LaneStatus/laneStatusFlagWARNING") {
    if ((checkResult.getLevel() / 100) == 0) {
      tInfo->SetText(0.1, 0.8, "Quality::GOOD");
      tInfo->SetTextColor(kGreen);
    } else if ((checkResult.getLevel() / 100) == 1) {
      tInfo->SetText(0.1, 0.8, "Quality::BAD(call expert)");
      tInfo->SetTextColor(kRed);
    }
    tInfo->SetTextSize(17);
    tInfo->SetNDC();
    h->GetListOfFunctions()->Add(tInfo);
  }
}

} // namespace o2::quality_control_modules::its
