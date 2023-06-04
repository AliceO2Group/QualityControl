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
/// \file   ITSFhrCheck.cxx
/// \author Liang Zhang
/// \author Jian Liu
///

#include "ITS/ITSFhrCheck.h"
#include "QualityControl/MonitorObject.h"
#include "QualityControl/Quality.h"

#include <fairlogger/Logger.h>
#include <TH1.h>
#include <TList.h>
#include <TPaveText.h>
#include <TLatex.h>
#include <TH2Poly.h>
#include <TString.h>
#include "Common/Utils.h"
#include "TMath.h"

namespace o2::quality_control_modules::its
{

Quality ITSFhrCheck::check(std::map<std::string, std::shared_ptr<MonitorObject>>* moMap)
{
  Quality result = Quality::Null;

  for (auto& [moName, mo] : *moMap) {
    (void)moName;
    if (mo->getName() == "General/ErrorPlots") {
      result = Quality::Good;
      auto* h = dynamic_cast<TH1D*>(mo->getObject());
      if (h->GetMaximum() > 0) {
        result.set(Quality::Bad);
        result.addReason(o2::quality_control::FlagReasonFactory::Unknown(), "BAD:Decoding error(s) detected;");
      }
    } else if (mo->getName() == "General/General_Occupancy") {
      auto* h = dynamic_cast<TH2Poly*>(mo->getObject());
      result.addMetadata("Gen_Occu_IB", "good");
      result.addMetadata("Gen_Occu_OB", "good");
      result.addMetadata("Gen_Occu_empty", "good");
      result.set(Quality::Good);
      std::vector<int> skipbins = convertToArray<int>(o2::quality_control_modules::common::getFromConfig<string>(mCustomParameters, "skipbins", ""));

      TIter next(h->GetBins());
      int ibin = 1;
      double_t nBadStaves[4];
      TString sErrorReason = "";
      while (TH2PolyBin* Bin = (TH2PolyBin*)next()) {
        if (std::find(skipbins.begin(), skipbins.end(), ibin) != skipbins.end()) {
          ibin++;
          continue;
        }

        double X_center = (Bin->GetXMax() + Bin->GetXMin()) / 2;
        double Y_center = (Bin->GetYMax() + Bin->GetYMin()) / 2;
        double phi = TMath::ATan2(Y_center, X_center);
        int sector = 2 * (phi + TMath::Pi()) / TMath::Pi();

        if (ibin <= 48) { // IB
          fhrcutIB = o2::quality_control_modules::common::getFromConfig<float>(mCustomParameters, "fhrcutIB", fhrcutIB);
          if (h->GetBinContent(ibin) > fhrcutIB) {
            result.updateMetadata("Gen_Occu_IB", "bad");
            result.set(Quality::Bad);
            TString text = "Bad: IB stave has high FHR;";
            if (!checkReason(result, text))
              result.addReason(o2::quality_control::FlagReasonFactory::Unknown(), text.Data());
          }
        } else { // OB
          fhrcutOB = o2::quality_control_modules::common::getFromConfig<float>(mCustomParameters, "fhrcutOB", fhrcutOB);
          if (h->GetBinContent(ibin) > fhrcutOB) {
            result.updateMetadata("Gen_Occu_OB", "bad");
            result.set(Quality::Bad);
            TString text = "Bad: OB stave has high FHR;";
            if (!checkReason(result, text))
              result.addReason(o2::quality_control::FlagReasonFactory::Unknown(), text.Data());
          }
        }

        if (Bin->GetContent() < 1e-15) { // in case of empty stave
          nBadStaves[sector]++;
        }
        ibin++;
      }

      for (int iSector = 0; iSector < 4; iSector++) {
        if (nBadStaves[iSector] / 48 > 0.1) {
          result.updateMetadata("Gen_Occu_empty", "bad");
          result.set(Quality::Bad);
          result.addReason(o2::quality_control::FlagReasonFactory::Unknown(), "Bad: Many empty staves in sector; IGNORE if run has just started OR it's TECH RUN");
          break;
        } else if (nBadStaves[iSector] / 48 > 0.05) {
          result.updateMetadata("Gen_Occu_empty", "medium");
          result.set(Quality::Medium);
          result.addReason(o2::quality_control::FlagReasonFactory::Unknown(), "Medium: Many empty staves in sector; IGNORE if run has just started OR it's TECH RUN");
        }
      }

    } else if (mo->getName() == "General/Noisy_Pixel") {
      auto* h = dynamic_cast<TH2Poly*>(mo->getObject());
      result.addMetadata("Noi_Pix", "good");
      for (int ibin = 0; ibin < h->GetNumberOfBins(); ++ibin) {
        if (h->GetBinContent(ibin) == 0) {
          continue;
        }
        if (ibin < 48) {
          if ((h->GetBinContent(ibin) / (double)mNPixelPerStave[0]) > 0.0001) {
            result.updateMetadata("Noi_Pix", "bad");
          } else if ((h->GetBinContent(ibin) / (double)mNPixelPerStave[0]) > 0.00005 && strcmp(result.getMetadata("Noi_Pix").c_str(), "good") == 0) {
            result.updateMetadata("Noi_Pix", "medium");
          }
        } else if (ibin < 102) {
          if ((h->GetBinContent(ibin) / (double)mNPixelPerStave[1]) > 0.0001) {
            result.updateMetadata("Noi_Pix", "bad");
          } else if ((h->GetBinContent(ibin) / (double)mNPixelPerStave[1]) > 0.00005 && strcmp(result.getMetadata("Noi_Pix").c_str(), "good") == 0) {
            result.updateMetadata("Noi_Pix", "medium");
          }
        } else {
          if ((h->GetBinContent(ibin) / (double)mNPixelPerStave[2]) > 0.0001) {
            result.updateMetadata("Noi_Pix", "bad");
          } else if ((h->GetBinContent(ibin) / (double)mNPixelPerStave[2]) > 0.00005 && strcmp(result.getMetadata("Noi_Pix").c_str(), "good") == 0) {
            result.updateMetadata("Noi_Pix", "medium");
          }
        }
      }
    }
    TString objectName = mo->getName();
    if (objectName.Contains("ChipStave")) {
      auto* h = dynamic_cast<TH2D*>(mo->getObject());
      TString layerString = TString(objectName(15, 1).Data());
      int layer = layerString.Atoi();
      result.addMetadata(Form("Layer%d", layer), "good");
      double maxOccupancy = h->GetMaximum();
      if (maxOccupancy > pow(10, -5)) {
        result.updateMetadata(Form("Layer%d", layer), "bad");
      } else if (maxOccupancy > pow(10, -6)) {
        result.updateMetadata(Form("Layer%d", layer), "medium");
      }
    }
  }
  return result;
}

std::string ITSFhrCheck::getAcceptedType() { return "TH1"; }

void ITSFhrCheck::beautify(std::shared_ptr<MonitorObject> mo, Quality checkResult)
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

