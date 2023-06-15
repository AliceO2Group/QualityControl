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
#include <string.h>
#include <TLatex.h>
#include <TLine.h>
#include <iostream>
#include "Common/Utils.h"

namespace o2::quality_control_modules::its
{

Quality ITSClusterCheck::check(std::map<std::string, std::shared_ptr<MonitorObject>>* moMap)
{
  Quality result = Quality::Null;
  double averageClusterSizeLimit[NLayer] = { 5, 5, 5, 5, 5, 5, 5 };

  std::map<std::string, std::shared_ptr<MonitorObject>>::iterator iter;
  for (iter = moMap->begin(); iter != moMap->end(); ++iter) {
    result = Quality::Good;

    if (iter->second->getName().find("AverageClusterSize") != std::string::npos) {
      auto* h = dynamic_cast<TH2F*>(iter->second->getObject());
      for (int ilayer = 0; ilayer < NLayer; ilayer++) {
        result.addMetadata(Form("Layer%d", ilayer), "good");
        if (iter->second->getName().find(Form("Layer%d", ilayer)) != std::string::npos && h->GetMaximum() > averageClusterSizeLimit[ilayer]) {
          result.updateMetadata(Form("Layer%d", ilayer), "medium");
          result.set(Quality::Medium);
        }
      }
    }

    if (iter->second->getName().find("EmptyLaneFractionGlobal") != std::string::npos) {
      auto* h = dynamic_cast<TH1D*>(iter->second->getObject());
      result.addMetadata("EmptyLaneFractionGlobal", "good");
      MaxEmptyLaneFraction = o2::quality_control_modules::common::getFromConfig<float>(mCustomParameters, "MaxEmptyLaneFraction", MaxEmptyLaneFraction);
      if (h->GetBinContent(1) + h->GetBinContent(2) + h->GetBinContent(3) > MaxEmptyLaneFraction) {
        result.updateMetadata("EmptyLaneFractionGlobal", "bad");
        result.set(Quality::Bad);
        result.addReason(o2::quality_control::FlagReasonFactory::Unknown(), Form("BAD:>%.0f %% of the lanes are empty", (h->GetBinContent(1) + h->GetBinContent(2) + h->GetBinContent(3)) * 100));
      }
    } // end summary loop

    if (iter->second->getName().find("General_Occupancy") != std::string::npos) {
      auto* hp = dynamic_cast<TH2F*>(iter->second->getObject());
      std::vector<int> skipxbins = convertToArray<int>(o2::quality_control_modules::common::getFromConfig<string>(mCustomParameters, "skipxbinsoccupancy", ""));
      std::vector<int> skipybins = convertToArray<int>(o2::quality_control_modules::common::getFromConfig<string>(mCustomParameters, "skipybinsoccupancy", ""));
      std::vector<std::pair<int, int>> xypairs;
      for (int i = 0; i < (int)skipxbins.size(); i++) {
        xypairs.push_back(std::make_pair(skipxbins[i], skipybins[i]));
      }
      result.set(Quality::Good);
      for (int iy = 1; iy <= hp->GetNbinsY(); iy++) {
        int ilayer = iy <= hp->GetNbinsY() / 2 ? hp->GetNbinsY() / 2 - iy : iy - hp->GetNbinsY() / 2 - 1;
        std::string tb = iy <= hp->GetNbinsY() / 2 ? "B" : "T";
        result.addMetadata(Form("Layer%d%s", ilayer, tb.c_str()), "good");
        bool mediumHalfLayer = false;
        for (int ix = 1; ix <= hp->GetNbinsX(); ix++) { // loop on staves
          if (std::find(xypairs.begin(), xypairs.end(), std::make_pair(ix, iy)) != xypairs.end()) {
            continue;
          }

          maxcluocc[ilayer] = o2::quality_control_modules::common::getFromConfig<int>(mCustomParameters, Form("maxcluoccL%d", ilayer), maxcluocc[ilayer]);
          if (hp->GetBinContent(ix, iy) > maxcluocc[ilayer]) {
            result.set(Quality::Medium);
            result.updateMetadata(Form("Layer%d%s", ilayer, tb.c_str()), "medium");
            mediumHalfLayer = true;
          }
        }
        if (mediumHalfLayer)
          result.addReason(o2::quality_control::FlagReasonFactory::Unknown(), Form("Medium: Layer%d%s has high cluster occupancy;", ilayer, tb.c_str()));

        // check for empty bins (empty staves)
        result.addMetadata(Form("Layer%d%s_empty", ilayer, tb.c_str()), "good");
        bool badHalfLayer = false;
        for (int ix = 12; ix > 12 - mNStaves[ilayer] / 4; ix--) {
          if (std::find(xypairs.begin(), xypairs.end(), std::make_pair(ix, iy)) != xypairs.end()) {
            continue;
          }
          if (hp->GetBinContent(ix, iy) < 1e-15) {
            result.updateMetadata(Form("Layer%d%s_empty", ilayer, tb.c_str()), "bad");
            result.set(Quality::Bad);
            badHalfLayer = true;
            ILOG(Debug, Devel) << "************************ " << Form("Layer%d%s_empty", ilayer, tb.c_str()) << ENDM;
          }
        }
        int stop = (iy == 2 || iy == 13) ? 13 + mNStaves[ilayer] / 4 + 1 : 13 + mNStaves[ilayer] / 4; // for L5
        for (int ix = 13; ix < stop; ix++) {
          if (std::find(xypairs.begin(), xypairs.end(), std::make_pair(ix, iy)) != xypairs.end()) {
            continue;
          }
          if (hp->GetBinContent(ix, iy) < 1e-15) {
            result.updateMetadata(Form("Layer%d%s_empty", ilayer, tb.c_str()), "bad");
            result.set(Quality::Bad);
            badHalfLayer = true;
          }
        }
        if (badHalfLayer)
          result.addReason(o2::quality_control::FlagReasonFactory::Unknown(), Form("BAD: Layer%d%s has empty stave;", ilayer, tb.c_str()));
      }
    } // end GeneralOccupancy
  }
  return result;
} // end check

std::string ITSClusterCheck::getAcceptedType() { return "TH2F"; }

void ITSClusterCheck::beautify(std::shared_ptr<MonitorObject> mo, Quality checkResult)
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
  Double_t positionX, positionY;

