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
/// \file   RawCheck.cxx
/// \author Cristina Terrevoli
///

#include <TCanvas.h>
#include <TH1.h>
#include <TH2.h>
#include <TPaveText.h>
#include <TLatex.h>
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
  if (mo->getName().find("BunchMinRawAmplitude") == 0) {
    auto* h = dynamic_cast<TH1*>(mo->getObject());
    if (h->GetEntries() == 0) {
      result = Quality::Medium;
    } else {
      Float_t totentries = h->Integral(10, 100); // Exclude dominant part of the channels for which the signal is in the noise range below pedestal
      Float_t entriesBadRegion = h->Integral(20, 60);
      int first = h->GetXaxis()->GetFirst(),
          last = h->GetXaxis()->GetLast();
      h->GetXaxis()->SetRangeUser(20, 60);
      Int_t maxbin = h->GetMaximumBin();
      h->GetXaxis()->SetRange(first, last);
      if (maxbin > 20 && maxbin < 50) {
        //Float_t entries = h->GetBinContent(maxbin + 1);
        if (entriesBadRegion > 0.5 * totentries) {
          result = Quality::Bad;
        }
      }
    } // checker for the raw ampl histos (second peak around 40)
  }
  if (mo->getName() == "FECidMaxChWithInput_perSM") {
    double errormargin = 2.;
    auto* h = dynamic_cast<TH2*>(mo->getObject());
    if (h->GetEntries() == 0) {
      result = Quality::Medium;
    } else {
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
  }
  if (mo->getName() == "PayloadSizeTFPerDDL") {
    std::map<int, double> meanPayloadSizeDDL;
    double errormargin = 2.;
    auto* h = dynamic_cast<TH2*>(mo->getObject());
    if (h->GetEntries() == 0) {
      result = Quality::Medium;
    } else {
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
  }
  if (mo->getName().find("PayloadSize") != std::string::npos && mo->getName().find("1D") != std::string::npos) {
    double errormargin = 2.;
    auto* h = dynamic_cast<TH1*>(mo->getObject());
    if (!h->GetEntries()) {
      result = Quality::Medium;
    } else {
      // get count rates for truncated mean
      std::vector<double> payloadSize1D;
      for (int ipay = 0; ipay < h->GetXaxis()->GetNbins(); ipay++) {
        double countrate = h->GetBinContent(ipay + 1);
        if (countrate > DBL_EPSILON)
          payloadSize1D.push_back(countrate);
      }
      // evaluate truncated mean to find outliers
      double mean, sigma;
      TRobustEstimator estimmator;
      estimmator.EvaluateUni(payloadSize1D.size(), payloadSize1D.data(), mean, sigma);
      // compare count rate of each FEC to truncated mean
      for (int ipay = 0; ipay < h->GetXaxis()->GetNbins(); ipay++) {
        double countrate = h->GetBinContent(ipay + 1);
        if (countrate > mean + errormargin * sigma) {
          result = Quality::Bad;
        }
      }
    }
  }
  std::cout << " result " << result << std::endl;
  return result;
}

std::string RawCheck::getAcceptedType() { return "TH1"; }

void RawCheck::beautify(std::shared_ptr<MonitorObject> mo, Quality checkResult)
{
  QcInfoLogger::GetInstance().setDetector("EMC");
  if (mo->getName().find("Error") != std::string::npos) {
    auto* h = dynamic_cast<TH1*>(mo->getObject());

    if (checkResult == Quality::Good) {
      //check the error type, loop on X entries to find the SM and on Y to find the Error Number
      TLatex* msg = new TLatex(0.2, 0.8, "#color[418]{No Error: OK}");
      msg->SetNDC();
      msg->SetTextSize(16);
      msg->SetTextFont(43);
      h->GetListOfFunctions()->Add(msg);
      msg->Draw();
    } else if (checkResult == Quality::Bad) {
      LOG(info) << "Quality::Bad, setting to red";
      QcInfoLogger::GetInstance() << QcInfoLogger::Error << " QualityBad:Presence of Error Code" << AliceO2::InfoLogger::InfoLogger::endm;
      TLatex* msg = new TLatex(0.2, 0.8, "#color[2]{Presence of Error Code: call EMCAL oncall}");
      msg->SetNDC();
      msg->SetTextSize(16);
      msg->SetTextFont(43);
      h->GetListOfFunctions()->Add(msg);
      msg->Draw();
      h->SetFillColor(kRed);
    } else if (checkResult == Quality::Medium) {
      LOG(info) << "Quality::medium, setting to orange";
      h->SetFillColor(kOrange);
    }
    h->SetLineColor(kBlack);
  }
  if (mo->getName().find("BunchMinRawAmplitude") != std::string::npos) {
    auto* h = dynamic_cast<TH1*>(mo->getObject());
    if (checkResult == Quality::Good) {
      TLatex* msg = new TLatex(0.3, 0.8, "#color[418]{Data OK}");
      msg->SetNDC();
      msg->SetTextSize(16);
      msg->SetTextFont(43);
      h->GetListOfFunctions()->Add(msg);
      msg->Draw();
      h->SetFillColor(kGreen);
    } else if (checkResult == Quality::Bad) {
      LOG(info) << "Quality::Bad, setting to red";
      QcInfoLogger::GetInstance() << QcInfoLogger::Error << " QualityBad:Bunch min Amplitude outside limits " << AliceO2::InfoLogger::InfoLogger::endm;
      TLatex* msg = new TLatex(0.2, 0.8, "#color[2]{Pedestal peak detected}");
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
      h->SetFillColor(kRed);
      h->GetListOfFunctions()->Add(msg);
      msg->Draw();
    } else if (checkResult == Quality::Medium) {
      LOG(info) << "Quality::medium, setting to orange";
      TLatex* msg = new TLatex(0.2, 0.8, "#color[42]{empty:if in run, call EMCAL oncall}");
      msg->SetNDC();
      msg->SetTextSize(16);
      msg->SetTextFont(43);
      h->GetListOfFunctions()->Add(msg);
      msg->Draw();
    }
    h->SetLineColor(kBlack);
  }
  std::vector<std::string> payloadhists = { "FECidMaxChWithInput_perSM", "PayloadSizeTFPerDDL", "PayloadSizeTFPerDDL_1D", "PayloadSizePerDDL_1D" };
  if (std::find(payloadhists.begin(), payloadhists.end(), mo->getName()) != payloadhists.end()) {
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
      LOG(info) << "Quality::Bad, setting to red";
      TLatex* msg;
      if (mo->getName() == "FECidMaxChWithInput_perSM")
        msg = new TLatex(0.2, 0.8, "#color[2]{Noisy FEC detected}");
      if (mo->getName().find("PayloadSize") != std::string::npos)
        msg = new TLatex(0.2, 0.8, "#color[2]{Large payload in several DDLs}");
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
      LOG(info) << "Quality::medium, setting to orange";
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
