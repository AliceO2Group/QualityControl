// Copyright 2019-2020 CERN and copyright holders of ALICE O2.
// See https://alice-o2.web.cern.ch/copyright for details of the copyright holders.
// All rights not expressly granted are reserved.
//
// This software is distributed under the terms of the GNU General Public
// License v3 (GPL Version 3), copied verbatim in the file "COPYING".
//
// In applyingthis license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.

///
/// \file   RawCheck.cxx
/// \author Cristina Terrevoli
///

#include <TCanvas.h>
#include <TH1.h>
#include <TH2.h>
#include <TPaveText.h>
#include <TList.h>
#include <TRobustEstimator.h>

#include "QualityControl/QcInfoLogger.h"
#include "QualityControl/MonitorObject.h"
#include "QualityControl/Quality.h"
#include <fairlogger/Logger.h>
#include "EMCAL/RawCheck.h"

using namespace std;

namespace o2::quality_control_modules::emcal
{

void RawCheck::configure(std::string) {}

Quality RawCheck::check(std::map<std::string, std::shared_ptr<MonitorObject>>* moMap)
{
  auto mo = moMap->begin()->second;
  Quality result = Quality::Good;
  if (mo->getName() == "ErrorTypePerSM") {
    auto* h = dynamic_cast<TH2*>(mo->getObject());
    if (h->GetEntries() != 0) {
      result = Quality::Bad;
    } // checker for the error type per SM
  }
  if (mo->getName() == "BunchMinRawAmplutudeFull_PHYS" || mo->getName().find("RawAmplMinEMCAL") == 0) {
    auto* h = dynamic_cast<TH1D*>(mo->getObject());
    Float_t totentries = h->Integral(10, 100); // Exclude dominant part of the channels for which the signal is in the noise range below pedestal
    h->GetXaxis()->SetRangeUser(20, 60);
    Int_t maxbin = h->GetMaximumBin();
    if (maxbin > 30 && maxbin < 50) {
      Float_t entries = h->GetBinContent(maxbin + 1);
      if (entries > 0.5 * totentries) {
        result = Quality::Bad;
      }
    }
  } // checker for the raw ampl histos (second peak around 40)
  if (mo->getName() == "FECidMaxChWithInput_perSM") {
    double errormargin = 2.;
    auto* h = dynamic_cast<TH2D*>(mo->getObject());
    bool hasBadFEC = false;
    for (int ism = 0; ism < h->GetXaxis()->GetNbins(); ism++) {
      std::unique_ptr<TH1> smbin(h->ProjectionY("nextsm", ism + 1, ism + 1));
      if (!smbin->GetEntries())
        continue;
      // get count rates for truncated mean
      std::vector<double> feccounts;
      for (int ifec = 0; ifec < smbin->GetXaxis()->GetNbins(); ifec++) {
        double countrate = smbin->GetBinContent(ifec + 1);
        if (countrate > DBL_EPSILON)
          feccounts.push_back(countrate);
      }
      // evaluate truncated mean to find outliers
      double mean, sigma;
      TRobustEstimator estimmator;
      estimmator.EvaluateUni(feccounts.size(), feccounts.data(), mean, sigma);
      // compare count rate of each FEC to truncated mean
      for (int ifec = 0; ifec < smbin->GetXaxis()->GetNbins(); ifec++) {
        double countrate = smbin->GetBinContent(ifec + 1);
        if (countrate > mean + errormargin * sigma) {
          hasBadFEC = true;
          break;
        }
      }
      if (hasBadFEC)
        break;
    }
    if (hasBadFEC)
      result = Quality::Bad;
  }
  if (mo->getName() == "PayloadSizeTFPerDDL") {
    std::map<int, double> meanPayloadSizeDDL;
    double errormargin = 2.;
    auto* h = dynamic_cast<TH2D*>(mo->getObject());
    std::vector<double> meanPayloads;
    for (int ism = 0; ism < h->GetXaxis()->GetNbins(); ism++) {
      std::unique_ptr<TH1> smbin(h->ProjectionY("nextsm", ism + 1, ism + 1));
      if (!smbin->GetEntries()) {
        meanPayloadSizeDDL[ism] = 0;
        continue;
      }
      auto meanPayload = smbin->GetMean();
      meanPayloads.push_back(meanPayload);
      meanPayloadSizeDDL[ism] = meanPayload;
      // get count rates for truncated mean
    }
    double mean, sigma;
    TRobustEstimator estimmator;
    estimmator.EvaluateUni(meanPayloads.size(), meanPayloads.data(), mean, sigma);
    for (auto [ddlID, payloadmean] : meanPayloadSizeDDL) {
      if (payloadmean > mean + errormargin * sigma) {
        result = Quality::Bad;
        break;
      }
    }
  }
  std::cout << " result " << result << std::endl;
  return result;
}

std::string RawCheck::getAcceptedType() { return "TH1"; }

void RawCheck::beautify(std::shared_ptr<MonitorObject> mo, Quality checkResult)
{
  if (mo->getName().find("Error") != std::string::npos) {
    auto* h = dynamic_cast<TH1*>(mo->getObject());
    TPaveText* msg = new TPaveText(0.5, 0.5, 0.9, 0.75, "NDC");
    h->GetListOfFunctions()->Add(msg);
    msg->SetName(Form("%s_msg", mo->GetName()));

    if (checkResult == Quality::Good) {
      //check the error type, loop on X entries to find the SM and on Y to find the Error Number
      //
      msg->Clear();
      msg->AddText("No Error: OK!!!");
      msg->SetFillColor(kGreen);
      //
      h->SetFillColor(kGreen);
    } else if (checkResult == Quality::Bad) {
      LOG(info) << "Quality::Bad, setting to red";
      msg->Clear();
      msg->AddText("Presence of Error Code"); //Type of the Error, in SM XX
      msg->AddText("If NOT a technical run,");
      msg->AddText("call EMCAL on-call.");
      h->SetFillColor(kRed);
    } else if (checkResult == Quality::Medium) {
      LOG(info) << "Quality::medium, setting to orange";
      h->SetFillColor(kOrange);
    }
    h->SetLineColor(kBlack);
  }
  if (mo->getName() == "BunchMinRawAmplutudeFull_PHYS") {
    auto* h = dynamic_cast<TH1*>(mo->getObject());
    TPaveText* msg = new TPaveText(0.5, 0.5, 0.9, 0.75, "NDC");
    h->GetListOfFunctions()->Add(msg);
    msg->SetName(Form("%s_msg", mo->GetName()));

    if (checkResult == Quality::Good) {
      msg->Clear();
      msg->AddText("No Error: OK!!!");
      msg->SetFillColor(kGreen);
      h->SetFillColor(kGreen);
    } else if (checkResult == Quality::Bad) {
      LOG(info) << "Quality::Bad, setting to red";
      msg->Clear();
      msg->AddText("Presence of a second peak around 40"); //Type of the Error, in SM XX
      msg->AddText("If NOT a technical run,");
      msg->AddText("call EMCAL on-call.");
      h->SetFillColor(kRed);
    } else if (checkResult == Quality::Medium) {
      LOG(info) << "Quality::medium, setting to orange";
      h->SetFillColor(kOrange);
    }
    h->SetLineColor(kBlack);
  }
  if (mo->getName() == "FECidMaxChWithInput_perSM" || "BunchMinRawAmplutudeFull_PHYS") {
    auto* h = dynamic_cast<TH1*>(mo->getObject());
    TPaveText* msg = new TPaveText(0.5, 0.5, 0.9, 0.75, "NDC");
    h->GetListOfFunctions()->Add(msg);
    msg->SetName(Form("%s_msg", mo->GetName()));

    if (checkResult == Quality::Good) {
      msg->Clear();
      msg->AddText("No Error: OK!!!");
      msg->SetFillColor(kGreen);
      h->SetFillColor(kGreen);
    } else if (checkResult == Quality::Bad) {
      LOG(info) << "Quality::Bad, setting to red";
      msg->Clear();
      msg->AddText("entry out of trend!"); //Type of the Error, in SM XX
      msg->AddText("If NOT a technical run,");
      msg->AddText("call EMCAL on-call.");
      h->SetFillColor(kRed);
    } else if (checkResult == Quality::Medium) {
      LOG(info) << "Quality::medium, setting to orange";
      h->SetFillColor(kOrange);
    }
    h->SetLineColor(kBlack);
  }
}

} // namespace o2::quality_control_modules::emcal
