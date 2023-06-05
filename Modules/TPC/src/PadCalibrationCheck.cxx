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
/// \file   PadCalibrationCheck.cxx
/// \author Laura Serksnyte
///

#include "TPC/PadCalibrationCheck.h"
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
void PadCalibrationCheck::configure()
{
  if (auto param = mCustomParameters.find("mediumQualityNoiseMean"); param != mCustomParameters.end()) {
    mMediumQualityLimitNoiseMean = std::atof(param->second.c_str());
  }
  if (auto param = mCustomParameters.find("badQualityNoiseMean"); param != mCustomParameters.end()) {
    mBadQualityLimitNoiseMean = std::atof(param->second.c_str());
  }
}

//______________________________________________________________________________
Quality PadCalibrationCheck::check(std::map<std::string, std::shared_ptr<MonitorObject>>* moMap)
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
    // If it is a noise histogram, not only the quality but also the counts, mean and standart deviation excluding 0 bin must be stored
    if (moName == "c_Sides_Noise" || moName == "c_ROCs_Noise_1D") {
      result = Quality::Good;
      if (moName == "c_Sides_Noise") {
        padsstart = 3;
        padsTotal = 4;
      }
      if (moName == "c_ROCs_Noise_1D") {
        padsstart = 1;
        padsTotal = 72;
      }
      auto* canv = dynamic_cast<TCanvas*>(mo->getObject());
      if (!canv) {
        continue;
      }
      // Check all histograms in the canvas
      for (int tpads = padsstart; tpads <= padsTotal; tpads++) {
        const auto padName = fmt::format("{:s}_{:d}", moName, tpads);
        TPad* pad = (TPad*)canv->GetListOfPrimitives()->FindObject(padName.data());
        if (!pad) {
          continue;
        }
        TH1F* h = nullptr;
        if (moName == "c_Sides_Noise") {
          if (tpads == 3) {
            histName = "h_Aside_1D_Noise";
          }
          if (tpads == 4) {
            histName = "h_Cside_1D_Noise";
          }
        } else if (moName == "c_ROCs_Noise_1D") {
          histName = fmt::format("h1_Noise_{:02d}", tpads - 1);
        }
        h = (TH1F*)pad->GetListOfPrimitives()->FindObject(histName.data());
        if (!h) {
          continue;
        }
        const std::string titleh = h->GetTitle();
        // Set histogram range to correct one and then obtain information about total counts, mean, standard deviation
        const int NX = h->GetNbinsX();
        h->GetXaxis()->SetRangeUser(h->GetXaxis()->GetBinCenter(2), h->GetXaxis()->GetBinCenter(NX));
        auto mean = h->GetMean();
        auto stdDev = h->GetStdDev();
        auto nonZeroEntries = h->Integral();
        mNoiseMean.push_back(mean);
        mNoiseStdDev.push_back(stdDev);
        mNoiseNonZeroEntries.push_back(nonZeroEntries);
        // check quality
        if (mean > mMediumQualityLimitNoiseMean && mean < mBadQualityLimitNoiseMean) {
          if (result == Quality::Good) {
            result = Quality::Medium;
          }
          mSectorsName.push_back(titleh);
          mSectorsQuality.push_back(Quality::Medium);
        } else if (mean > mBadQualityLimitNoiseMean) {
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
std::string PadCalibrationCheck::getAcceptedType() { return "TCanvas"; }

//______________________________________________________________________________
void PadCalibrationCheck::beautify(std::shared_ptr<MonitorObject> mo, Quality)
{
  auto moName = mo->getName();
  if (moName == "c_Sides_Noise" || moName == "c_ROCs_Noise_1D") {
    int padsTotal = 0, padsstart = 1000;
    auto* tcanv = dynamic_cast<TCanvas*>(mo->getObject());
    std::string histNameS, histName;
    if (moName == "c_Sides_Noise") {
      padsstart = 3;
      padsTotal = 4;
    } else if (moName == "c_ROCs_Noise_1D") {
      padsstart = 1;
      padsTotal = 72;
      histNameS = "h1_Noise";
    }
    for (int tpads = padsstart; tpads <= padsTotal; tpads++) {
      const std::string padName = fmt::format("{:s}_{:d}", moName, tpads);
      TPad* pad = (TPad*)tcanv->GetListOfPrimitives()->FindObject(padName.data());
      if (!pad) {
        continue;
      }
      pad->cd();
      TH1F* h = nullptr;
      // In case of working on noise information
      if (moName == "c_Sides_Noise") {
        if (tpads == 3) {
          histName = "h_Aside_1D_Noise";
        }
        if (tpads == 4) {
          histName = "h_Cside_1D_Noise";
        }
      } else {
        histName = fmt::format("{:s}_{:02d}", histNameS, tpads - 1);
      }
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
      TPaveText* msg = new TPaveText(0.7, 0.8, 0.898, 0.9, "NDC");
      h->SetStats(0);
      msg->SetBorderSize(1);
      msg->SetName(Form("%s_msg", mo->GetName()));
      msg->Clear();
      msg->AddText(fmt::format("Entries: {:d}", mNoiseNonZeroEntries[index]).data());
      msg->AddText(fmt::format("Mean: {:.4f}", mNoiseMean[index]).data());
      msg->AddText(fmt::format("Std Dev: {:.4f}", mNoiseStdDev[index]).data());
      msg->Draw("same");

      // In case of all histograms
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
    mSectorsName.clear();
    mSectorsQuality.clear();
    mNoiseMean.clear();
    mNoiseStdDev.clear();
    mNoiseNonZeroEntries.clear();
  }
}

} // namespace o2::quality_control_modules::tpc
