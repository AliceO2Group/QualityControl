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
#include "ITSMFTReconstruction/DecodingStat.h"

#include <fairlogger/Logger.h>
#include <TList.h>
#include <TH2.h>
#include <iostream>
#include "Common/Utils.h"

namespace o2::quality_control_modules::its
{

Quality ITSDecodingErrorCheck::check(std::map<std::string, std::shared_ptr<MonitorObject>>* moMap)
{
  std::vector<int> vDecErrorLimits = convertToArray<int>(o2::quality_control_modules::common::getFromConfig<string>(mCustomParameters, "DecLinkErrorLimits", ""));
  if (vDecErrorLimits.size() != o2::itsmft::GBTLinkDecodingStat::NErrorsDefined) {
    LOG(error) << "Incorrect vector with DecodingError limits, check .json" << ENDM;
    doFlatCheck = true;
  }

  Quality result = Quality::Null;

  for (auto& [moName, mo] : *moMap) {
    (void)moName;
    if (mo->getName() == "General/ChipErrorPlots") {
      result = Quality::Good;
      auto* h = dynamic_cast<TH1D*>(mo->getObject());
      if (h->GetMaximum() > 200)
        result.set(Quality::Bad);
    }

    if (mo->getName() == "General/LinkErrorPlots") {
      result = Quality::Good;
      auto* h = dynamic_cast<TH1D*>(mo->getObject());

      if (doFlatCheck) {
        if (h->GetMaximum() > 200)
          result.set(Quality::Bad);

      } else {
        for (int iBin = 1; iBin <= h->GetNbinsX(); iBin++) {

          if (vDecErrorLimits[iBin - 1] < 0)
            continue; // skipping bin
          if (vDecErrorLimits[iBin - 1] <= h->GetBinContent(iBin)) {
            vListErrorIdBad.push_back(iBin - 1);
            result.set(Quality::Bad);
            result.addReason(o2::quality_control::FlagReasonFactory::Unknown(), Form("BAD: ID = %d, %s", iBin - 1, std::string(statistics.ErrNames[iBin - 1]).c_str()));
          } else if (vDecErrorLimits[iBin - 1] / 2 < h->GetBinContent(iBin)) {
            vListErrorIdMedium.push_back(iBin - 1);
            if (result != Quality::Bad) {
              result.addReason(o2::quality_control::FlagReasonFactory::Unknown(), Form("Medium: ID = %d, %s", iBin - 1, std::string(statistics.ErrNames[iBin - 1]).c_str()));
              result.set(Quality::Medium);
            }
          }
        }
      }
    }
  }
  return result;
}

void ITSDecodingErrorCheck::beautify(std::shared_ptr<MonitorObject> mo, Quality checkResult)
{
  std::vector<string> vPlotWithTextMessage = convertToArray<string>(o2::quality_control_modules::common::getFromConfig<string>(mCustomParameters, "plotWithTextMessage", ""));
  std::vector<string> vTextMessage = convertToArray<string>(o2::quality_control_modules::common::getFromConfig<string>(mCustomParameters, "textMessage", ""));
  std::map<string, string> ShifterInfoText;

  if ((int)vTextMessage.size() == (int)vPlotWithTextMessage.size()) {
    for (int i = 0; i < (int)vTextMessage.size(); i++) {
      ShifterInfoText[vPlotWithTextMessage[i]] = vTextMessage[i];
    }
  } else
    ILOG(Warning, Support) << "Bad list of plot with TextMessages for shifter, check .json" << ENDM;

  std::shared_ptr<TLatex> tShifterInfo = std::make_shared<TLatex>(0.005, 0.006, Form("#bf{%s}", TString(ShifterInfoText[mo->getName()]).Data()));
  tShifterInfo->SetTextSize(0.04);
  tShifterInfo->SetTextFont(43);
  tShifterInfo->SetNDC();

  TString status;
  int textColor;
  if ((mo->getName() == "General/LinkErrorPlots") || (mo->getName() == "General/ChipErrorPlots")) {
    auto* h = dynamic_cast<TH1D*>(mo->getObject());
    if (checkResult == Quality::Good) {
      status = "Quality::GOOD";
      textColor = kGreen;
    } else {

      if (checkResult == Quality::Bad) {
        status = "Quality::BAD (call expert)";
        for (int id = 0; id < vListErrorIdBad.size(); id++) {
          int currentError = vListErrorIdBad[id];
          tInfo = std::make_shared<TLatex>(0.12, 0.835 - 0.04 * (id + 1), Form("BAD: ID = %d, %s", currentError, std::string(statistics.ErrNames[currentError]).c_str()));
          tInfo->SetTextColor(kRed + 2);
          tInfo->SetTextSize(0.04);
          tInfo->SetTextFont(43);
          tInfo->SetNDC();
          h->GetListOfFunctions()->Add(tInfo->Clone());
        }
        textColor = kRed + 2;
      }
      if (vListErrorIdMedium.size() > 0) {
        if (checkResult == Quality::Medium) {
          status = "Quality::Medium";
          textColor = kOrange;
        }
        for (int id = 0; id < vListErrorIdMedium.size(); id++) {
          int currentError = vListErrorIdMedium[id];
          tInfo = std::make_shared<TLatex>(0.12, 0.6 - 0.04 * (id + 1), Form("Medium: ID = %d, %s", currentError, std::string(statistics.ErrNames[currentError]).c_str()));
          tInfo->SetTextColor(kOrange + 1);
          tInfo->SetTextSize(0.04);
          tInfo->SetTextFont(43);
          tInfo->SetNDC();
          h->GetListOfFunctions()->Add(tInfo->Clone());
        }
      }
    }
    tInfo = std::make_shared<TLatex>(0.05, 0.95, Form("#bf{%s}", status.Data()));
    tInfo->SetTextColor(textColor);
    tInfo->SetTextSize(0.06);
    tInfo->SetTextFont(43);
    tInfo->SetNDC();
    h->GetListOfFunctions()->Add(tInfo->Clone());
    if (ShifterInfoText[mo->getName()] != "")
      h->GetListOfFunctions()->Add(tShifterInfo->Clone());
  }
  vListErrorIdBad.clear();
  vListErrorIdMedium.clear();
}

} // namespace o2::quality_control_modules::its
