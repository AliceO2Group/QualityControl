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

<<<<<<< HEAD:Modules/FIT/FV0/src/TriggersSwVsTcmCheck.cxx
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
=======
constexpr int kBinSwOnly = 0;
constexpr int kBinTcmOnly = 1;

void TriggersSwVsTcm::configure() {
    if (auto param = mCustomParameters.find("ccdbUrl"); param != mCustomParameters.end()) {
      setCcdbUrl(param->second);
      ILOG(Debug, Support) << "configure() : using deadChannelMap from CCDB, configured url = " << param->second << ENDM;
    } else {
      setCcdbUrl("o2-ccdb.internal");
      ILOG(Debug, Support) << "configure() : using deadChannelMap from CCDB, default url = "
                           << "o2-ccdb.internal" << ENDM;
    }
}

Quality TriggersSwVsTcm::check(std::map<std::string, std::shared_ptr<MonitorObject>>* moMap)
{
  Quality result = Quality::Null;
  result.set(Quality::Medium);
  result.addReason(FlagReasonFactory::Unknown(),
            "Test Warning"); 
>>>>>>> c8a05a9e (Upload changes during work on TriggersSwVsTcmCheck):Modules/FV0/src/TriggersSwVsTcmCheck.cxx
  int mNumErrors = 0;
  for (auto& [moName, mo] : *moMap) {
    (void)moName;
    if (mo->getName() == "TriggersSoftwareVsTCM") {
      auto* histogram = dynamic_cast<TH2F*>(mo->getObject());
<<<<<<< HEAD:Modules/FIT/FV0/src/TriggersSwVsTcmCheck.cxx
      result = Quality::Good;
      int numberOfBinsX = histogram->GetNbinsX();
      for (int binId = 1; binId <= numberOfBinsX; binId++) {
        if ((bool)(histogram->GetBinContent(binId, kBinSwOnly)) || (bool)(histogram->GetBinContent(binId, kBinTcmOnly))) {
          mNumErrors++;
          if (result.isBetterThan(Quality::Bad)) {
              result.set(Quality::Bad);
          }
          if ((bool)(histogram->GetBinContent(binId, kBinSwOnly))) {
            result.addReason(FlagReasonFactory::Unknown(),
                    Form("TriggersSoftwareVsTCM. < \"Error\" Only SW trigger was activated for bin nr %d", binId));
          } else {
            result.addReason(FlagReasonFactory::Unknown(),
                    Form("TriggersSoftwareVsTCM. < \"Error\" Only TCM trigger was activated for bin nr %d", binId));
          }
        }
=======

      result = Quality::Good;

      int numXBins = histogram->GetNbinsX();
      int numYBins = histogram->GetNbinsY();

      for (int i = 0; i < numXBins; i++) {
          if ((bool)histogram->GetBinContent(i, kBinSwOnly) == (bool)histogram->GetBinContent(i, kBinTcmOnly)) {
            mNumErrors++;
            if (result.isBetterThan(Quality::Bad)) {
                result.set(Quality::Bad);
                result.addReason(FlagReasonFactory::Unknown(),
                        "TriggersSoftwareVsTCM. < \"Error\" threshold in channel " + std::to_string(i));                
            }   
          }
>>>>>>> c8a05a9e (Upload changes during work on TriggersSwVsTcmCheck):Modules/FV0/src/TriggersSwVsTcmCheck.cxx
      }
    }
  }
  result.addMetadata("nErrors", std::to_string(mNumErrors));
<<<<<<< HEAD:Modules/FIT/FV0/src/TriggersSwVsTcmCheck.cxx
  return result;
}

std::string TriggersSwVsTcmCheck::getAcceptedType() { return "TH2"; }

void TriggersSwVsTcmCheck::beautify(std::shared_ptr<MonitorObject> mo, Quality checkResult)
=======
  result.addMetadata("nWarnings", std::to_string(0));

  return result;
}

std::string TriggersSwVsTcm::getAcceptedType() { return "TH2"; }

void TriggersSwVsTcm::beautify(std::shared_ptr<MonitorObject> mo, Quality checkResult)
>>>>>>> c8a05a9e (Upload changes during work on TriggersSwVsTcmCheck):Modules/FV0/src/TriggersSwVsTcmCheck.cxx
{
  if (mo->getName() == "TriggersSoftwareVsTCM") {
    auto* histogram = dynamic_cast<TH2F*>(mo->getObject());

<<<<<<< HEAD:Modules/FIT/FV0/src/TriggersSwVsTcmCheck.cxx
    TPaveText* msg = new TPaveText(mPositionMsgBox[0], mPositionMsgBox[1], mPositionMsgBox[2], mPositionMsgBox[3], "NDC");
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
=======
    TPaveText* msg = new TPaveText(0.15, 0.2, 0.85, 0.45, "NDC");
    histogram->GetListOfFunctions()->Add(msg);
    msg->SetName(Form("%s_msg", mo->GetName()));
    msg->Clear();

    if (checkResult == Quality::Good) {
      msg->AddText(">> Quality::Good <<");
      msg->SetFillColor(kGreen);
    } else if (checkResult == Quality::Bad) {
      auto reasons = checkResult.getReasons();
      msg->SetFillColor(kRed);
      msg->AddText(">> Quality::Bad <<");
    } else if (checkResult == Quality::Medium) {
      auto reasons = checkResult.getReasons();
      msg->SetFillColor(kOrange);
      msg->AddText(">> Quality::Medium <<");
    } else {
      msg->AddText(">> Quality::Null <<");
      msg->SetFillColor(kGray);
    }
  }
}

} // namespace o2::quality_control_modules::fv0
>>>>>>> c8a05a9e (Upload changes during work on TriggersSwVsTcmCheck):Modules/FV0/src/TriggersSwVsTcmCheck.cxx
