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
/// \file   PadCalibrationCheck2D.cxx
/// \author Laura Serksnyte
///

#include "TPC/PadCalibrationCheck2D.h"
#include "QualityControl/MonitorObject.h"
#include "QualityControl/Quality.h"

#include <fairlogger/Logger.h>
// ROOT
#include <TCanvas.h>
#include <TH2.h>
#include <TPad.h>
#include <TList.h>
#include <TPaveText.h>

namespace o2::quality_control_modules::tpc
{

void PadCalibrationCheck2D::configure(std::string) {}
Quality PadCalibrationCheck2D::check(std::map<std::string, std::shared_ptr<MonitorObject>>* moMap)
{

  auto mo = moMap->begin()->second;
  Quality result = Quality::Null;
  if (mo->getName() == "c_ROCs_Pedestal_2D") {
    result = Quality::Good;
    auto* canv = (TCanvas*)mo->getObject();
    // Check all histograms in the canvas
    for (int tpads = 1; tpads <= 72; tpads++) {
      const auto padName = fmt::format("c_ROCs_Pedestal_2D_{:d}", tpads);
      const auto histName = fmt::format("h_Pedestals_ROC_{:02d}", tpads - 1);
      TPad* pad = (TPad*)canv->GetListOfPrimitives()->FindObject(padName.data());
      if (!pad) {
        badSectorsName.push_back("notitle");
        badSectorsQuality.push_back(Quality::Null);
        continue;
      }
      TH2F* h = (TH2F*)pad->GetListOfPrimitives()->FindObject(histName.data());
      if (!h) {
        badSectorsName.push_back("notitle");
        badSectorsQuality.push_back(Quality::Null);
        continue;
      }
      const std::string titleh = h->GetTitle();

      //check if we are dealing with IROC or OROC
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
          int val = h->GetBinContent(i, j);
          if (val > 0) {
            sum += 1;
          }
        }
      }

      // Check how many are off
      const float mediumLimit = 0.7;
      const float badLimit = 0.4;
      if (sum > badLimit * totalPads && sum < mediumLimit * totalPads) {
        if (result == Quality::Good) {
          result = Quality::Medium;
        }
        badSectorsName.push_back(titleh);
        badSectorsQuality.push_back(Quality::Medium);
      } else if (sum < badLimit * totalPads) {
        result = Quality::Bad;
        badSectorsName.push_back(titleh);
        badSectorsQuality.push_back(Quality::Bad);
      } else {
        badSectorsName.push_back(titleh);
        badSectorsQuality.push_back(Quality::Good);
      }
    }
  }
  return result;
}

std::string PadCalibrationCheck2D::getAcceptedType() { return "TCanvas"; }

void PadCalibrationCheck2D::beautify(std::shared_ptr<MonitorObject> mo, Quality)
{

  auto* canv = (TCanvas*)mo->getObject();
  // Loop over all histograms and beautify accordingly to specific quality object
  for (int tpads = 1; tpads <= 72; tpads++) {
    const auto padName = fmt::format("c_ROCs_Pedestal_2D_{:d}", tpads);
    const auto histName = fmt::format("h_Pedestals_ROC_{:02d}", tpads - 1);
    TPad* pad = (TPad*)canv->GetListOfPrimitives()->FindObject(padName.data());
    if (!pad) {
      continue;
    }
    pad->cd();
    TH2F* h = (TH2F*)pad->GetListOfPrimitives()->FindObject(histName.data());
    if (!h) {
      continue;
    }
    const std::string titleh = h->GetTitle();
    auto it = std::find(badSectorsName.begin(), badSectorsName.end(), titleh);
    if (it == badSectorsName.end()) {
      continue;
    }
    TPaveText* msg = new TPaveText(0.1, 0.9, 0.9, 0.95, "NDC");
    msg->SetBorderSize(1);
    const int index = std::distance(badSectorsName.begin(), it);
    Quality qualitySpecial = badSectorsQuality[index];
    msg->SetName(Form("%s_msg", mo->GetName()));

    if (qualitySpecial == Quality::Good) {
      msg->Clear();
      msg->AddText("Good");
      msg->SetFillColor(kGreen);
    } else if (qualitySpecial == Quality::Bad) {
      msg->Clear();
      msg->AddText("Bad");
      msg->SetFillColor(kRed);
    } else if (qualitySpecial == Quality::Medium) {
      msg->Clear();
      msg->AddText("Medium");
      msg->SetFillColor(kOrange);
    } else if (qualitySpecial == Quality::Null) {
      h->SetFillColor(0);
    }
    h->SetLineColor(kBlack);

    msg->Draw("same");
  }
}

} // namespace o2::quality_control_modules::tpc