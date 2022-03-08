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

void QcMFTDigitCheck::configure() {}

Quality QcMFTDigitCheck::check(std::map<std::string, std::shared_ptr<MonitorObject>>* moMap)
{
  Quality result = Quality::Null;

  for (auto& [moName, mo] : *moMap) {

    (void)moName;

    if (mo->getName() == "mDigitChipOccupancy") {
      auto* histogram = dynamic_cast<TH1F*>(mo->getObject());

      float den = histogram->GetBinContent(0); // normalisation stored in the uderflow bin

      for (int iBin = 0; iBin < histogram->GetNbinsX(); iBin++) {
        float num = histogram->GetBinContent(iBin + 1);
        float ratio = (den > 0) ? (num / den) : 0.0;
        histogram->SetBinContent(iBin + 1, ratio);
      }
    }

    if (mo->getName() == "mDigitOccupancySummary") {
      auto* histogram = dynamic_cast<TH2F*>(mo->getObject());

      float den = histogram->GetBinContent(0, 0); // normalisation stored in the uderflow bin

      for (int iBinX = 0; iBinX < histogram->GetNbinsX(); iBinX++) {
        for (int iBinY = 0; iBinY < histogram->GetNbinsY(); iBinY++) {
          float num = histogram->GetBinContent(iBinX + 1, iBinY + 1);
          float ratio = (den > 0) ? (num / den) : 0.0;
          histogram->SetBinContent(iBinX + 1, iBinY + 1, ratio);
        }
      }
    }

    if (mo->getName().find("mDigitChipStdDev") != std::string::npos) {
      auto* histogram = dynamic_cast<TH1F*>(mo->getObject());

      // test it
      if (histogram->GetBinContent(histogram->GetMinimumBin()) > 250) {
        // result = Quality::Good;
      }
      if ((histogram->GetBinContent(histogram->GetMinimumBin()) < 250) && (histogram->GetBinContent(histogram->GetMinimumBin()) > 200)) {
        // result = Quality::Medium;
      }
      if (histogram->GetBinContent(histogram->GetMinimumBin()) < 200) {
        // result = Quality::Bad;
      }
    }
  }
  return result;
}

std::string QcMFTDigitCheck::getAcceptedType() { return "TH1"; }

void QcMFTDigitCheck::beautify(std::shared_ptr<MonitorObject> mo, Quality checkResult)
{
  if (mo->getName().find("mDigitChipStdDev") != std::string::npos) {
    auto* histogram = dynamic_cast<TH1F*>(mo->getObject());

    if (checkResult == Quality::Good) {
      LOG(info) << "Quality::Good";
      TLatex* tl = new TLatex(350, 1.05 * histogram->GetMaximum(), "#color[418]{Dummy check status: Good!}");
      histogram->GetListOfFunctions()->Add(tl);
      tl->Draw();
    } else if (checkResult == Quality::Bad) {
      LOG(info) << "Quality::Bad";
      TLatex* tl = new TLatex(350, 1.05 * histogram->GetMaximum(), "#color[633]{Dummy check status: Bad!}");
      histogram->GetListOfFunctions()->Add(tl);
      tl->Draw();
    } else if (checkResult == Quality::Medium) {
      LOG(info) << "Quality::Medium";
      TLatex* tl = new TLatex(350, 1.05 * histogram->GetMaximum(), "#color[800]{Dummy check status: Medium!}");
      histogram->GetListOfFunctions()->Add(tl);
      tl->Draw();
    }
  }
}

} // namespace o2::quality_control_modules::mft
