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
/// \file   ITSDecodingErrorCheck.cxx
/// \author Zhen Zhang
///

#include "ITS/ITSDecodingErrorCheck.h"
#include "QualityControl/MonitorObject.h"
#include "QualityControl/Quality.h"

#include <fairlogger/Logger.h>
#include <TList.h>
#include <TH2.h>
#include <iostream>

namespace o2::quality_control_modules::its
{

Quality ITSDecodingErrorCheck::check(std::map<std::string, std::shared_ptr<MonitorObject>>* moMap)
{
  Quality result = Quality::Null;

  for (auto& [moName, mo] : *moMap) {
    (void)moName;
    if ((mo->getName() == "General/LinkErrorPlots") || (mo->getName() == "General/ChipErrorPlots")) {
      result = Quality::Good;
      auto* h = dynamic_cast<TH1D*>(mo->getObject());
      if (h->GetMaximum() > 0) {
        result.set(Quality::Bad);
      }
    }
  }
  return result;
}

std::string ITSDecodingErrorCheck::getAcceptedType() { return "TH1"; }

void ITSDecodingErrorCheck::beautify(std::shared_ptr<MonitorObject> mo, Quality checkResult)
{
  TString status;
  int textColor;
  if ((mo->getName() == "General/LinkErrorPlots") || (mo->getName() == "General/ChipErrorPlots")) {
    auto* h = dynamic_cast<TH1D*>(mo->getObject());
    if (checkResult == Quality::Good) {
      status = "Quality::GOOD";
      textColor = kGreen;
    } else if (checkResult == Quality::Bad) {
      status = "Quality::BAD (call expert)";
      textColor = kRed;
    }
    tInfo = std::make_shared<TLatex>(0.12, 0.835, Form("#bf{%s}", status.Data()));
    tInfo->SetTextColor(textColor);
    tInfo->SetTextSize(0.04);
    tInfo->SetTextFont(43);
    tInfo->SetNDC();
    h->GetListOfFunctions()->Add(tInfo->Clone());
  }
}

std::vector<int> ITSDecodingErrorCheck::convertToIntArray(std::string input)
{
  std::replace(input.begin(), input.end(), ',', ' ');
  std::istringstream stringReader{ input };

  std::vector<int> result;
  int number;
  while (stringReader >> number) {
    result.push_back(number);
  }

  return result;
}

} // namespace o2::quality_control_modules::its
