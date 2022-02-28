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

namespace o2::quality_control_modules::its
{

void ITSFhrCheck::configure() {}

Quality ITSFhrCheck::check(std::map<std::string, std::shared_ptr<MonitorObject>>* moMap)
{
  Quality result = 0;                                                    // The FHR Checker will check three plots:
  std::map<std::string, std::shared_ptr<MonitorObject>>::iterator iter;  // General/ErrorPlots, General/General_Occupancy and
  for (iter = moMap->begin(); iter != moMap->end(); ++iter) {            // General/Noisy_Pixel. and store the result in a
    if (iter->second->getName() == "General/ErrorPlots") {               // check the General/ErrorPlots		//three digits: XXXXXXXXXX
      auto* h = dynamic_cast<TH1D*>(iter->second->getObject());          //		                                                ||||||||||-> ErrorPlots
      if (h->GetMaximum() > 0) {                                         //		                                                |||||||||-> General_Occupancy
        result = result.getLevel() + 1;                                  //		                                                ||||||||-> Noisy_Pixel
      }                                                                  // the number for each digits is correspond:		        |||||||->Occupancy Layer0
    } else if (iter->second->getName() == "General/General_Occupancy") { // 0: Good, 1: Bad, 2: Medium(if exist)			||||||->Occupancy Layer1
      auto* h = dynamic_cast<TH2Poly*>(iter->second->getObject());       // So if the quality level is 210, that means		        |||||->Occupancy Layer2
      for (int ibin = 0; ibin < h->GetNumberOfBins(); ++ibin) {          // Medium Noisy_Pixel, Bad General_Occupancy and	        ||||->Occupancy Layer3
        if (h->GetBinContent(ibin) == 0) {                               // Good ErrorPlots					        |||->Occupancy Layer4
          continue;                                                      //							        ||->Occupancy Layer5
        }                                                                //							        |->Occupancy Layer6
        if (ibin < 48) {
          if (h->GetBinContent(ibin) > pow(10, -5)) {
            result = (result.getLevel() % 10) + 10;
          } else if (h->GetBinContent(ibin) > pow(10, -6) && (result.getLevel() / 10 < 1)) {
            result = (result.getLevel() % 10) + 20;
          }
        } else if (ibin < 102) {
          if (h->GetBinContent(ibin) > pow(10, -5)) {
            result = (result.getLevel() % 10) + 10;
          } else if (h->GetBinContent(ibin) > pow(10, -6) && (result.getLevel() / 10 < 1)) {
            result = (result.getLevel() % 10) + 20;
          }
        } else {
          if (h->GetBinContent(ibin) > pow(10, -5)) {
            result = (result.getLevel() % 10) + 10;
          } else if (h->GetBinContent(ibin) > pow(10, -6) && (result.getLevel() / 10 < 1)) {
            result = (result.getLevel() % 10) + 20;
          }
        }
      }
    } else if (iter->second->getName() == "General/Noisy_Pixel") {
      auto* h = dynamic_cast<TH2Poly*>(iter->second->getObject());
      for (int ibin = 0; ibin < h->GetNumberOfBins(); ++ibin) {
        if (h->GetBinContent(ibin) == 0) {
          continue;
        }
        if (ibin < 48) {
          if ((h->GetBinContent(ibin) / (double)mNPixelPerStave[0]) > 0.0001) {
            result = (result.getLevel() % 100) + 100;
          } else if (h->GetBinContent(ibin) > 0.00005 && (result.getLevel() / 100 < 1)) {
            result = (result.getLevel() % 100) + 200;
          }
        } else if (ibin < 102) {
          if ((h->GetBinContent(ibin) / (double)mNPixelPerStave[1]) > 0.0001) {
            result = (result.getLevel() % 100) + 100;
          } else if ((h->GetBinContent(ibin) / (double)mNPixelPerStave[1]) > 0.00005 && (result.getLevel() / 100 < 1)) {
            result = (result.getLevel() % 100) + 200;
          }
        } else {
          if ((h->GetBinContent(ibin) / (double)mNPixelPerStave[2]) > 0.0001) {
            result = (result.getLevel() % 100) + 100;
          } else if ((h->GetBinContent(ibin) / (double)mNPixelPerStave[2]) > 0.00005 && (result.getLevel() / 100 < 1)) {
            result = (result.getLevel() % 100) + 200;
          }
        }
      }
    }
    TString objectName = iter->second->getName();
    if (objectName.Contains("ChipStave")) {
      auto* h = dynamic_cast<TH2D*>(iter->second->getObject());
      TString layerString = TString(objectName(15, 1).Data());
      int layer = layerString.Atoi();
      double maxOccupancy = h->GetMaximum();
      if (maxOccupancy > pow(10, -5)) {
        result = (result.getLevel() % (int)pow(10, 3 + layer)) + (2 * pow(10, 3 + layer));
      } else if (maxOccupancy > pow(10, -6)) {
        result = (result.getLevel() % (int)pow(10, 3 + layer)) + (1 * pow(10, 3 + layer));
      }
    }
  }
  return result;
}

std::string ITSFhrCheck::getAcceptedType() { return "TH1"; }

void ITSFhrCheck::beautify(std::shared_ptr<MonitorObject> mo, Quality checkResult)
{
  TLatex* text[5];
  if (mo->getName() == "General/ErrorPlots") {
    auto* h = dynamic_cast<TH1D*>(mo->getObject());
    if ((checkResult.getLevel() % 10) == 0) {
      text[0] = new TLatex(10, 0.6, "Quality::Good");
      text[1] = new TLatex(10, 0.4, "There is no Error found");
      for (int i = 0; i < 2; ++i) {
        text[i]->SetTextAlign(23);
        text[i]->SetTextSize(0.08);
        text[i]->SetTextColor(kGreen);
        h->GetListOfFunctions()->Add(text[i]);
      }
    } else if ((checkResult.getLevel() % 10) == 1) {
      text[0] = new TLatex(10, 0.7, "Quality::Bad");
      text[1] = new TLatex(10, 0.5, "Decoding ERROR detected");
      text[2] = new TLatex(10, 0.3, "Please Call Expert");
      for (int i = 0; i < 3; ++i) {
        text[i]->SetTextAlign(23);
        text[i]->SetTextSize(0.08);
        text[i]->SetTextColor(kRed);
        h->GetListOfFunctions()->Add(text[i]);
      }
    }
  } else if (mo->getName() == "General/General_Occupancy") {
    auto* h = dynamic_cast<TH2Poly*>(mo->getObject());
    if (((checkResult.getLevel() % 100) / 10) == 0) {
      text[0] = new TLatex(0, 0, "Quality::Good");
      text[0]->SetTextAlign(23);
      text[0]->SetTextSize(0.08);
      text[0]->SetTextColor(kGreen);
      h->GetListOfFunctions()->Add(text[0]);
    } else if (((checkResult.getLevel() % 100) / 10) == 1) {
      text[0] = new TLatex(0, 100, "Quality::Bad");
      text[1] = new TLatex(0, 0, "Max Occupancy over 10^{-5}");
      text[2] = new TLatex(0, -100, "Please Call Expert");
      for (int i = 0; i < 3; ++i) {
        text[i]->SetTextAlign(23);
        text[i]->SetTextSize(0.08);
        text[i]->SetTextColor(kRed);
        h->GetListOfFunctions()->Add(text[i]);
      }
    } else if (checkResult == Quality::Medium) {
      text[0] = new TLatex(0, 0, "Quality::Medium");
      text[1] = new TLatex(0, -100, "Max Occupancy over 10^{-6}");
      text[2] = new TLatex(0, -100, "Please Notify Expert On MM");
      for (int i = 0; i < 2; ++i) {
        text[i]->SetTextAlign(23);
        text[i]->SetTextSize(0.08);
        text[i]->SetTextColor(46);
        h->GetListOfFunctions()->Add(text[i]);
      }
    }
  } else if (mo->getName() == "General/Noisy_Pixel") {
    auto* h = dynamic_cast<TH2Poly*>(mo->getObject());
    if ((checkResult.getLevel() / 100) == 0) {
      text[0] = new TLatex(0, 0, "Quality::Good");
      text[0]->SetTextAlign(23);
      text[0]->SetTextSize(0.08);
      text[0]->SetTextColor(kGreen);
      h->GetListOfFunctions()->Add(text[0]);
    } else if ((checkResult.getLevel() / 100) == 1) {
      text[0] = new TLatex(0, 100, "Quality::Bad");
      text[1] = new TLatex(0, 0, "Noisy Pixel over 0.01%");
      text[2] = new TLatex(0, -100, "Please Call Expert");
      for (int i = 0; i < 3; ++i) {
        text[i]->SetTextAlign(23);
        text[i]->SetTextSize(0.08);
        text[i]->SetTextColor(kRed);
        h->GetListOfFunctions()->Add(text[i]);
      }
    } else if ((checkResult.getLevel() / 100) == 2) {
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
    if ((int)(checkResult.getLevel() / pow(10, layer + 3)) == 0) {
      text[0] = new TLatex(0.5, 0.6, "Quality::Good");
      text[0]->SetNDC();
      text[0]->SetTextAlign(23);
      text[0]->SetTextSize(0.08);
      text[0]->SetTextColor(kGreen);
      h->GetListOfFunctions()->Add(text[0]);
    } else if ((int)(checkResult.getLevel() / pow(10, layer + 3)) == 1) {
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
    } else if ((int)(checkResult.getLevel() / pow(10, layer + 3)) == 2) {
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

} // namespace o2::quality_control_modules::its
