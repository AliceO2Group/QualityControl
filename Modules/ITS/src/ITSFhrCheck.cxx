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
/// \author Zhen Zhang
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

namespace o2::quality_control_modules::its
{

void ITSFhrCheck::configure() {}

Quality ITSFhrCheck::check(std::map<std::string, std::shared_ptr<MonitorObject>>* moMap)
{
  Quality result = Quality::Null;

  for (auto& [moName, mo] : *moMap) {
    (void)moName;
    result = Quality::Good;
    if (mo->getName() == "General/General_Occupancy") {
      auto* h = dynamic_cast<TH2Poly*>(mo->getObject());
      result.addMetadata("Gen_Occu", "good");
      for (int ibin = 0; ibin < h->GetNumberOfBins(); ++ibin) {
        if (h->GetBinContent(ibin) == 0) {
          continue;
        }
        if (h->GetBinContent(ibin) > pow(10, -5)) {
          result.updateMetadata("Gen_Occu", "bad");
        } else if (h->GetBinContent(ibin) > pow(10, -6) && (strcmp(result.getMetadata("Gen_Occu").c_str(), "good") == 0)) {
          result.updateMetadata("Gen_Occu", "medium");
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
  TString status;
  int textColor;
  if (mo->getName() == "General/General_Occupancy") {
    auto* h = dynamic_cast<TH2Poly*>(mo->getObject());
    if (strcmp(checkResult.getMetadata("Gen_Occu").c_str(), "good") == 0) {
      status = "Quality::Good";
      textColor = kGreen;
    } else if (strcmp(checkResult.getMetadata("Gen_Occu").c_str(), "bad") == 0) {
      status = "Quality::BAD (call expert)";
      textColor = kRed;
    } else if (strcmp(checkResult.getMetadata("Gen_Occu").c_str(), "medium") == 0) {
      status = "Quality::Medium (Notify Expert)";
      textColor = kBlue;
    }
    tInfoOccu = std::make_shared<TLatex>(0.12, 0.835, Form("#bf{%s}", status.Data()));
    tInfoOccu->SetTextColor(textColor);
    tInfoOccu->SetTextSize(0.08);
    tInfoOccu->SetTextFont(23);
    tInfoOccu->SetNDC();
    h->GetListOfFunctions()->Add(tInfoOccu->Clone());

  } else if (mo->getName() == "General/Noisy_Pixel") {
    auto* h = dynamic_cast<TH2Poly*>(mo->getObject());
    if (strcmp(checkResult.getMetadata("Noi_Pix").c_str(), "good") == 0) {
      status = "Quality::Good";
      textColor = kGreen;
    } else if (strcmp(checkResult.getMetadata("Noi_Pix").c_str(), "bad") == 0) {
      status = "Quality::BAD (call expert)";
      textColor = kRed;
    } else if (strcmp(checkResult.getMetadata("Noi_Pix").c_str(), "medium") == 0) {
      status = "Quality::Medium (Notify Expert)";
      textColor = kBlue;
    }
    tInfoNoisy = std::make_shared<TLatex>(0.12, 0.835, Form("#bf{%s}", status.Data()));
    tInfoNoisy->SetTextColor(textColor);
    tInfoNoisy->SetTextSize(0.08);
    tInfoNoisy->SetTextFont(23);
    tInfoNoisy->SetNDC();
    h->GetListOfFunctions()->Add(tInfoNoisy->Clone());
  }
  TString objectName = mo->getName();
  if (objectName.Contains("ChipStave")) {
    TString layerString = TString(objectName(15, 1).Data());
    int layer = layerString.Atoi();
    auto* h = dynamic_cast<TH2D*>(mo->getObject());
    if (strcmp(checkResult.getMetadata(Form("Layer%d", layer)).c_str(), "good") == 0) {
      status = "Quality::Good";
      textColor = kGreen;
    } else if (strcmp(checkResult.getMetadata(Form("Layer%d", layer)).c_str(), "medium") == 0) {
      status = "Quality::Medium (Notify Expert)";
      textColor = kBlue;
    } else if (strcmp(checkResult.getMetadata(Form("Layer%d", layer)).c_str(), "bad") == 0) {
      status = "Quality::BAD (call expert)";
      textColor = kRed;
    }
    tInfoChip = std::make_shared<TLatex>(0.12, 0.835, Form("#bf{%s}", status.Data()));
    tInfoChip->SetTextColor(textColor);
    tInfoChip->SetTextSize(0.08);
    tInfoChip->SetTextFont(23);
    tInfoChip->SetNDC();
    h->GetListOfFunctions()->Add(tInfoChip->Clone());
  }
}

} // namespace o2::quality_control_modules::its
