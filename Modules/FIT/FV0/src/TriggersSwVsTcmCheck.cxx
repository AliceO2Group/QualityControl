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

void TriggersSwVsTcmCheck::configure()
{
  if (auto param = mCustomParameters.find("ccdbUrl"); param != mCustomParameters.end()) {
    setCcdbUrl(param->second);
    ILOG(Debug, Support) << "configure() : using deadChannelMap from CCDB, configured url = " << param->second << ENDM;
  } else {
    setCcdbUrl("o2-ccdb.internal");
    ILOG(Debug, Support) << "configure() : using deadChannelMap from CCDB, default url = "
                         << "o2-ccdb.internal" << ENDM;
  }

  mPositionMsgBox = { 0.15, 0.75, 0.85, 0.9 };
  if (auto param = mCustomParameters.find("positionMsgBox"); param != mCustomParameters.end()) {
    stringstream ss(param->second);
    int i = 0;
    while (ss.good()) {
      if (i > 4) {
        ILOG(Info, Support) << "Skipping next values provided for positionMsgBox" << ENDM;
        break;
      }
      string substr;
      getline(ss, substr, ',');
      mPositionMsgBox[i] = std::stod(substr);
      i++;
    }
    float minHeight = 0.09, minWidth = 0.19;
    if (mPositionMsgBox[2] - mPositionMsgBox[0] < minWidth || mPositionMsgBox[3] - mPositionMsgBox[1] < minHeight) {
      mPositionMsgBox = { 0.15, 0.75, 0.85, 0.9 };
      ILOG(Info, Support) << "MsgBox too small: returning to default" << ENDM;
    }
  }
}

Quality TriggersSwVsTcmCheck::check(std::map<std::string, std::shared_ptr<MonitorObject>>* moMap)
{
  Quality result = Quality::Null;
  int mNumErrors = 0;
  for (auto& [moName, mo] : *moMap) {
    (void)moName;
    if (mo->getName() == "TriggersSoftwareVsTCM") {
      auto* histogram = dynamic_cast<TH2*>(mo->getObject());

      if (!histogram) {
        ILOG(Error, Support) << "check(): MO TriggersSoftwareVsTCM not found" << ENDM;
        result.addReason(FlagReasonFactory::Unknown(), "MO TriggersSoftwareVsTCM not found");
        result.set(Quality::Null);
        return result;
      }

      result = Quality::Good;
      int numberOfBinsX = histogram->GetNbinsX();
      for (int binId = 1; binId <= numberOfBinsX; binId++) {
        if ((bool)(histogram->GetBinContent(binId, kBinSwOnly)) || (bool)(histogram->GetBinContent(binId, kBinTcmOnly))) {
          mNumErrors++;
          if (result.isBetterThan(Quality::Bad)) {
            result.set(Quality::Bad);
          }
          result.addReason(FlagReasonFactory::Unknown(), "Only SW or TCM trigger was activated");
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
  if (!mo) {
    ILOG(Error, Support) << "beautify(): MO NULL pointer" << ENDM;
    return;
  }

  if (mo->getName() == "TriggersSoftwareVsTCM") {
    auto* histogram = dynamic_cast<TH2*>(mo->getObject());

    if (!histogram) {
      ILOG(Error, Support) << "beautify(): MO TriggersSoftwareVsTCM not found" << ENDM;
      return;
    }

    TPaveText* msg = new TPaveText(mPositionMsgBox[0], mPositionMsgBox[1], mPositionMsgBox[2], mPositionMsgBox[3], "NDC");
    histogram->GetListOfFunctions()->Add(msg);
    msg->SetName(Form("%s_msg", mo->GetName()));
    msg->Clear();
    auto reasons = checkResult.getReasons();
    for (int i = 0; i < int(reasons.size()); i++) {
      if (i == 0) {
        msg->AddText(reasons[i].second.c_str());
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
