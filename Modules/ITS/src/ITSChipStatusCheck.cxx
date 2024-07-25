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
  Quality result = Quality::Null;
  for (auto& [moName, mo] : *moMap) {
    if (mo->getName() == "StaveStatusOverview") {
      result = Quality::Good;
      auto* h = dynamic_cast<TH2Poly*>(mo->getObject());
      if (h == nullptr) {
        ILOG(Error, Support) << "could not cast ChipError plots to TH2Poly" << ENDM;
        continue;
      }
      for (int ilayer = 0; ilayer < NLayer; ilayer++) {
        for (int ibin = StaveBoundary[ilayer] + 1; ibin <= StaveBoundary[ilayer + 1]; ++ibin) {
          if (abs(h->GetBinContent(ibin) - 1) < 0.01) {
            result = Quality::Bad;
            TString text = Form("BAD: At least one stave is without data", ilayer, ibin - StaveBoundary[ilayer] - 1);
            vBadStaves.push_back(Form("L%d_%d", ilayer, ibin - StaveBoundary[ilayer]));
            result.addFlag(o2::quality_control::FlagTypeFactory::Unknown(), text.Data());
          }
        }
      }
    }
  }
  return result;
}

void ITSChipStatusCheck::beautify(std::shared_ptr<MonitorObject> mo, Quality checkResult)
{
  TString status;
  int textColor;
  if ((mo->getName() == "StaveStatusOverview")) {
    auto* h = dynamic_cast<TH2Poly*>(mo->getObject());
    if (h == nullptr) {
      ILOG(Error, Support) << "could not cast StatusOverview to TH2Poly*" << ENDM;
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
