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
/// \file   QcMFTDigitCheck.cxx
/// \author Tomas Herman
/// \author Guillermo Contreras
/// \author Katarina Krizkova Gajdosova
/// \author Diana Maria Krupova

// Fair
#include <fairlogger/Logger.h>
// ROOT
#include <TH1.h>
#include <TH2.h>
#include <TLatex.h>
#include <TList.h>
// Quality Control
#include "MFT/QcMFTDigitCheck.h"
#include "QualityControl/MonitorObject.h"
#include "QualityControl/Quality.h"
#include "QualityControl/QcInfoLogger.h"

using namespace std;

namespace o2::quality_control_modules::mft
{

void QcMFTDigitCheck::configure()
{

  // this is how to get access to custom parameters defined in the config file at qc.tasks.<task_name>.taskParameters
  if (auto param = mCustomParameters.find("ZoneThresholdMedium"); param != mCustomParameters.end()) {
    ILOG(Info, Support) << "Custom parameter - ZoneThresholdMedium: " << param->second << ENDM;
    mZoneThresholdMedium = stoi(param->second);
  }
  if (auto param = mCustomParameters.find("ZoneThresholdBad"); param != mCustomParameters.end()) {
    ILOG(Info, Support) << "Custom parameter - ZoneThresholdBad: " << param->second << ENDM;
    mZoneThresholdBad = stoi(param->second);
  }
}

Quality QcMFTDigitCheck::check(std::map<std::string, std::shared_ptr<MonitorObject>>* moMap)
{
  Quality result = Quality::Null;

  for (auto& [moName, mo] : *moMap) {

    (void)moName;

    if (mo->getName() == "mDigitChipOccupancy") {
      auto* hChipOccupancy = dynamic_cast<TH1F*>(mo->getObject());

      float den = hChipOccupancy->GetBinContent(0); // normalisation stored in the uderflow bin

      for (int iBin = 0; iBin < hChipOccupancy->GetNbinsX(); iBin++) {
        if (hChipOccupancy->GetBinContent(iBin + 1) == 0) {
          hChipOccupancy->Fill(937); // number of chips with zero digits stored in the overflow bin
        }
        float num = hChipOccupancy->GetBinContent(iBin + 1);
        float ratio = (den > 0) ? (num / den) : 0.0;
        hChipOccupancy->SetBinContent(iBin + 1, ratio);
      }
    }

    if (mo->getName() == "mDigitOccupancySummary") {
      auto* hOccupancySummary = dynamic_cast<TH2F*>(mo->getObject());

      float den = hOccupancySummary->GetBinContent(0, 0); // normalisation stored in the uderflow bin
      float nEmptyBins = 0;                               // number of empty zones stored here

      for (int iBinX = 0; iBinX < hOccupancySummary->GetNbinsX(); iBinX++) {
        for (int iBinY = 0; iBinY < hOccupancySummary->GetNbinsY(); iBinY++) {
          float num = hOccupancySummary->GetBinContent(iBinX + 1, iBinY + 1);
          float ratio = (den > 0) ? (num / den) : 0.0;
          hOccupancySummary->SetBinContent(iBinX + 1, iBinY + 1, ratio);
          if ((hOccupancySummary->GetBinContent(iBinX + 1, iBinY + 1)) == 0) {
            nEmptyBins = nEmptyBins + 1;
          }
        }
      }

      if (nEmptyBins <= mZoneThresholdMedium) {
        result = Quality::Good;
      }
      if (nEmptyBins > mZoneThresholdMedium && nEmptyBins <= mZoneThresholdBad) {
        result = Quality::Medium;
      }
      if (nEmptyBins > mZoneThresholdBad) {
        result = Quality::Bad;
      }
    }
  }
  return result;
}

std::string QcMFTDigitCheck::getAcceptedType() { return "TH1"; }

void QcMFTDigitCheck::beautify(std::shared_ptr<MonitorObject> mo, Quality checkResult)
{

  if (mo->getName().find("mDigitOccupancySummary") != std::string::npos) {
    auto* hOccupancySummary = dynamic_cast<TH2F*>(mo->getObject());

    if (checkResult == Quality::Good) {
      LOG(info) << "Quality::Good";
      TLatex* tl = new TLatex(0.15, 6.0, "Quality Good");
      tl->SetTextColor(kGreen);
      hOccupancySummary->GetListOfFunctions()->Add(tl);
      tl->Draw();
    } else if (checkResult == Quality::Medium) {
      LOG(info) << "Quality::Medium";
      TLatex* tl = new TLatex(0.15, 6.0, "Quality medium: write a logbook entry tagging MFT");
      tl->SetTextColor(kOrange);
      hOccupancySummary->GetListOfFunctions()->Add(tl);
      tl->Draw();
    } else if (checkResult == Quality::Bad) {
      LOG(info) << "Quality::Bad";
      TLatex* tl = new TLatex(0.15, 6.0, "Quality bad: call the on-call!");
      tl->SetTextColor(kRed);
      hOccupancySummary->GetListOfFunctions()->Add(tl);
      tl->Draw();
    }
  }
}

} // namespace o2::quality_control_modules::mft
