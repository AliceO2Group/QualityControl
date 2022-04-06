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
/// \file   CheckForEmptyPads.cxx
/// \author Laura Serksnyte
///

#include "TPC/CheckForEmptyPads.h"
#include "QualityControl/MonitorObject.h"
#include "QualityControl/Quality.h"

#include <fairlogger/Logger.h>
// ROOT
#include <TCanvas.h>
#include <TH1.h>
#include <TH2.h>
#include <TPad.h>
#include <TList.h>
#include <TPaveText.h>

#include <iostream>

namespace o2::quality_control_modules::tpc
{
void CheckForEmptyPads::configure()
{
  if (auto param = mCustomParameters.find("mediumQualityPercentageOfWorkingPads"); param != mCustomParameters.end()) {
    mMediumQualityLimit = std::atof(param->second.c_str());
  }
  if (auto param = mCustomParameters.find("badQualityPercentageOfWorkingPads"); param != mCustomParameters.end()) {
    mBadQualityLimit = std::atof(param->second.c_str());
  }
  if (auto param = mCustomParameters.find("MOsNames2D"); param != mCustomParameters.end()) {
    auto temp = param->second.c_str();
    std::istringstream ss(temp);
    std::string token;
    while (std::getline(ss, token, ',')) {
      mMOsToCheck2D.emplace_back(token);
    }
  }
}

//______________________________________________________________________________
Quality CheckForEmptyPads::check(std::map<std::string, std::shared_ptr<MonitorObject>>* moMap)
{
  Quality result = Quality::Null;
  for (auto const& moObj : *moMap) {
    auto mo = moObj.second;
    if (!mo) {
      continue;
    }
    auto moName = mo->getName();
    std::string histName, histNameS;
    int padsTotal = 0, padsstart = 1000;
    if (auto it = std::find(mMOsToCheck2D.begin(), mMOsToCheck2D.end(), moName); it != mMOsToCheck2D.end()) {
      size_t end = moName.find("_2D");
      auto histSubName = moName.substr(7, end - 7);
      result = Quality::Good;
      auto* canv = (TCanvas*)mo->getObject();
      if (!canv)
        continue;
      // Check all histograms in the canvas
      for (int tpads = 1; tpads <= 72; tpads++) {
        const auto padName = fmt::format("{:s}_{:d}", moName, tpads);
        const auto histName = fmt::format("h_{:s}_ROC_{:02d}", histSubName, tpads - 1);
        TPad* pad = (TPad*)canv->GetListOfPrimitives()->FindObject(padName.data());
        if (!pad) {
          mSectorsName.push_back("notitle");
          mSectorsQuality.push_back(Quality::Null);
          continue;
        }
        TH2F* h = (TH2F*)pad->GetListOfPrimitives()->FindObject(histName.data());
        if (!h) {
          mSectorsName.push_back("notitle");
          mSectorsQuality.push_back(Quality::Null);
          continue;
        }
        const std::string titleh = h->GetTitle();

        // check if we are dealing with IROC or OROC
        int totalPads = 0;
        if (titleh.find("IROC") != std::string::npos) {
          totalPads = 5280;
        } else if (titleh.find("OROC") != std::string::npos) {
          totalPads = 9280;
        } else {
          return Quality::Null;
        }
        const int NX = h->GetNbinsX();
        const int NY = h->GetNbinsY();
        // Check how many of the pads are non zero
        int sum = 0;
        for (int i = 1; i <= NX; i++) {
          for (int j = 1; j <= NY; j++) {
            float val = h->GetBinContent(i, j);
            if (val > 0.) {
              sum += 1;
            }
          }
        }
        // Check how many are off
        if (sum > mBadQualityLimit * totalPads && sum < mMediumQualityLimit * totalPads) {
          if (result == Quality::Good) {
            result = Quality::Medium;
          }
          mSectorsName.push_back(titleh);
          mSectorsQuality.push_back(Quality::Medium);
        } else if (sum < mBadQualityLimit * totalPads) {
          result = Quality::Bad;
          mSectorsName.push_back(titleh);
          mSectorsQuality.push_back(Quality::Bad);
        } else {
          mSectorsName.push_back(titleh);
          mSectorsQuality.push_back(Quality::Good);
        }
      }
    }
  } // end of loop over moMap

  return result;
}

//______________________________________________________________________________
std::string CheckForEmptyPads::getAcceptedType() { return "TCanvas"; }

//______________________________________________________________________________
void CheckForEmptyPads::beautify(std::shared_ptr<MonitorObject> mo, Quality)
{
  auto moName = mo->getName();
  if (auto it = std::find(mMOsToCheck2D.begin(), mMOsToCheck2D.end(), moName); it != mMOsToCheck2D.end()) {
    int padsTotal = 0, padsstart = 1000;
    auto* tcanv = (TCanvas*)mo->getObject();
    std::string histNameS, histName;
    padsstart = 1;
    padsTotal = 72;
    size_t end = moName.find("_2D");
    auto histSubName = moName.substr(7, end - 7);
    histNameS = fmt::format("h_{:s}_ROC", histSubName);
    for (int tpads = padsstart; tpads <= padsTotal; tpads++) {
      const std::string padName = fmt::format("{:s}_{:d}", moName, tpads);
      TPad* pad = (TPad*)tcanv->GetListOfPrimitives()->FindObject(padName.data());
      if (!pad) {
        continue;
      }
      pad->cd();
      TH1F* h = nullptr;
      histName = fmt::format("{:s}_{:02d}", histNameS, tpads - 1);
      h = (TH1F*)pad->GetListOfPrimitives()->FindObject(histName.data());
      if (!h) {
        continue;
      }
      const std::string titleh = h->GetTitle();
      auto it = std::find(mSectorsName.begin(), mSectorsName.end(), titleh);
      if (it == mSectorsName.end()) {
        continue;
      }
      const int index = std::distance(mSectorsName.begin(), it);
      TPaveText* msgQuality = new TPaveText(0.1, 0.9, 0.9, 0.95, "NDC");
      msgQuality->SetBorderSize(1);
      Quality qualitySpecial = mSectorsQuality[index];
      msgQuality->SetName(Form("%s_msg", mo->GetName()));
      if (qualitySpecial == Quality::Good) {
        msgQuality->Clear();
        msgQuality->AddText("Good");
        msgQuality->SetFillColor(kGreen);
      } else if (qualitySpecial == Quality::Bad) {
        msgQuality->Clear();
        msgQuality->AddText("Bad");
        msgQuality->SetFillColor(kRed);
      } else if (qualitySpecial == Quality::Medium) {
        msgQuality->Clear();
        msgQuality->AddText("Medium");
        msgQuality->SetFillColor(kOrange);
      } else if (qualitySpecial == Quality::Null) {
        h->SetFillColor(0);
      }
      h->SetLineColor(kBlack);
      msgQuality->Draw("same");
    }
    auto savefileNAme = fmt::format("/home/ge56luj/Desktop/ThisIsBeautifyObject{:s}.pdf", moName);
    tcanv->SaveAs(savefileNAme.c_str());
    mSectorsName.clear();
    mSectorsQuality.clear();
  }
}

} // namespace o2::quality_control_modules::tpc
