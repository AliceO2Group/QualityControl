// Copyright CERN and copyright holders of ALICE O2. This software is
// distributed under the terms of the GNU General Public License v3 (GPL
// Version 3), copied verbatim in the file "COPYING".
//
// See http://alice-o2.web.cern.ch/license for full licensing information.
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
  Quality result = Quality::Null;
  std::map<std::string, std::shared_ptr<MonitorObject>>::iterator iter;
  for (iter = moMap->begin(); iter != moMap->end(); ++iter) {
    if (iter->second->getName() == "LaneStatus/laneStatusFlagFAULT") {
      auto* h = dynamic_cast<TH2I*>(iter->second->getObject());
      if (h->GetMaximum() > 0) {
        result = Quality::Bad;
      } else {
        result = Quality::Good;
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

  if (checkResult == Quality::Good) {
    tInfo->SetText(0.1, 0.8, "Quality::GOOD");
    tInfo->SetTextColor(kGreen);
  } else if (checkResult == Quality::Bad) {
    tInfo->SetText(0.1, 0.8, "Quality::BAD");
    tInfo->SetTextColor(kRed);
  }
  tInfo->SetTextSize(17);
  tInfo->SetNDC();
  h->GetListOfFunctions()->Add(tInfo);
}

} // namespace o2::quality_control_modules::its
