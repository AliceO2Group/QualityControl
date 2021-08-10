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
/// \file   ITSClusterCheck.cxx
/// \author Artem Isakov
/// \author LiAng Zhang
/// \author Jian Liu
///

#include "ITS/ITSClusterCheck.h"
#include "QualityControl/MonitorObject.h"
#include "QualityControl/Quality.h"

#include <fairlogger/Logger.h>
#include <TList.h>
#include <TH2.h>
#include <TText.h>

namespace o2::quality_control_modules::its
{

void ITSClusterCheck::configure(std::string) {}

Quality ITSClusterCheck::check(std::map<std::string, std::shared_ptr<MonitorObject>>* moMap)
{
  Quality result = Quality::Null;
  std::map<std::string, std::shared_ptr<MonitorObject>>::iterator iter;
  for (iter = moMap->begin(); iter != moMap->end(); ++iter) {
    if (iter->second->getName() == "Layer0/AverageClusterSize") {
      auto* h = dynamic_cast<TH2D*>(iter->second->getObject());
      if (h->GetMaximum() > 30) {
        result = Quality::Bad;
      } else {
        result = Quality::Good;
      }
    }
  }
  return result;
}

std::string ITSClusterCheck::getAcceptedType() { return "TH2D"; }

void ITSClusterCheck::beautify(std::shared_ptr<MonitorObject> mo, Quality checkResult)
{
  auto* h = dynamic_cast<TH2D*>(mo->getObject());
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
