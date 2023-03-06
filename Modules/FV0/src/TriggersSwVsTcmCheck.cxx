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
/// \file   TriggersSwVsTcmCheck.cxx
/// \author Dawid Skora dawid.mateusz.skora@cern.ch
///

#include "FV0/TriggersSwVsTcmCheck.h"
#include "QualityControl/MonitorObject.h"
#include "QualityControl/Quality.h"
#include "QualityControl/QcInfoLogger.h"
// ROOT
#include <TH1.h>
#include <TH2.h>
#include <TPaveText.h>
#include <TLine.h>
#include <TList.h>

#include <DataFormatsQualityControl/FlagReasons.h>

using namespace std;
using namespace o2::quality_control;

namespace o2::quality_control_modules::fv0
{

constexpr int kBinSwOnly = 1;
constexpr int kBinTcmOnly = 2;

void TriggersSwVsTcmCheck::configure() {
    if (auto param = mCustomParameters.find("ccdbUrl"); param != mCustomParameters.end()) {
      setCcdbUrl(param->second);
      ILOG(Debug, Support) << "configure() : using deadChannelMap from CCDB, configured url = " << param->second << ENDM;
    } else {
      setCcdbUrl("o2-ccdb.internal");
      ILOG(Debug, Support) << "configure() : using deadChannelMap from CCDB, default url = "
                           << "o2-ccdb.internal" << ENDM;
    }
}

Quality TriggersSwVsTcmCheck::check(std::map<std::string, std::shared_ptr<MonitorObject>>* moMap)
{
  Quality result = Quality::Null;
  int mNumErrors = 0;
  for (auto& [moName, mo] : *moMap) {
    (void)moName;
    if (mo->getName() == "TriggersSoftwareVsTCM") {
      auto* histogram = dynamic_cast<TH2F*>(mo->getObject());
      result = Quality::Good;
      int numberOfBinsX = histogram->GetNbinsX();
      for (int i = 1; i <= numberOfBinsX; i++) {
        if ((bool)(histogram->GetBinContent(i, kBinSwOnly)) == (bool)(histogram->GetBinContent(i, kBinTcmOnly))) {
          mNumErrors++;
          if (result.isBetterThan(Quality::Bad)) {
              result.set(Quality::Bad);
          }   
          result.addReason(FlagReasonFactory::Unknown(),
                  "TriggersSoftwareVsTCM. < \"Error\" Only one of the SW or TCM triggers was activated");
        }
      }
    }
  }
  result.addMetadata("nErrors", std::to_string(mNumErrors));
  return result;
}

std::string TriggersSwVsTcmCheck::getAcceptedType() { return "TH2"; }

void TriggersSwVsTcmCheck::beautify(std::shared_ptr<MonitorObject> mo, Quality checkResult)
{
  if (mo->getName() == "TriggersSoftwareVsTCM") {
    auto* histogram = dynamic_cast<TH2F*>(mo->getObject());

    TPaveText* msg = new TPaveText(0.15, 0.2, 0.85, 0.45, "NDC");
    histogram->GetListOfFunctions()->Add(msg);
    msg->SetName(Form("%s_msg", mo->GetName()));
    msg->Clear();
    auto reasons = checkResult.getReasons();
    for (int i = 0; i < int(reasons.size()); i++) {
      msg->AddText(reasons[i].second.c_str());
      if (i > 4) {
        msg->AddText("et al ... ");
        break;
      }
    }
    int color = kBlack;
    if (checkResult == Quality::Good) {
      color = kGreen + 1;
      msg->AddText(">> Quality::Good <<");
    } else if (checkResult == Quality::Medium) {
      color = kOrange - 1;
      msg->AddText(">> Quality::Medium <<");
    } else if (checkResult == Quality::Bad) {
      color = kRed;
      msg->AddText(">> Quality::Bad <<");
    }
    msg->SetFillStyle(1);
    msg->SetLineWidth(3);
    msg->SetLineColor(color);
    msg->SetShadowColor(color);
    msg->SetTextColor(color);
    msg->SetMargin(0.0);
  }
}

} // namespace o2::quality_control_modules::fv0
