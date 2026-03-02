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
#include <array>
#include <bitset>
#include <memory>
#include <string>
#include <exception>

#include <TCanvas.h>
#include <TH1.h>
#include <TH2.h>
#include <TLatex.h>
#include <TList.h>
#include <TLine.h>
#include <TRobustEstimator.h>

#include "QualityControl/QcInfoLogger.h"
#include "QualityControl/MonitorObject.h"
#include "QualityControl/Quality.h"
#include <fairlogger/Logger.h>
#include "EMCAL/RawCheck.h"
#include "QualityControl/stringUtils.h"

#include <DataFormatsQualityControl/FlagType.h>
#include <DataFormatsQualityControl/FlagTypeFactory.h>

using namespace std;

namespace o2::quality_control_modules::emcal
{

void RawCheck::configure()
{
  // switch on/off messages on the infoLogger
  loadConfigValueBool("MessageBunchMinAmpSM", mIlMessageBunchMinAmpCheckSM);
  loadConfigValueBool("MessageBunchMinAmpDetector", mIlMessageBunchMinAmpCheckDetector);
  loadConfigValueBool("MessageBunchMinAmpFull", mIlMessageBunchMinAmpCheckFull);
  loadConfigValueBool("MessageErrorCheck", mILMessageRawErrorCheck);
  loadConfigValueBool("MessageNoisyFECCheck", mILMessageNoisyFECCheck);
  loadConfigValueBool("MessagePayloadSizeCheck", mILMessagePayloadSizeCheck);
  // configure nsigma sigma-based checkers
  loadConfigValueDouble("SigmaFECMaxPayload", mNsigmaFECMaxPayload);
  loadConfigValueDouble("SigmaPayloadSize", mNsigmaPayloadSize);

  // configure bunch min. amp checker
  loadConfigValueInt("BunchMinAmpMinEntries", mBunchMinCheckMinEntries);
  loadConfigValueDouble("BunchMinAmpFractionSignal", mBunchMinCheckFractionSignal);
  loadConfigValueInt("BunchMinAmpMinEntriesSM", mBunchMinCheckMinEntriesSM);
  loadConfigValueDouble("BunchMinAmpFractionSignalSM", mBunchMinCheckFractionSignalSM);
  loadConfigValueInt("BunchMinAmpMinEntriesFEC", mBunchMinCheckMinEntriesFEC);
  loadConfigValueDouble("BunchMinAmpFractionSignalFEC", mBunchMinCheckFractionSignalFEC);

  mIndicesConverter.Initialize();
}

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
    if (mo->getObject()->InheritsFrom(TH2::Class())) {
      auto hist = static_cast<TH2*>(mo->getObject());
      if (!hist->GetEntries()) {
        result = Quality::Medium;
      } else {
        bool perSM = hist->GetYaxis()->GetNbins() == 20;
        auto [minentries, siganlfraction] = getCutsBunchMinAmpHist(perSM, !perSM);
        std::vector<int> badModules;
        auto modulequalities = runPedestalCheck2D(hist, minentries, siganlfraction);
        for (int imod = 0; imod < modulequalities.size(); imod++) {
          if (modulequalities[imod] == Quality::Bad) {
            badModules.emplace_back(imod);
          }
        }
        if (badModules.size()) {
          std::string message;
          if (perSM) {
            result = Quality::Bad;
            std::stringstream messagebuilder;
            for (auto imod : badModules) {
              messagebuilder << "Pedestals peak detected in SM " << imod << " " << mIndicesConverter.GetOnlineSMIndex(imod) << std::endl;
            }
            message = messagebuilder.str();
          } else {
            // check if we have Full SMs bad (at least 60 % of the expected FECs bad)
            auto fecsPerSM = reduceBadFECs(badModules);
            std::bitset<20> badsms;
            for (int ism = 0; ism < fecsPerSM.size(); ism++) {
              if (static_cast<double>(fecsPerSM[ism].size()) > 0.6 * static_cast<double>(getExepctedFECs(ism))) {
                badsms.set(ism, true);
              }
            }
            if (badsms.any()) {
              result = Quality::Bad;
            } else {
              result = Quality::Medium;
            }
            std::stringstream messagebuilder;
            for (int ism = 0; ism < fecsPerSM.size(); ism++) {
              if (badsms.test(ism)) {
                messagebuilder << "Pedestals not set full SM " << ism << " " << mIndicesConverter.GetOnlineSMIndex(ism) << std::endl;
              } else {
                for (auto& fec : fecsPerSM[ism]) {
                  messagebuilder << "Pedestals not set SM " << ism << " " << mIndicesConverter.GetOnlineSMIndex(ism) << " FEC " << fec << std::endl;
                }
              }
            }
            message = messagebuilder.str();
          }
          result.addFlag(o2::quality_control::FlagTypeFactory::BadEMCalorimetry(), message);
        }
      }
    } else if (mo->getObject()->InheritsFrom(TH1::Class())) {
      auto hist = static_cast<TH1*>(mo->getObject());
      if (!hist->GetEntries()) {
        result = Quality::Medium;
      } else {
        auto [minentries, siganlfraction] = getCutsBunchMinAmpHist(mo->getName().find("SM") != std::string::npos, false);
        result = runPedestalCheck1D(hist, mBunchMinCheckMinEntries, mBunchMinCheckFractionSignal);
      }
    }
  }
  if (mo->getName() == "FECidMaxChWithInput_perSM") {
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
          if (countrate > mean + mNsigmaFECMaxPayload * sigma) {
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
  if (mo->getName().find("PayloadSize") != std::string::npos && mo->getName().find("1D") == std::string::npos) {
    // 2D checkers for payload size
    std::map<int, double> meanPayloadSizeDDL;
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
        if (payloadmean > mean + mNsigmaPayloadSize * sigma) {
          result = Quality::Bad;
          break;
        }
      }
    }
  }
  if (mo->getName().find("PayloadSize") != std::string::npos && mo->getName().find("1D") != std::string::npos) {
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
        if (countrate > mean + mNsigmaPayloadSize * sigma) {
          result = Quality::Bad;
        }
      }
    }
  }
  return result;
}