  TLatex* text[5];
  TString status;
  int textColor;
  if (mo->getName() == "General/ErrorPlots") {
    auto* h = dynamic_cast<TH1D*>(mo->getObject());
    if (checkResult == Quality::Good) {
      text[0] = new TLatex(0.05, 0.95, "#bf{Quality::Good}");
      text[1] = new TLatex(0.2, 0.4, "There is no Error found");
      for (int i = 0; i < 2; ++i) {
        text[i]->SetTextFont(43);
        text[i]->SetTextSize(0.06);
        text[i]->SetTextColor(kGreen);
        text[i]->SetNDC();
        h->GetListOfFunctions()->Add(text[i]);
      }
    } else if (checkResult == Quality::Bad) {
      text[0] = new TLatex(0.05, 0.95, "#bf{Quality::Bad}");
      text[1] = new TLatex(0.2, 0.65, "Decoding error(s) detected");
      text[2] = new TLatex(0.2, 0.6, "Do not call, create a log entry");
      for (int i = 0; i < 3; ++i) {
        text[i]->SetTextFont(43);
        text[i]->SetTextSize(0.06);
        text[i]->SetTextColor(kRed);
        text[i]->SetNDC();
        h->GetListOfFunctions()->Add(text[i]);
      }
    }
    if (ShifterInfoText[mo->getName()] != "")
      h->GetListOfFunctions()->Add(tShifterInfo->Clone());

  } else if (mo->getName() == "General/General_Occupancy") {
    auto* h = dynamic_cast<TH2Poly*>(mo->getObject());
    if (checkResult == Quality::Good) {
      status = "Quality GOOD";
      textColor = kGreen;
    } else if (checkResult == Quality::Bad) {
      status = "Quality::Bad (call expert)";
      textColor = kRed;
    }

    if (strcmp(checkResult.getMetadata("Gen_Occu_IB").c_str(), "bad") == 0) {
      fhrcutIB = o2::quality_control_modules::common::getFromConfig<float>(mCustomParameters, "fhrcutIB", fhrcutIB);
      tInfo[0] = std::make_shared<TLatex>(0.4, 0.7, Form("IB has stave(s) with FHR > %.2e hits/ev/pix", fhrcutIB));
      tInfo[0]->SetTextColor(kRed);
      tInfo[0]->SetTextSize(0.03);
      tInfo[0]->SetTextFont(43);
      tInfo[0]->SetNDC();
      h->GetListOfFunctions()->Add(tInfo[0]->Clone());
    }
    if (strcmp(checkResult.getMetadata("Gen_Occu_OB").c_str(), "bad") == 0) {
      fhrcutOB = o2::quality_control_modules::common::getFromConfig<float>(mCustomParameters, "fhrcutOB", fhrcutOB);
      tInfo[1] = std::make_shared<TLatex>(0.4, 0.65, Form("OB has stave(s) with FHR > %.2e hits/ev/pix", fhrcutOB));
      tInfo[1]->SetTextColor(kRed);
      tInfo[1]->SetTextSize(0.03);
      tInfo[1]->SetTextFont(43);
      tInfo[1]->SetNDC();
      h->GetListOfFunctions()->Add(tInfo[1]->Clone());
    }
    if (strcmp(checkResult.getMetadata("Gen_Occu_empty").c_str(), "bad") == 0) {
      tInfo[2] = std::make_shared<TLatex>(0.4, 0.3, "#splitline{ITS sector has more than 10% of empty staves}{IGNORE them if run has just started OR if it's TECH RUN}");
      tInfo[2]->SetTextColor(kRed);
      tInfo[2]->SetTextSize(0.03);
      tInfo[2]->SetTextFont(43);
      tInfo[2]->SetNDC();
      h->GetListOfFunctions()->Add(tInfo[2]->Clone());
    } else if (strcmp(checkResult.getMetadata("Gen_Occu_empty").c_str(), "medium") == 0) {
      tInfo[2] = std::make_shared<TLatex>(0.4, 0.3, "#splitline{ITS sector has more than 5% of empty staves}{IGNORE them if run has just started OR if it's TECH RUN}");
      tInfo[2]->SetTextColor(kOrange);
      tInfo[2]->SetTextSize(0.03);
      tInfo[2]->SetTextFont(43);
      tInfo[2]->SetNDC();
      h->GetListOfFunctions()->Add(tInfo[2]->Clone());
    }

    tInfo[3] = std::make_shared<TLatex>(0.05, 0.95, Form("#bf{%s}", status.Data()));
    tInfo[3]->SetTextColor(textColor);
    tInfo[3]->SetTextSize(0.06);
    tInfo[3]->SetTextFont(43);
    tInfo[3]->SetNDC();
    h->GetListOfFunctions()->Add(tInfo[3]->Clone());

    if (ShifterInfoText[mo->getName()] != "")
      h->GetListOfFunctions()->Add(tShifterInfo->Clone());

  } else if (mo->getName() == "General/Noisy_Pixel") {
    auto* h = dynamic_cast<TH2Poly*>(mo->getObject());
    if (strcmp(checkResult.getMetadata("Noi_Pix").c_str(), "good") == 0) {
      text[0] = new TLatex(0, 0, "Quality::Good");
      text[0]->SetTextAlign(23);
      text[0]->SetTextSize(0.08);
      text[0]->SetTextColor(kGreen);
      h->GetListOfFunctions()->Add(text[0]);
    } else if (strcmp(checkResult.getMetadata("Noi_Pix").c_str(), "bad") == 0) {
      text[0] = new TLatex(0, 100, "Quality::Bad");
      text[1] = new TLatex(0, 0, "Noisy Pixel over 0.01%");
      text[2] = new TLatex(0, -100, "Please Call Expert");
      for (int i = 0; i < 3; ++i) {
        text[i]->SetTextAlign(23);
        text[i]->SetTextSize(0.08);
        text[i]->SetTextColor(kRed);
        h->GetListOfFunctions()->Add(text[i]);
      }
    } else if (strcmp(checkResult.getMetadata("Noi_Pix").c_str(), "medium") == 0) {
      text[0] = new TLatex(0, 100, "Quality::Medium");
      text[1] = new TLatex(0, 0, "Noisy Pixel over 0.005%");
      text[2] = new TLatex(0, -100, "Please Notify Expert On MM");
      for (int i = 0; i < 3; ++i) {
        text[i]->SetTextAlign(23);
        text[i]->SetTextSize(0.08);
        text[i]->SetTextColor(46);
        h->GetListOfFunctions()->Add(text[i]);
      }
    }
    if (ShifterInfoText[mo->getName()] != "")
      h->GetListOfFunctions()->Add(tShifterInfo->Clone());
  }
  TString objectName = mo->getName();
  if (objectName.Contains("ChipStave")) {
    TString layerString = TString(objectName(15, 1).Data());
    int layer = layerString.Atoi();
    auto* h = dynamic_cast<TH2D*>(mo->getObject());
    if (strcmp(checkResult.getMetadata(Form("Layer%d", layer)).c_str(), "good") == 0) {
      text[0] = new TLatex(0.5, 0.6, "Quality::Good");
      text[0]->SetNDC();
      text[0]->SetTextAlign(23);
      text[0]->SetTextSize(0.08);
      text[0]->SetTextColor(kGreen);
      h->GetListOfFunctions()->Add(text[0]);
    } else if (strcmp(checkResult.getMetadata(Form("Layer%d", layer)).c_str(), "medium") == 0) {
      text[0] = new TLatex(0.5, 0.73, "Quality::Medium");
      text[0]->SetNDC();
      text[1] = new TLatex(0.5, 0.6, "Max Chip Occupancy Over 10^{-6}");
      text[1]->SetNDC();
      text[2] = new TLatex(0.5, 0.47, "Please Notify Expert On MM");
      text[2]->SetNDC();
      for (int i = 0; i < 3; ++i) {
        text[i]->SetTextAlign(23);
        text[i]->SetTextSize(0.08);
        text[i]->SetTextColor(46);
        h->GetListOfFunctions()->Add(text[i]);
      }
    } else if (strcmp(checkResult.getMetadata(Form("Layer%d", layer)).c_str(), "bad") == 0) {
      text[0] = new TLatex(0.5, 0.73, "Quality::Bad");
      text[0]->SetNDC();
      text[1] = new TLatex(0.5, 0.6, "Max Chip Occupancy Over 10^{-5}");
      text[1]->SetNDC();
      text[2] = new TLatex(0.5, 0.47, "Please Call Expert");
      text[2]->SetNDC();
      for (int i = 0; i < 3; ++i) {
        text[i]->SetTextAlign(23);
        text[i]->SetTextSize(0.08);
        text[i]->SetTextColor(kRed);
        h->GetListOfFunctions()->Add(text[i]);
      }
    }
    if (ShifterInfoText[mo->getName()] != "")
      h->GetListOfFunctions()->Add(tShifterInfo->Clone());
  }
}
bool ITSFhrCheck::checkReason(Quality checkResult, TString text)
{
  auto reasons = checkResult.getReasons();
  for (int i = 0; i < int(reasons.size()); i++) {
    if (text.Contains(reasons[i].second.c_str()))
      return true;
  }
  return false;
}

} // namespace o2::quality_control_modules::its
