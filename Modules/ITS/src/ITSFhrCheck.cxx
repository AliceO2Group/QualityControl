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
      }
    } else if (mo->getName() == "General/General_Occupancy") {
      auto* h = dynamic_cast<TH2Poly*>(mo->getObject());
      result.addMetadata("Gen_Occu_IB", "good");
      result.addMetadata("Gen_Occu_OB", "good");
      result.addMetadata("Gen_Occu_empty", "good");
      result.set(Quality::Good);
      std::vector<int> skipbins = convertToIntArray(o2::quality_control_modules::common::getFromConfig<string>(mCustomParameters, "skipbins", ""));
      for (int ibin = 1; ibin <= h->GetNumberOfBins(); ibin++) {
        if (ibin <= 48) { // IB
          fhrcutIB = o2::quality_control_modules::common::getFromConfig<float>(mCustomParameters, "fhrcutIB", fhrcutIB);
          if (h->GetBinContent(ibin) > fhrcutIB) {
            if (std::find(skipbins.begin(), skipbins.end(), ibin) != skipbins.end()) {
              continue;
            }
            result.updateMetadata("Gen_Occu_IB", "bad");
            result.set(Quality::Bad);
          }
        } else { // OB
          fhrcutOB = o2::quality_control_modules::common::getFromConfig<float>(mCustomParameters, "fhrcutOB", fhrcutOB);
          if (h->GetBinContent(ibin) > fhrcutOB) {
            if (std::find(skipbins.begin(), skipbins.end(), ibin) != skipbins.end()) {
              continue;
            }
            result.updateMetadata("Gen_Occu_OB", "bad");
            result.set(Quality::Bad);
          }
        }

        if (h->GetBinContent(ibin) < 1e-15) { // in case of empty stave
          if (std::find(skipbins.begin(), skipbins.end(), ibin) != skipbins.end()) {
            continue;
          }
          result.updateMetadata("Gen_Occu_empty", "bad");
          result.set(Quality::Bad);
        }
      } // end loop on bins
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
  TLatex* text[5];
  TString status;
  int textColor;
  if (mo->getName() == "General/ErrorPlots") {
    auto* h = dynamic_cast<TH1D*>(mo->getObject());
    if (checkResult == Quality::Good) {
      text[0] = new TLatex(10, 0.6, "Quality::Good");
      text[1] = new TLatex(10, 0.4, "There is no Error found");
      for (int i = 0; i < 2; ++i) {
        text[i]->SetTextAlign(23);
        text[i]->SetTextSize(0.08);
        text[i]->SetTextColor(kGreen);
        h->GetListOfFunctions()->Add(text[i]);
      }
    } else if (checkResult == Quality::Bad) {
      text[0] = new TLatex(0.2, 0.7, "Quality::Bad");
      text[1] = new TLatex(0.2, 0.65, "Decoding error(s) detected");
      text[2] = new TLatex(0.2, 0.6, "Do not call, create a log entry");
      for (int i = 0; i < 3; ++i) {
        text[i]->SetTextFont(43);
        text[i]->SetTextSize(0.04);
        text[i]->SetTextColor(kRed);
        text[i]->SetNDC();
        h->GetListOfFunctions()->Add(text[i]);
      }
    }
  } else if (mo->getName() == "General/General_Occupancy") {
    auto* h = dynamic_cast<TH2Poly*>(mo->getObject());
    if (checkResult == Quality::Good) {
      status = "Quality GOOD";
      textColor = kGreen;
    } else if (checkResult == Quality::Bad) {
      status = "Quality Bad (call expert)";
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
      tInfo[2] = std::make_shared<TLatex>(0.4, 0.3, "#splitline{There are staves without hits}{IGNORE them if run has just started OR if it's TECH RUN}");
      tInfo[2]->SetTextColor(kRed);
      tInfo[2]->SetTextSize(0.03);
      tInfo[2]->SetTextFont(43);
      tInfo[2]->SetNDC();
      h->GetListOfFunctions()->Add(tInfo[2]->Clone());
    }

    tInfo[3] = std::make_shared<TLatex>(0.12, 0.835, Form("#bf{%s}", status.Data()));
    tInfo[3]->SetTextColor(textColor);
    tInfo[3]->SetTextSize(0.03);
    tInfo[3]->SetTextFont(43);
    tInfo[3]->SetNDC();
    h->GetListOfFunctions()->Add(tInfo[3]->Clone());

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
  }
}

std::vector<int> ITSFhrCheck::convertToIntArray(std::string input)
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
