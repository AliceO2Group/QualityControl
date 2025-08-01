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
/// \file   ITSChipStatusCheck.cxx
/// \author Zhen Zhang
///

#include "ITS/ITSChipStatusCheck.h"
#include "QualityControl/MonitorObject.h"
#include "QualityControl/Quality.h"
#include "QualityControl/QcInfoLogger.h"
#include "ITSMFTReconstruction/DecodingStat.h"
#include <DataFormatsQualityControl/FlagTypeFactory.h>

#include <fairlogger/Logger.h>
#include "Common/Utils.h"

namespace o2::quality_control_modules::its
{

Quality ITSChipStatusCheck::check(std::map<std::string, std::shared_ptr<MonitorObject>>* moMap)
{

  // limits to be used as "X,Y" --> BAD if at least X FFEIDs have at least Y chips each into error
  std::vector<float> feeidlimitsIB = convertToArray<float>(o2::quality_control_modules::common::getFromConfig<std::string>(mCustomParameters, "feeidlimitsIB", ""));
  std::vector<float> feeidlimitsML = convertToArray<float>(o2::quality_control_modules::common::getFromConfig<std::string>(mCustomParameters, "feeidlimitsML", ""));
  std::vector<float> feeidlimitsOL = convertToArray<float>(o2::quality_control_modules::common::getFromConfig<std::string>(mCustomParameters, "feeidlimitsOL", ""));
  std::vector<int> excludedfeeid = convertToArray<int>(o2::quality_control_modules::common::getFromConfig<std::string>(mCustomParameters, "excludedfeeid", ""));

  if (feeidlimitsIB.size() != 2) {
    ILOG(Error, Support) << "Incorrect setting for feeidlimitsIB, check .json. Using default 1,1" << ENDM;
    feeidlimitsIB.clear();
    feeidlimitsIB = std::vector<float>{ 1, 1. };
  }
  if (feeidlimitsML.size() != 2) {
    ILOG(Error, Support) << "Incorrect setting for feeidlimitsML, check .json. Using default 1,1" << ENDM;
    feeidlimitsML.clear();
    feeidlimitsML = std::vector<float>{ 1, 1. };
  }
  if (feeidlimitsOL.size() != 2) {
    ILOG(Error, Support) << "Incorrect setting for feeidlimitsOL, check .json. Using default 1,1" << ENDM;
    feeidlimitsOL.clear();
    feeidlimitsOL = std::vector<float>{ 1, 1. };
  }

  Quality result = Quality::Null;
  for (auto& [moName, mo] : *moMap) {

    if (mo->getName() == "StaveStatusOverview") {
      result = Quality::Good;
      auto* h = dynamic_cast<TH2Poly*>(mo->getObject());
      if (h == nullptr) {
        ILOG(Error, Support) << "could not cast StaveStatusOverview plot from ChipError to TH2Poly" << ENDM;
        continue;
      }
      for (int ilayer = 0; ilayer < NLayer; ilayer++) {
        for (int ibin = StaveBoundary[ilayer] + 1; ibin <= StaveBoundary[ilayer + 1]; ++ibin) {
          if (abs(h->GetBinContent(ibin) - 1) < 0.01) {
            result = Quality::Bad;
            TString text = Form("BAD: At least one stave is without data");
            vBadStaves.push_back(Form("L%d_%d", ilayer, ibin - StaveBoundary[ilayer] - 1));
            result.addFlag(o2::quality_control::FlagTypeFactory::Unknown(), text.Data());
          }
        }
      }
    }

    if (mo->getName() == "FEEIDOverview") {
      result = Quality::Good;
      auto* h = dynamic_cast<TH1D*>(mo->getObject());
      if (h == nullptr) {
        ILOG(Error, Support) << "could not cast FEEIDoverview plot from ChipStatus to TH1D" << ENDM;
        continue;
      }

      int nBadIB = 0;
      int nBadML = 0;
      int nBadOL = 0;
      for (int ifee = 0; ifee < h->GetNbinsX(); ifee++) {

        if (excludedfeeid.size() > 0 && std::find(excludedfeeid.begin(), excludedfeeid.end(), ifee) != excludedfeeid.end()) {
          continue;
        }

        if (ifee >= FeeIDBoundaryVsBarrel[0] && ifee < FeeIDBoundaryVsBarrel[1] && h->GetBinContent(ifee + 1) >= feeidlimitsIB[1]) {
          nBadIB++;
        }
        if (ifee >= FeeIDBoundaryVsBarrel[1] && ifee < FeeIDBoundaryVsBarrel[2] && h->GetBinContent(ifee + 1) >= feeidlimitsML[1]) {
          nBadML++;
        }
        if (ifee >= FeeIDBoundaryVsBarrel[2] && ifee < FeeIDBoundaryVsBarrel[3] && h->GetBinContent(ifee + 1) >= feeidlimitsOL[1]) {
          nBadOL++;
        }
      }

      if (nBadIB >= feeidlimitsIB[0] || nBadML >= feeidlimitsML[0] || nBadOL >= feeidlimitsOL[0]) {
        result = Quality::Bad;
        TString text = "At least one FEEid with large number of missing chips";
        result.addFlag(o2::quality_control::FlagTypeFactory::Unknown(), text.Data());
      }
    }
  }
  return result;
}

void ITSChipStatusCheck::beautify(std::shared_ptr<MonitorObject> mo, Quality checkResult)
{
  TString status;
  int textColor;

  if ((std::string)mo->GetName() == "FEEIDOverview") {
    auto* h = dynamic_cast<TH1D*>(mo->getObject());
    if (h == nullptr) {
      ILOG(Error, Support) << "could not cast FEEIDOverview to TH1D*" << ENDM;
      return;
    }

    if (checkResult == Quality::Good) {
      status = "Quality::GOOD";
      textColor = kGreen;
    } else if (checkResult == Quality::Bad) {
      status = "Quality::BAD (call expert)";
      textColor = kRed + 2;
    }

    tInfo = std::make_shared<TLatex>(0.05, 0.95, Form("#bf{%s}", status.Data()));
    tInfo->SetTextColor(textColor);
    tInfo->SetTextSize(0.06);
    tInfo->SetTextFont(43);
    tInfo->SetNDC();
    h->GetListOfFunctions()->Add(tInfo->Clone());
  }

  if ((std::string)mo->getName() == "StaveStatusOverview") {
    auto* h = dynamic_cast<TH2Poly*>(mo->getObject());
    if (h == nullptr) {
      ILOG(Error, Support) << "could not cast StaveStatusOverview to TH2Poly*" << ENDM;
      return;
    }
    if (checkResult == Quality::Good) {
      status = "Quality::GOOD";
      textColor = kGreen;
    } else {

      if (checkResult == Quality::Bad) {
        status = "Quality::BAD (call expert)";
        for (int id = 0; id < vBadStaves.size(); id++) {
          tInfo = std::make_shared<TLatex>(0.12, 0.835 - 0.04 * (id + 1), Form("BAD: stave %s is empty", vBadStaves[id].Data()));
          tInfo->SetTextColor(kRed + 2);
          tInfo->SetTextSize(0.04);
          tInfo->SetTextFont(43);
          tInfo->SetNDC();
          h->GetListOfFunctions()->Add(tInfo->Clone());
        }
        textColor = kRed + 2;
      }
    }
    tInfo = std::make_shared<TLatex>(0.05, 0.95, Form("#bf{%s}", status.Data()));
    tInfo->SetTextColor(textColor);
    tInfo->SetTextSize(0.06);
    tInfo->SetTextFont(43);
    tInfo->SetNDC();
    h->GetListOfFunctions()->Add(tInfo->Clone());
  }
  vBadStaves.clear();
}

} // namespace o2::quality_control_modules::its
