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

/// \file   ITSTPCmatchingCheck.cxx
/// \author felix.schlepper@cern.ch

#include "GLO/ITSTPCmatchingCheck.h"
#include "QualityControl/MonitorObject.h"
#include "QualityControl/Quality.h"
#include <DataFormatsQualityControl/FlagType.h>
#include <DataFormatsQualityControl/FlagTypeFactory.h>
#include "QualityControl/QcInfoLogger.h"
#include "Common/Utils.h"

#include "Rtypes.h"
#include "TH1.h"
#include "TF1.h"
#include "TPaveText.h"
#include "TMath.h"
#include "TArrow.h"
#include "TText.h"
#include "TLine.h"
#include "TLegend.h"
#include "TPolyMarker.h"

#include "fmt/format.h"
#include <utility>

using namespace std;
using namespace o2::quality_control;

namespace o2::quality_control_modules::glo
{

Quality ITSTPCmatchingCheck::check(std::map<std::string, std::shared_ptr<MonitorObject>>* moMap)
{
  Quality result = Quality::Null;
  Quality ptQual = Quality::Good;
  Quality phiQual = Quality::Good;
  Quality etaQual = Quality::Good;

  for (auto& [_, mo] : *moMap) {
    const std::string moName = mo->GetName();

    if (mShowPt && moName == "mFractionITSTPCmatch_ITS_Hist") {
      auto* eff = dynamic_cast<TH1*>(mo->getObject());
      if (eff == nullptr) {
        ILOG(Error) << "Failed cast for ITSTPCmatch_ITS check!" << ENDM;
        continue;
      }
      auto binLow = eff->FindFixBin(mMinPt), binUp = eff->FindFixBin(mMaxPt);

      ptQual = Quality::Good;
      result.addMetadata("checkPtQuality", "good");
      result.addMetadata("checkPtBins", "");
      result.addMetadata("checkPt", "good");
      if (eff->GetEntries() == 0) {
        ptQual = Quality::Null;
        result.updateMetadata("checkPt", "null");
      } else {
        std::vector<int> badBins;
        for (int iBin = binLow; iBin <= binUp; ++iBin) {
          if (eff->GetBinContent(iBin) < mThresholdPt) {
            ptQual = Quality::Bad;
            badBins.push_back(iBin);
          }
        }
        auto ranges = findRanges(badBins);
        if (ptQual == Quality::Bad) {
          result.addFlag(FlagTypeFactory::BadTracking(), "Check vdrift online calibration and ITS/TPC QC");
          result.updateMetadata("checkPtQuality", "bad");

          std::string cBins{ "Bad matching efficiency in: " };
          if (mLimitRange >= 0 && ranges.size() > mLimitRange) {
            result.updateMetadata("checkPtBins", "too many bad bins");
          } else {
            for (const auto& [binLow, binUp] : ranges) {
              float low = eff->GetXaxis()->GetBinLowEdge(binLow), up = eff->GetXaxis()->GetBinUpEdge(binUp);
              cBins += fmt::format("{:.1f}-{:.1f},", low, up);
            }
            cBins.pop_back(); // remove last `,`
            cBins += " (GeV/c)";
            result.updateMetadata("checkPtBins", cBins);
          }
        }
      }
    } else if (mShowPhi && moName == "mFractionITSTPCmatchPhi_ITS_Hist") {
      auto* eff = dynamic_cast<TH1*>(mo->getObject());
      if (eff == nullptr) {
        ILOG(Error) << "Failed cast for ITSTPCmatchPhi_ITS check!" << ENDM;
        continue;
      }

      phiQual = Quality::Good;
      result.addMetadata("checkPhiQuality", "good");
      result.addMetadata("checkPhiBins", "");
      result.addMetadata("checkPhi", "good");
      if (eff->GetEntries() == 0) {
        phiQual = Quality::Null;
        result.updateMetadata("checkPhi", "null");
      } else {
        std::vector<int> badBins;
        for (int iBin{ 1 }; iBin <= eff->GetNbinsX(); ++iBin) {
          if (eff->GetBinContent(iBin) < mThresholdPhi) {
            phiQual = Quality::Bad;
            badBins.push_back(iBin);
          }
        }
        auto ranges = findRanges(badBins);
        if (phiQual == Quality::Bad) {
          result.addFlag(FlagTypeFactory::BadTracking(), "Check TPC sectors or ITS staves QC!");
          result.updateMetadata("checkPhiQuality", "bad");

          if (mLimitRange >= 0 && ranges.size() > mLimitRange) {
            result.updateMetadata("checkPhiBins", "too many bad bins");
          } else {
            std::string cBins{ "Bad matching efficiency in: " };
            for (const auto& [binLow, binUp] : ranges) {
              float low = eff->GetXaxis()->GetBinLowEdge(binLow), up = eff->GetXaxis()->GetBinUpEdge(binUp);
              cBins += fmt::format("{:.1f}-{:.1f},", low, up);
            }
            cBins.pop_back(); // remove last `,`
            cBins += " (rad)";
            result.updateMetadata("checkPhiBins", cBins);
          }
        }
      }
    } else if (mShowEta && moName == "mFractionITSTPCmatchEta_ITS_Hist") {
      auto* eff = dynamic_cast<TH1*>(mo->getObject());
      if (eff == nullptr) {
        ILOG(Error) << "Failed cast for ITSTPCmatchEta_ITS check!" << ENDM;
        continue;
      }
      auto binLow = eff->FindFixBin(mMinEta), binUp = eff->FindFixBin(mMaxEta);

      etaQual = Quality::Good;
      result.addMetadata("checkEtaQuality", "good");
      result.addMetadata("checkEtaBins", "");
      result.addMetadata("checkEta", "good");
      if (eff->GetEntries() == 0) {
        etaQual = Quality::Null;
        result.updateMetadata("checkEta", "null");
      } else {
        std::vector<int> badBins;
        for (int iBin = binLow; iBin <= binUp; ++iBin) {
          if (eff->GetBinContent(iBin) < mThresholdEta) {
            etaQual = Quality::Bad;
            badBins.push_back(iBin);
          }
        }
        auto ranges = findRanges(badBins);
        if (etaQual == Quality::Bad) {
          result.updateMetadata("checkEtaQuality", "bad");

          if (mLimitRange >= 0 && ranges.size() > mLimitRange) {
            result.updateMetadata("checkEtaBins", "too many bad bins");
          } else {
            std::string cBins{ "Bad matching efficiency in: " };
            for (const auto& [binLow, binUp] : ranges) {
              float low = eff->GetXaxis()->GetBinLowEdge(binLow), up = eff->GetXaxis()->GetBinUpEdge(binUp);
              cBins += fmt::format("{:.1f}-{:.1f},", low, up);
            }
            cBins.pop_back(); // remove last `,`
            result.updateMetadata("checkEtaBins", cBins);
          }
        }
      }
    }
  }

  // Overall Quality
  auto isWorse = [](Quality const& a, Quality const& b) {
    return a.isWorseThan(b) ? a : b;
  };
  // Somehow this is not accepted by the CI, although running git-clang-format locally accepts it
  // clang-format off
  auto findWorst = [&]<typename T, typename... Args>(const T& first, Args... args) {
    return isWorse(first, isWorse(args...));
  };
  // clang-format on
  result.set(findWorst(ptQual, etaQual, phiQual));

  return result;
}

void ITSTPCmatchingCheck::beautify(std::shared_ptr<MonitorObject> mo, Quality checkResult)
{
  if (mo == nullptr) {
    ILOG(Error) << "Received nullptr from upstream, returning..." << ENDM;
    return;
  }

  const auto name = mo->getName();

  if (mShowPt && name == "mFractionITSTPCmatch_ITS_Hist") {
    auto* eff = dynamic_cast<TH1*>(mo->getObject());
    if (eff == nullptr) {
      ILOG(Error) << "Failed cast for ITSTPCmatch_ITS_Hist beautify!" << ENDM;
      return;
    }
    eff->GetYaxis()->SetRangeUser(0, 1.1);
    eff->GetYaxis()->SetBit(TAxis::EStatusBits::kDecimals);

    // Draw threshold lines
    float xmin = eff->GetXaxis()->GetXmin();
    float xmax = eff->GetXaxis()->GetXmax();
    float xminT = mMinPt;
    float xmaxT = mMaxPt;
    auto* l1 = new TLine(xmin, mThresholdPt, xmax, mThresholdPt);
    l1->SetLineStyle(kDashed);
    l1->SetLineWidth(4);
    l1->SetLineColor(kCyan - 7);
    auto* l2 = new TLine(xminT, 0, xminT, 1.1);
    l2->SetLineStyle(kDashed);
    l2->SetLineWidth(4);
    l2->SetLineColor(kCyan - 7);
    auto* l3 = new TLine(xmaxT, 0, xmaxT, 1.1);
    l3->SetLineStyle(kDashed);
    l3->SetLineWidth(4);
    l3->SetLineColor(kCyan - 7);
    auto* tt = new TText(xmaxT + 0.3, 1.02, "Checked Range");
    tt->SetTextSize(0.04);
    auto* ttt = new TText(xmaxT + 0.3, mThresholdPt + 0.012, "Threshold");
    auto* aa = new TArrow(xminT + 0.01, 1.02, xmaxT - 0.01, 1.02, 0.02, "<|>");
    if (checkResult.getMetadata("checkPtQuality") == "good") {
      l1->SetLineColor(kCyan - 7);
      l2->SetLineColor(kCyan - 7);
      l3->SetLineColor(kCyan - 7);
      aa->SetFillColor(kCyan - 7);
      aa->SetLineColor(kCyan - 7);
    } else {
      l1->SetLineColor(kRed - 4);
      l2->SetLineColor(kRed - 4);
      l3->SetLineColor(kRed - 4);
      aa->SetFillColor(kRed - 4);
      aa->SetLineColor(kRed - 4);
    }
    eff->GetListOfFunctions()->Add(l1);
    eff->GetListOfFunctions()->Add(l2);
    eff->GetListOfFunctions()->Add(l3);
    eff->GetListOfFunctions()->Add(tt);
    eff->GetListOfFunctions()->Add(ttt);
    eff->GetListOfFunctions()->Add(aa);

    // Color Bins in Window
    int cG{ 0 }, cB{ 0 }, cTot{ 0 };
    auto* good = new TPolyMarker();
    good->SetMarkerColor(kGreen);
    good->SetMarkerStyle(25);
    good->SetMarkerSize(1);
    auto* bad = new TPolyMarker();
    bad->SetMarkerColor(kRed);
    bad->SetMarkerStyle(25);
    bad->SetMarkerSize(1);
    auto binLow = eff->FindFixBin(mMinPt), binUp = eff->FindFixBin(mMaxPt);
    for (int iBin = binLow; iBin <= binUp; ++iBin) {
      auto x = eff->GetBinCenter(iBin);
      auto y = eff->GetBinContent(iBin);
      if (y < mThresholdPt) {
        bad->SetPoint(cB++, x, y);
      } else {
        good->SetPoint(cG++, x, y);
      }
    }
    cTot = cG + cB;
    auto* leg = new TLegend(0.7, 0.13, 0.89, 0.3);
    leg->SetHeader("Threshold Checks");
    leg->AddEntry(good, Form("Good %d / %d", cG, cTot), "P");
    leg->AddEntry(bad, Form("Bad %d / %d", cB, cTot), "P");
    eff->GetListOfFunctions()->Add(good);
    eff->GetListOfFunctions()->Add(bad);
    eff->GetListOfFunctions()->Add(leg);

    // Quality
    auto* msg = new TPaveText(0.12, 0.12, 0.5, 0.3, "NDC;NB");
    if (checkResult.getMetadata("checkPtQuality") == "good") {
      msg->AddText("Quality: Good");
      msg->SetFillColor(kGreen);
    } else if (checkResult.getMetadata("checkPtQuality") == "bad") {
      msg->AddText("Quality: Bad");
      msg->SetFillColor(kRed);
      msg->AddText("Check vdrift online calibration and ITS/TPC QC");
      msg->AddText(checkResult.getMetadata("checkPtBins").c_str());
      msg->SetTextColor(kWhite);
    } else if (checkResult.getMetadata("checkPtQuality") == "null") {
      msg->SetFillColor(kWhite);
      msg->AddText("Quality: Null");
      msg->AddText("No ITS tracks in denominator!");
      msg->AddText(checkResult.getMetadata("checkPtBins").c_str());
      msg->SetTextColor(kBlack);
    } else {
      msg->AddText("Quality: Undefined");
      msg->AddText("Not-handled Quality flag, don't panic...");
    }
    eff->GetListOfFunctions()->Add(msg);
  } else if (mShowPhi && name == "mFractionITSTPCmatchPhi_ITS_Hist") {
    auto* eff = dynamic_cast<TH1*>(mo->getObject());
    if (eff == nullptr) {
      ILOG(Error) << "Failed cast for ITSTPCmatchPhi_ITS_Hist beautify!" << ENDM;
      return;
    }
    eff->GetYaxis()->SetRangeUser(0, 1.1);
    eff->GetYaxis()->SetBit(TAxis::EStatusBits::kDecimals);

    // Draw threshold lines
    auto* l1 = new TLine(0, mThresholdPhi, 2 * TMath::Pi(), mThresholdPhi);
    l1->SetLineStyle(kDashed);
    l1->SetLineWidth(4);
    l1->SetLineColor(kCyan - 7);
    auto* tt = new TText(TMath::Pi() - 1., 1.05, "Checked Range");
    tt->SetTextSize(0.04);
    auto* ttt = new TText(2 * TMath::Pi() - 2, mThresholdPhi + 0.012, "Threshold");
    auto* aa = new TArrow(0.01, 1.02, 2 * TMath::Pi() - 0.01, 1.02, 0.02, "<|>");
    if (checkResult.getMetadata("checkPhiQuality") == "good") {
      l1->SetLineColor(kCyan - 7);
      aa->SetFillColor(kCyan - 7);
      aa->SetLineColor(kCyan - 7);
    } else {
      l1->SetLineColor(kRed - 4);
      aa->SetFillColor(kRed - 4);
      aa->SetLineColor(kRed - 4);
    }
    eff->GetListOfFunctions()->Add(l1);
    eff->GetListOfFunctions()->Add(tt);
    eff->GetListOfFunctions()->Add(ttt);
    eff->GetListOfFunctions()->Add(aa);

    // Color Bins in Window
    int cG{ 0 }, cB{ 0 }, cTot{ 0 };
    auto* good = new TPolyMarker();
    good->SetMarkerColor(kGreen);
    good->SetMarkerStyle(25);
    good->SetMarkerSize(1);
    auto* bad = new TPolyMarker();
    bad->SetMarkerColor(kRed);
    bad->SetMarkerStyle(25);
    bad->SetMarkerSize(1);
    for (int iBin{ 1 }; iBin <= eff->GetNbinsX(); ++iBin) {
      auto x = eff->GetBinCenter(iBin);
      auto y = eff->GetBinContent(iBin);
      if (y < mThresholdPhi) {
        bad->SetPoint(cB++, x, y);
      } else {
        good->SetPoint(cG++, x, y);
      }
    }
    cTot = cG + cB;
    auto* leg = new TLegend(0.7, 0.13, 0.89, 0.3);
    leg->SetHeader("Threshold Checks");
    leg->AddEntry(good, Form("Good %d / %d", cG, cTot), "P");
    leg->AddEntry(bad, Form("Bad %d / %d", cB, cTot), "P");
    eff->GetListOfFunctions()->Add(good);
    eff->GetListOfFunctions()->Add(bad);
    eff->GetListOfFunctions()->Add(leg);

    // Quality
    auto* msg = new TPaveText(0.12, 0.12, 0.5, 0.3, "NDC;NB");
    if (checkResult.getMetadata("checkPhiQuality") == "good") {
      msg->AddText("Quality: Good");
      msg->SetFillColor(kGreen);
    } else if (checkResult.getMetadata("checkPhiQuality") == "bad") {
      msg->AddText("Quality: Bad");
      msg->SetFillColor(kRed);
      msg->AddText("Check TPC sector or ITS staves!");
      msg->AddText(checkResult.getMetadata("checkPhiBins").c_str());
      msg->SetTextColor(kWhite);
    } else if (checkResult.getMetadata("checkPhiQuality") == "null") {
      msg->SetFillColor(kWhite);
      msg->AddText("Quality: Null");
      msg->AddText("No ITS tracks in denominator!");
      msg->SetTextColor(kBlack);
    } else {
      msg->AddText("Quality: Undefined");
      msg->AddText("Not-handled Quality flag, don't panic...");
    }
    eff->GetListOfFunctions()->Add(msg);
  } else if (mShowEta && name == "mFractionITSTPCmatchEta_ITS_Hist") {
    auto* eff = dynamic_cast<TH1*>(mo->getObject());
    if (eff == nullptr) {
      ILOG(Error) << "Failed cast for ITSTPCmatchEta_ITS_Hist beautify!" << ENDM;
      return;
    }
    eff->GetYaxis()->SetRangeUser(0, 1.1);
    eff->GetYaxis()->SetBit(TAxis::EStatusBits::kDecimals);

    // Draw threshold lines
    float xmin = eff->GetXaxis()->GetXmin();
    float xmax = eff->GetXaxis()->GetXmax();
    float xminT = mMinEta;
    float xmaxT = mMaxEta;
    auto* l1 = new TLine(xmin, mThresholdEta, xmax, mThresholdEta);
    l1->SetLineStyle(kDashed);
    l1->SetLineWidth(4);
    l1->SetLineColor(kCyan - 7);
    auto* l2 = new TLine(xminT - 0.02, 0, xminT - 0.02, 1.1);
    l2->SetLineStyle(kDashed);
    l2->SetLineWidth(4);
    l2->SetLineColor(kCyan - 7);
    auto* l3 = new TLine(xmaxT + 0.02, 0, xmaxT + 0.02, 1.1);
    l3->SetLineStyle(kDashed);
    l3->SetLineWidth(4);
    l3->SetLineColor(kCyan - 7);
    auto* tt = new TText(xminT + 0.1, 1.05, "Checked Range");
    tt->SetTextSize(0.04);
    auto* ttt = new TText(xmaxT + 0.2, mThresholdEta + 0.012, "Threshold");
    auto* aa = new TArrow(xminT + 0.01, 1.02, xmaxT - 0.01, 1.02, 0.02, "<|>");
    if (checkResult.getMetadata("checkEtaQuality") == "good") {
      l1->SetLineColor(kCyan - 7);
      l2->SetLineColor(kCyan - 7);
      l3->SetLineColor(kCyan - 7);
      aa->SetFillColor(kCyan - 7);
      aa->SetLineColor(kCyan - 7);
    } else {
      l1->SetLineColor(kRed - 4);
      l2->SetLineColor(kRed - 4);
      l3->SetLineColor(kRed - 4);
      aa->SetFillColor(kRed - 4);
      aa->SetLineColor(kRed - 4);
    }
    eff->GetListOfFunctions()->Add(l1);
    eff->GetListOfFunctions()->Add(l2);
    eff->GetListOfFunctions()->Add(l3);
    eff->GetListOfFunctions()->Add(tt);
    eff->GetListOfFunctions()->Add(ttt);
    eff->GetListOfFunctions()->Add(aa);

    // Color Bins in Window
    int cG{ 0 }, cB{ 0 }, cTot{ 0 };
    auto* good = new TPolyMarker();
    good->SetMarkerColor(kGreen);
    good->SetMarkerStyle(25);
    good->SetMarkerSize(1);
    auto* bad = new TPolyMarker();
    bad->SetMarkerColor(kRed);
    bad->SetMarkerStyle(25);
    bad->SetMarkerSize(1);
    auto binLow = eff->FindFixBin(mMinEta), binUp = eff->FindFixBin(mMaxEta);
    for (int iBin = binLow; iBin <= binUp; ++iBin) {
      auto x = eff->GetBinCenter(iBin);
      auto y = eff->GetBinContent(iBin);
      if (y < mThresholdEta) {
        bad->SetPoint(cB++, x, y);
      } else {
        good->SetPoint(cG++, x, y);
      }
    }
    cTot = cG + cB;
    auto* leg = new TLegend(0.7, 0.13, 0.89, 0.3);
    leg->SetHeader("Threshold Checks");
    leg->AddEntry(good, Form("Good %d / %d", cG, cTot), "P");
    leg->AddEntry(bad, Form("Bad %d / %d", cB, cTot), "P");
    eff->GetListOfFunctions()->Add(good);
    eff->GetListOfFunctions()->Add(bad);
    eff->GetListOfFunctions()->Add(leg);

    // Quality
    auto* msg = new TPaveText(0.12, 0.12, 0.5, 0.3, "NDC;NB");
    if (checkResult.getMetadata("checkEtaQuality") == "good") {
      msg->AddText("Quality: Good");
      msg->SetFillColor(kGreen);
    } else if (checkResult.getMetadata("checkEtaQuality") == "bad") {
      msg->AddText("Quality: Bad");
      msg->SetFillColor(kRed);
      msg->AddText(checkResult.getMetadata("checkEtaBins").c_str());
      msg->SetTextColor(kWhite);
    } else if (checkResult.getMetadata("checkEtaQuality") == "null") {
      msg->SetFillColor(kWhite);
      msg->AddText("Quality: Null");
      msg->AddText("No ITS tracks in denominator!");
      msg->SetTextColor(kBlack);
    } else {
      msg->AddText("Quality: Undefined");
      msg->AddText("Not-handled Quality flag, don't panic...");
    }
    eff->GetListOfFunctions()->Add(msg);
  } else if (mShowK0s && (name == "mK0sMassVsPtVsOcc_Cycle_pmass" || name == "mK0sMassVsPtVsOcc_Integral_pmass")) {
    auto* h = dynamic_cast<TH1*>(mo->getObject());
    if (!h) {
      ILOG(Error) << "Failed cast for " << name << " beautify!" << ENDM;
      return;
    }
    auto isCycle = name.find("Cycle") != std::string::npos;
    auto msg = new TPaveText(0.6, 0.6, 0.88, 0.88, "NDC;NB");
    h->SetTitle(Form("K0s invariant mass (integrated over #it{p}_{T} and occupancy, %s);K0s mass (GeV/c^{2});entries", (isCycle) ? "current cycle" : "integrated"));
    const auto fSignal = (TF1*)h->GetListOfFunctions()->FindObject("gloFitK0sMassSignal");
    if (!fSignal) {
      msg->AddText("Fit: Not Performed");
      msg->SetFillColor(kRed);
      msg->SetTextColor(kWhite);
    } else {
      auto unc = std::abs(mMassK0s - fSignal->GetParameter(4)) / fSignal->GetParameter(5);
      auto rerr = std::abs(mMassK0s - fSignal->GetParameter(4)) / mMassK0s;
      auto max = h->GetMaximum(), min = h->GetMinimum(), textp{ (max - min) * 0.1 };
      auto l = new TLine(mMassK0s, 0, mMassK0s, max);
      l->SetLineStyle(kDotted);
      h->GetListOfFunctions()->Add(l);
      auto t = new TText(mMassK0s - 0.025, textp, "PDG K0s");
      h->GetListOfFunctions()->Add(t);
      if (unc > mAccUncertainty || rerr > mAccRelError) {
        msg->AddText("Fit: BAD");
        msg->AddText("Not converged");
        msg->AddText(Form("Discrepant %.2f", unc));
        msg->AddText(Form("RError %.2f%%", rerr * 1e2));
        msg->SetFillColor(kRed);
        msg->SetTextColor(kWhite);
      } else {
        msg->AddText("Fit: GOOD");
        msg->AddText(Form("Mass %.1f #pm %.1f (MeV)", fSignal->GetParameter(4) * 1e3, fSignal->GetParameter(5) * 1e3));
        msg->AddText(Form("Consistent %.2f", unc));
        msg->AddText(Form("RError %.2f%%", rerr * 1e2));
        msg->SetFillColor(kGreen);
      }
    }
    h->GetListOfFunctions()->Add(msg);
  }
}

void ITSTPCmatchingCheck::startOfActivity(const Activity& activity)
{
  mActivity = make_shared<Activity>(activity);

  if ((mShowPt = common::getFromExtendedConfig(activity, mCustomParameters, "showPt", false))) {
    mThresholdPt = common::getFromExtendedConfig(activity, mCustomParameters, "thresholdPt", 0.5f);
    mMinPt = common::getFromExtendedConfig(activity, mCustomParameters, "minPt", 1.0f);
    mMaxPt = common::getFromExtendedConfig(activity, mCustomParameters, "maxPt", 1.999f);
  }

  if ((mShowPhi = common::getFromExtendedConfig(activity, mCustomParameters, "showPhi", false))) {
    mThresholdPhi = common::getFromExtendedConfig(activity, mCustomParameters, "thresholdPhi", 0.3f);
  }

  if ((mShowEta = common::getFromExtendedConfig(activity, mCustomParameters, "showEta", false))) {
    mThresholdEta = common::getFromExtendedConfig(activity, mCustomParameters, "thresholdEta", 0.4f);
    mMinEta = common::getFromExtendedConfig(activity, mCustomParameters, "minEta", -0.8f);
    mMaxEta = common::getFromExtendedConfig(activity, mCustomParameters, "maxEta", 0.8f);
  }

  if ((mShowK0s = common::getFromExtendedConfig(activity, mCustomParameters, "showK0s", false))) {
    mAccRelError = common::getFromExtendedConfig(activity, mCustomParameters, "acceptableK0sRError", 0.2f);
    mAccUncertainty = common::getFromExtendedConfig(activity, mCustomParameters, "acceptableK0sUncertainty", 2.f);
  }

  mLimitRange = common::getFromExtendedConfig(activity, mCustomParameters, "limitRanges", 5);
  mIsPbPb = activity.mBeamType == "Pb-Pb";
}

std::vector<std::pair<int, int>> ITSTPCmatchingCheck::findRanges(const std::vector<int>& nums) noexcept
{
  std::vector<std::pair<int, int>> ranges;
  if (nums.empty()) {
    return ranges;
  }

  int start = nums[0];
  int end = start;

  for (size_t i = 1; i < nums.size(); ++i) {
    if (nums[i] == end + 1) {
      end = nums[i];
    } else {
      ranges.emplace_back(start, end);
      start = end = nums[i];
    }
  }
  ranges.emplace_back(start, end); // Add the last range
  return ranges;
}

} // namespace o2::quality_control_modules::glo