  if (mo->getName().find("AverageClusterSize") != std::string::npos) {
    auto* h = dynamic_cast<TH2F*>(mo->getObject());
    std::string histoName = mo->getName();
    int iLayer = histoName[histoName.find("Layer") + 5] - 48; // Searching for position of "Layer" in the name of the file, then +5 is the NUMBER of the layer, -48 is conversion to int

    if (checkResult == Quality::Medium) {
      status = "INFO: large clusters - do not call expert";
      textColor = kYellow;
      positionX = 0.15;
      positionY = 0.8;
    } else {
      status = "Quality::GOOD";
      textColor = kGreen;
      positionX = 0.02;
      positionY = 0.91;
    }

    msg = std::make_shared<TLatex>(positionX, positionY, Form("#bf{%s}", status.Data()));
    msg->SetTextColor(textColor);
    msg->SetTextSize(0.06);
    msg->SetTextFont(43);
    msg->SetNDC();
    h->GetListOfFunctions()->Add(msg->Clone());
    if (ShifterInfoText[mo->getName()] != "")
      h->GetListOfFunctions()->Add(tShifterInfo->Clone());
  }

  if (mo->getName().find("EmptyLaneFractionGlobal") != std::string::npos) {
    auto* h = dynamic_cast<TH1D*>(mo->getObject());
    if (checkResult == Quality::Good) {
      status = "Quality::GOOD";
      textColor = kGreen;
      positionX = 0.05;
      positionY = 0.91;
    } else if (checkResult == Quality::Bad) {
      status = "Quality::BAD (call expert)";
      textColor = kRed;
      if (strcmp(checkResult.getMetadata("EmptyLaneFractionGlobal").c_str(), "bad") == 0) {
        MaxEmptyLaneFraction = o2::quality_control_modules::common::getFromConfig<float>(mCustomParameters, "MaxEmptyLaneFraction", MaxEmptyLaneFraction);
        tInfoSummary = std::make_shared<TLatex>(0.12, 0.5, Form(">%.0f %% of the lanes are empty", MaxEmptyLaneFraction * 100));
        tInfoSummary->SetTextColor(kRed);
        tInfoSummary->SetTextSize(0.05);
        tInfoSummary->SetTextFont(43);
        tInfoSummary->SetNDC();
        h->GetListOfFunctions()->Add(tInfoSummary->Clone());
      }
    }
    tInfo = std::make_shared<TLatex>(0.1, 0.11, Form("#bf{%s}", "Threshold value"));
    tInfo->SetTextColor(kRed);
    tInfo->SetTextSize(0.05);
    tInfo->SetTextFont(43);
    h->GetListOfFunctions()->Add(tInfo->Clone());
    tInfoLine = std::make_shared<TLine>(0, 0.1, 4, 0.1);
    tInfoLine->SetLineColor(kRed);
    tInfoLine->SetLineStyle(9);
    h->GetListOfFunctions()->Add(tInfoLine->Clone());
    msg = std::make_shared<TLatex>(positionX, positionY, Form("#bf{%s}", status.Data()));
    msg->SetTextColor(textColor);
    msg->SetTextSize(0.06);
    msg->SetTextFont(43);
    msg->SetNDC();
    h->GetListOfFunctions()->Add(msg->Clone());
    if (ShifterInfoText[mo->getName()] != "")
      h->GetListOfFunctions()->Add(tShifterInfo->Clone());
  }
  if (mo->getName().find("General_Occupancy") != std::string::npos) {
    auto* h = dynamic_cast<TH2F*>(mo->getObject());
    if (checkResult == Quality::Good) {
      status = "Quality::GOOD";
      textColor = kGreen;
      positionX = 0.05;
      positionY = 0.95;
    } else {
      for (int il = 0; il < 14; il++) {
        std::string tb = il < 7 ? "T" : "B";
        if (strcmp(checkResult.getMetadata(Form("Layer%d%s", il % 7, tb.c_str())).c_str(), "medium") == 0) {
          text[il] = std::make_shared<TLatex>(0.3, 0.1 + il * 0.05, Form("L%d%s has large cluster occupancy", il % 7, tb.c_str()));
          text[il]->SetTextFont(43);
          text[il]->SetTextSize(0.03);
          text[il]->SetTextColor(kOrange);
          text[il]->SetNDC();
          h->GetListOfFunctions()->Add(text[il]->Clone());
          status = "Quality::Medium, create log entry";
          textColor = kOrange;
          positionX = 0.05;
          positionY = 0.95;
        }

        if (strcmp(checkResult.getMetadata(Form("Layer%d%s_empty", il % 7, tb.c_str())).c_str(), "bad") == 0) {
          text2[il] = std::make_shared<TLatex>(0.7, 0.1 + il * 0.05, Form("L%d%s has empty staves", il % 7, tb.c_str()));
          text2[il]->SetTextFont(43);
          text2[il]->SetTextSize(0.03);
          text2[il]->SetTextColor(kRed);
          text2[il]->SetNDC();
          h->GetListOfFunctions()->Add(text2[il]->Clone());
          status = "Quality::Bad, call expert";
          textColor = kRed;
          positionX = 0.05;
          positionY = 0.95;
        }
      }
    }
    msg = std::make_shared<TLatex>(positionX, positionY, Form("#bf{%s}", status.Data()));
    msg->SetTextColor(textColor);
    msg->SetTextSize(0.06);
    msg->SetTextFont(43);
    msg->SetNDC();
    h->GetListOfFunctions()->Add(msg->Clone());
    if (ShifterInfoText[mo->getName()] != "")
      h->GetListOfFunctions()->Add(tShifterInfo->Clone());
  }
}

} // namespace o2::quality_control_modules::its