void RawCheck::beautify(std::shared_ptr<MonitorObject> mo, Quality checkResult)
{
  if (mo->getName().find("Error") != std::string::npos) {
    auto* h = dynamic_cast<TH1*>(mo->getObject());

    if (checkResult == Quality::Good) {
      // check the error type, loop on X entries to find the SM and on Y to find the Error Number
      TLatex* msg = new TLatex(0.2, 0.8, "#color[418]{No Error: OK}");
      msg->SetNDC();
      msg->SetTextSize(16);
      msg->SetTextFont(43);
      h->GetListOfFunctions()->Add(msg);
      msg->Draw();
    } else if (checkResult == Quality::Bad) {
      if (mILMessageRawErrorCheck) {
        ILOG(Error, Support) << " QualityBad:Presence of Error Code" << ENDM;
      }
      TLatex* msg = new TLatex(0.2, 0.8, "#color[2]{Presence of Error Code: call EMCAL oncall}");
      msg->SetNDC();
      msg->SetTextSize(16);
      msg->SetTextFont(43);
      h->GetListOfFunctions()->Add(msg);
      msg->Draw();
      h->SetFillColor(kRed);
    } else if (checkResult == Quality::Medium) {
      ILOG(Info, Support) << "Quality::medium, setting to orange" << ENDM;
      h->SetFillColor(kOrange);
    }
    h->SetLineColor(kBlack);
  }
  if (mo->getName().find("BunchMinRawAmplitude") != std::string::npos) {
    if (mo->getObject()->InheritsFrom(TH2::Class())) {
      auto* h = dynamic_cast<TH2*>(mo->getObject());
      if (checkResult == Quality::Good) {
        TLatex* msg = new TLatex(0.3, 0.8, "#color[418]{Data OK}");
        msg->SetNDC();
        msg->SetTextSize(16);
        msg->SetTextFont(43);
        h->GetListOfFunctions()->Add(msg);
        msg->Draw();
        h->SetFillColor(kGreen);
      } else {
        auto flags = checkResult.getFlags();
        auto emflag = std::find_if(flags.begin(), flags.end(), [](const std::pair<quality_control::FlagType, std::string>& testflag) -> bool {
          bool found = testflag.first == quality_control::FlagTypeFactory::BadEMCalorimetry();
          return testflag.first == quality_control::FlagTypeFactory::BadEMCalorimetry();
        });
        if (emflag != flags.end()) {
          auto message = emflag->second;
          std::stringstream parser(message);
          std::string buffer;
          double currenty = 0.8;
          bool hasSM = false;
          int ilines = 0; // Display only the 5 first messages
          while (std::getline(parser, buffer, '\n')) {
            ilines++;
            bool isFEC = buffer.find("FEC") != std::string::npos;
            if (!isFEC) {
              hasSM = true;
            }
            if (ilines > 5) {
              continue;
            }
            int textcolor = isFEC ? 42 : 2;
            TLatex* msg = new TLatex(0.3, currenty, Form("#color[%d]{%s}", textcolor, buffer.data()));
            msg->SetNDC();
            msg->SetTextSize(16);
            msg->SetTextFont(43);
            h->GetListOfFunctions()->Add(msg);
            msg->Draw();
            currenty -= 0.1;
          }
          int textcolor = hasSM ? 2 : 42;
          TLatex* msg = new TLatex(0.3, currenty, Form("#color[%d]{If NOT techn.run: call EMCAL oncall}", textcolor));
          msg->SetNDC();
          msg->SetTextSize(16);
          msg->SetTextFont(43);
          h->GetListOfFunctions()->Add(msg);
          msg->Draw();
          currenty -= 0.1;
        }
      }
    } else {
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
        bool showMessage = false;
        if (mIlMessageBunchMinAmpCheckSM && (mo->getName().find("SM") != std::string::npos)) {
          showMessage = true;
        } else if (mIlMessageBunchMinAmpCheckDetector && (mo->getName().find("EMCAL") != std::string::npos || mo->getName().find("DCAL") != std::string::npos)) {
          showMessage = true;
        } else if (mIlMessageBunchMinAmpCheckFull && (mo->getName().find("Full") != std::string::npos)) {
          showMessage = true;
        }
        if (showMessage) {
          ILOG(Error, Support) << " QualityBad:Bunch min Amplitude outside limits (" << mo->getName() << ")" << ENDM;
        }
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
        ILOG(Info, Support) << "Quality::medium, setting to orange" << ENDM;
        TLatex* msg = new TLatex(0.2, 0.8, "#color[42]{empty:if in run, call EMCAL oncall}");
        msg->SetNDC();
        msg->SetTextSize(16);
        msg->SetTextFont(43);
        h->GetListOfFunctions()->Add(msg);
        msg->Draw();
      }
      h->SetLineColor(kBlack);
    }
  }
  std::vector<std::string> payloadhists = { "FECidMaxChWithInput_perSM", "PayloadSizePerDDL", "PayloadSizeTFPerDDL", "PayloadSizeTFPerDDL_1D", "PayloadSizePerDDL_1D" };
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
      TLatex* msg;
      if (mo->getName() == "FECidMaxChWithInput_perSM") {
        if (mILMessageNoisyFECCheck) {
          ILOG(Error, Support) << "Noisy FEC detected" << ENDM;
        }
        msg = new TLatex(0.2, 0.8, "#color[2]{Noisy FEC detected}");
      }
      if (mo->getName().find("PayloadSize") != std::string::npos) {
        if (mILMessagePayloadSizeCheck) {
          ILOG(Error, Support) << "Large payload in several DDLs" << ENDM;
        }
        msg = new TLatex(0.2, 0.8, "#color[2]{Large payload in several DDLs}");
      }
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
      ILOG(Info, Support) << "Quality::medium, setting to orange" << ENDM;
      TLatex* msg = new TLatex(0.2, 0.8, "#color[42]{empty:if in run, call EMCAL-oncall}");
      msg->SetNDC();
      msg->SetTextSize(16);
      msg->SetTextFont(43);
      h->GetListOfFunctions()->Add(msg);
      msg->Draw();
    }
    h->SetLineColor(kBlack);
  }
  if (mo->getName().find("ADC_EMCAL") != std::string::npos) {
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

    // vertical
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

Quality RawCheck::runPedestalCheck1D(TH1* adchist, double minentries, double signalfraction) const
{
  // checker for the raw ampl histos (second peak around 40)
  Quality result = Quality::Good;
  Float_t totentries = adchist->Integral(10, 100); // Exclude dominant part of the channels for which the signal is in the noise range below pedestal
  if (totentries > minentries) {                   // Do not check if we have only a few counts in the signal region

    Float_t entriesBadRegion = adchist->Integral(20, 60);
    int first = adchist->GetXaxis()->GetFirst(),
        last = adchist->GetXaxis()->GetLast();
    adchist->GetXaxis()->SetRangeUser(20, 60);
    Int_t maxbin = adchist->GetMaximumBin();
    adchist->GetXaxis()->SetRange(first, last);
    if (maxbin > 20 && maxbin < 50) {
      // Float_t entries = h->GetBinContent(maxbin + 1);
      if (entriesBadRegion > signalfraction * totentries) {
        result = Quality::Bad;
      }
    }
  }
  return result;
}

std::vector<Quality> RawCheck::runPedestalCheck2D(TH2* adchist, double minentries, double signalfraction) const
{
  std::vector<Quality> result;
  for (int ib = 0; ib < adchist->GetYaxis()->GetNbins(); ib++) {
    std::unique_ptr<TH1> projectionArea(adchist->ProjectionX("projectionModule", ib + 1, ib + 1));
    if (projectionArea->GetEntries()) {
      auto quality = runPedestalCheck1D(projectionArea.get(), minentries, signalfraction);
      result.emplace_back(quality);
    } else {
      result.emplace_back(Quality::Good);
    }
  }
  return result;
}

std::tuple<int, double> RawCheck::getCutsBunchMinAmpHist(bool perSM, bool perFEC)
{
  int minentries = mBunchMinCheckMinEntries;
  double signalfraction = mBunchMinCheckFractionSignal;
  if (perSM) {
    if (mBunchMinCheckMinEntriesSM) {
      minentries = mBunchMinCheckMinEntriesSM;
    }
    if (mBunchMinCheckFractionSignalSM > DBL_EPSILON) {
      signalfraction = mBunchMinCheckFractionSignalSM;
    }
  } else if (perFEC) {
    if (mBunchMinCheckMinEntriesFEC) {
      minentries = mBunchMinCheckMinEntriesFEC;
    }
    if (mBunchMinCheckFractionSignalFEC > DBL_EPSILON) {
      signalfraction = mBunchMinCheckFractionSignalFEC;
    }
  }
  return std::make_tuple(minentries, signalfraction);
}

std::array<std::vector<int>, 20> RawCheck::reduceBadFECs(std::vector<int> fecids) const
{
  std::array<std::vector<int>, 20> smfecs;
  for (auto fec : fecids) {
    int smID = fec / 40,
        fecSM = fec % 40;
    smfecs[smID].emplace_back(fecSM);
  }
  return smfecs;
}

int RawCheck::getExepctedFECs(int smID) const
{
  if (smID < 10) {
    // EMCAL full
    return 36; // 40 - TRU fecs
  } else if (smID >= 12 && smID < 18) {
    // DCAL 2/3
    return 24;
  } else {
    // small SMs
    return 12;
  }
}

void RawCheck::loadConfigValueInt(const std::string_view configvalue, int& target)
{
  auto found = mCustomParameters.find(configvalue.data());
  if (found != mCustomParameters.end()) {
    try {
      target = std::stoi(found->second.data());
    } catch (std::exception& e) {
      ILOG(Error, Support) << "Value " << found->second << " not of expected type (int)" << ENDM;
    }
  }
}

void RawCheck::loadConfigValueDouble(const std::string_view configvalue, double& target)
{
  auto found = mCustomParameters.find(configvalue.data());
  if (found != mCustomParameters.end()) {
    try {
      target = std::stod(found->second);
    } catch (std::exception& e) {
      ILOG(Error, Support) << "Value " << found->second << " not of expected type (double)" << ENDM;
    }
  }
}

void RawCheck::loadConfigValueBool(const std::string_view configvalue, bool& target)
{
  auto found = mCustomParameters.find(configvalue.data());
  if (found != mCustomParameters.end()) {
    try {
      mIlMessageBunchMinAmpCheckSM = decodeBool(found->second);
    } catch (std::exception& e) {
      ILOG(Error, Support) << e.what() << ENDM;
    }
  }
}

} // namespace o2::quality_control_modules::emcal
