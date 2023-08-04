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
/// \file   CellCheck.cxx
/// \author Cristina Terrevoli
///

#include "EMCAL/CellCheck.h"
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
#include <TLine.h>
#include <TRobustEstimator.h>
#include <ROOT/TSeq.hxx>
#include <iostream>
#include <vector>

using namespace std;

namespace o2::quality_control_modules::emcal
{

Quality CellCheck::check(std::map<std::string, std::shared_ptr<MonitorObject>>* moMap)
{
  auto mo = moMap->begin()->second;
  Quality result = Quality::Good;
  // cellAmplidute_PHYS
  // cellTime_PHYS
  std::vector<std::string> amplitudeHist = { "cellAmplitudeEMCAL", "cellAmplitudeDCAL", "cellAmplitude_PHYS" };
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

  /* if (mo->getName() == "cellTimeHG") {
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

  if (mo->getName() == "cellTimeLG") {
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

std::string CellCheck::getAcceptedType() { return "TH1"; }

void CellCheck::beautify(std::shared_ptr<MonitorObject> mo, Quality checkResult)
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
      // Large payload in several DDLs
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
  if (mo->getName().find("cellOccupancy") != std::string::npos) {
    auto* h2D = dynamic_cast<TH2*>(mo->getObject());
    // orizontal
    TLine* l1 = new TLine(-0.5, 24, 95.5, 24);
    TLine* l2 = new TLine(-0.5, 48, 95.5, 48);
    TLine* l3 = new TLine(-0.5, 72, 95.5, 72);
    TLine* l4 = new TLine(-0.5, 96, 95.5, 96);
    TLine* l5 = new TLine(-0.5, 120, 95.5, 120);
    TLine* l6 = new TLine(-0.5, 128, 95.5, 128);

    TLine* l7 = new TLine(-0.5, 152, 31.5, 152);
    TLine* l8 = new TLine(63.5, 152, 95.5, 152);

    TLine* l9 = new TLine(-0.5, 176, 31.5, 176);
    TLine* l10 = new TLine(63.5, 176, 95.5, 176);

    TLine* l11 = new TLine(-0.5, 200, 95.5, 200);
    TLine* l12 = new TLine(47.5, 200, 47.5, 207.5);

    //vertical
    TLine* l13 = new TLine(47.5, -0.5, 47.5, 128);
    TLine* l14 = new TLine(31.5, 128, 31.5, 200);
    TLine* l15 = new TLine(63.5, 128, 63.5, 200);

    h2D->GetListOfFunctions()->Add(l1);
    h2D->GetListOfFunctions()->Add(l2);
    h2D->GetListOfFunctions()->Add(l3);
    h2D->GetListOfFunctions()->Add(l4);
    h2D->GetListOfFunctions()->Add(l5);
    h2D->GetListOfFunctions()->Add(l6);
    h2D->GetListOfFunctions()->Add(l7);
    h2D->GetListOfFunctions()->Add(l8);
    h2D->GetListOfFunctions()->Add(l9);
    h2D->GetListOfFunctions()->Add(l10);
    h2D->GetListOfFunctions()->Add(l11);
    h2D->GetListOfFunctions()->Add(l12);
    h2D->GetListOfFunctions()->Add(l13);
    h2D->GetListOfFunctions()->Add(l14);
    h2D->GetListOfFunctions()->Add(l15);

    l1->Draw();
    l2->Draw();
    l3->Draw();
    l4->Draw();
    l5->Draw();
    l6->Draw();
    l7->Draw();
    l8->Draw();
    l9->Draw();
    l10->Draw();
    l11->Draw();
    l12->Draw();
    l13->Draw();
    l14->Draw();
    l15->Draw();
  }
}
} // namespace o2::quality_control_modules::emcal
