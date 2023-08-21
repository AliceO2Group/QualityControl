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
/// \file   DigitCheck.cxx
/// \author Cristina Terrevoli
///

#include "EMCAL/DigitCheck.h"
#include "QualityControl/MonitorObject.h"
#include "QualityControl/Quality.h"
#include "QualityControl/QcInfoLogger.h"
#include <fairlogger/Logger.h>
// ROOT
#include <TH1.h>
#include <TH2.h>
#include <TPaveText.h>
#include <TLatex.h>
#include <TList.h>
#include <TRobustEstimator.h>
#include <ROOT/TSeq.hxx>
#include <iostream>
#include <vector>

using namespace std;

namespace o2::quality_control_modules::emcal
{

Quality DigitCheck::check(std::map<std::string, std::shared_ptr<MonitorObject>>* moMap)
{
  auto mo = moMap->begin()->second;
  Quality result = Quality::Good;
  //digitAmplidute_PHYS
  //digitTime_PHYS
  std::vector<std::string> amplitudeHist = { "digitAmplitudeEMCAL", "digitAmplitudeDCAL", "digitAmplitude_PHYS" };
  if (std::find(amplitudeHist.begin(), amplitudeHist.end(), mo->getName()) != amplitudeHist.end()) {
    auto* h = dynamic_cast<TH1*>(mo->getObject());
    if (h->GetEntries() == 0)
      result = Quality::Bad;
  }
  if (mo->getName() == "SMMaxNumDigits") {
    double errormargin = 2.;
    auto hist = dynamic_cast<TH1*>(mo->getObject());
    std::vector<double> smcounts;
    for (auto ib : ROOT::TSeqI(0, hist->GetXaxis()->GetNbins())) {
      auto countSM = hist->GetBinContent(ib + 1);
      if (countSM > 0)
        smcounts.emplace_back(countSM);
    }
    if (!smcounts.size()) {
      result = Quality::Medium;
    } else {
      TRobustEstimator meanfinder;
      double mean, sigma;
      meanfinder.EvaluateUni(smcounts.size(), smcounts.data(), mean, sigma);
      for (auto ib : ROOT::TSeqI(0, hist->GetXaxis()->GetNbins())) {
        if (hist->GetBinContent(ib + 1) > mean + errormargin * sigma) {
          result = Quality::Bad;
          break;
        }
      }
    }
  }

  /* if (mo->getName() == "digitTimeHG") {
    auto* h = dynamic_cast<TH2*>(mo->getObject());
    if (h->GetEntries() == 0)
      result = Quality::Bad;
    else {
      Float_t timeMean = h->GetMean();
      Float_t timeThrs = 60.;
      if (timeMean < timeThrs - 20. || timeMean > timeThrs + 20.) {
        result = Quality::Bad;
      }
    }
  }

  if (mo->getName() == "digitTimeLG") {
    auto* h = dynamic_cast<TH2*>(mo->getObject());
    if (h->GetEntries() == 0)
      result = Quality::Bad;
    else {
      Float_t timeMean = h->GetMean();
      Float_t timeThrs = 60;
      if (timeMean < timeThrs - 20 || timeMean > timeThrs + 20) {
        result = Quality::Bad;
      }
    }
  }
  */
  return result;
}

std::string DigitCheck::getAcceptedType() { return "TH1"; }

void DigitCheck::beautify(std::shared_ptr<MonitorObject> mo, Quality checkResult)
{
  if (mo->getName().find("Time") != std::string::npos) {
    auto* h = dynamic_cast<TH2*>(mo->getObject());
    TPaveText* msg = new TPaveText(0.5, 0.5, 0.9, 0.75, "NDC");
    h->GetListOfFunctions()->Add(msg);
    msg->SetName(Form("%s_msg", mo->GetName()));

    if (checkResult == Quality::Good) {
      //
      msg->Clear();
      msg->AddText("Mean inside limits: OK!!!");
      msg->SetFillColor(kGreen);
      //
      h->SetFillColor(kGreen);
    } else if (checkResult == Quality::Bad) {
      ILOG(Debug, Devel) << "Quality::Bad, setting to red";
      msg->Clear();
      msg->AddText("Mean outside limits or no entries");
      msg->AddText("If NOT a technical run,");
      msg->AddText("call EMCAL on-call.");
      h->SetFillColor(kRed);
    } else if (checkResult == Quality::Medium) {
      ILOG(Debug, Devel) << "Quality::medium, setting to orange";
      h->SetFillColor(kOrange);
    }
    h->SetLineColor(kBlack);
  }
  if (mo->getName().find("Amplitude") != std::string::npos) {
    auto* h = dynamic_cast<TH1*>(mo->getObject());
    TPaveText* msg = new TPaveText(0.5, 0.5, 0.9, 0.75, "NDC");
    h->GetListOfFunctions()->Add(msg);
    msg->SetName(Form("%s_msg", mo->GetName()));

    if (checkResult == Quality::Good) {
      //
      msg->Clear();
      msg->AddText("Mean inside limits: OK!!!");
      msg->SetFillColor(kGreen);
      //
      h->SetFillColor(kGreen);
    } else if (checkResult == Quality::Bad) {
      ILOG(Debug, Devel) << "Quality::Bad, setting to red";
      msg->Clear();
      msg->AddText("Mean outside limits or no entries");
      msg->AddText("If NOT a technical run,");
      msg->AddText("call EMCAL on-call.");
      h->SetFillColor(kRed);
    } else if (checkResult == Quality::Medium) {
      ILOG(Debug, Devel) << "Quality::medium, setting to orange";
      h->SetFillColor(kOrange);
    }
    h->SetLineColor(kBlack);
  }
  if (mo->getName() == "SMMaxNumDigits") {
    auto* h = dynamic_cast<TH1*>(mo->getObject());
    if (checkResult == Quality::Good) {
      TLatex* msg = new TLatex(0.2, 0.8, "#color[418]{Data OK}");
      msg->SetNDC();
      msg->SetTextSize(16);
      msg->SetTextFont(43);
      h->SetFillColor(kGreen);
      h->GetListOfFunctions()->Add(msg);
      msg->Draw();
    } else if (checkResult == Quality::Bad) {
      ILOG(Debug, Devel) << "Quality::Bad, setting to red";
      TLatex* msg = new TLatex(0.2, 0.8, "#color[2]{Noisy supermodule detected}");
      //Large payload in several DDLs
      msg->SetNDC();
      msg->SetTextSize(16);
      msg->SetTextFont(43);
      msg->SetTextColor(kRed);
      h->GetListOfFunctions()->Add(msg);
      msg->Draw();
      msg = new TLatex(0.2, 0.7, "#color[2]{If NOT techn.run: call EMCAL oncall}");
      msg->SetNDC();
      msg->SetTextSize(16);
      msg->SetTextFont(43);
      msg->SetTextColor(kRed);
      h->GetListOfFunctions()->Add(msg);
      msg->Draw();
    } else if (checkResult == Quality::Medium) {
      ILOG(Debug, Devel) << "Quality::medium, setting to orange";
      TLatex* msg = new TLatex(0.2, 0.8, "#color[42]{empty:if in run, call EMCAL-oncall}");
      msg->SetNDC();
      msg->SetTextSize(16);
      msg->SetTextFont(43);
      h->GetListOfFunctions()->Add(msg);
      msg->Draw();
    }
    h->SetLineColor(kBlack);
  }
}
} // namespace o2::quality_control_modules::emcal
