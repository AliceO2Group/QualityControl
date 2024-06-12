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
/// \file   ITSTPCmatchingCheck.cxx
/// \author felix.schlepper@cern.ch
///

#include "GLO/ITSTPCmatchingCheck.h"
#include "QualityControl/MonitorObject.h"
#include "QualityControl/Quality.h"
#include "QualityControl/QcInfoLogger.h"
#include "Common/Utils.h"

#include <Rtypes.h>
#include <TH1.h>
#include <TPaveText.h>
#include <TMath.h>
#include <TArrow.h>
#include <TText.h>
#include <TBox.h>
#include <TLine.h>
#include <TLegend.h>
#include <TFile.h>
#include <TPolyMarker.h>

#include <DataFormatsQualityControl/FlagType.h>
#include <DataFormatsQualityControl/FlagTypeFactory.h>

#include <utility>
#include <type_traits>
#include "fmt/format.h"

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

  for (auto& [moName, mo] : *moMap) {
    (void)moName;

    if (mShowPt && mo->getName() == "mFractionITSTPCmatch_ITS_Hist") {
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

    if (mShowPhi && mo->getName() == "mFractionITSTPCmatchPhi_ITS_Hist") {
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

    if (mShowEta && mo->getName() == "mFractionITSTPCmatchEta_ITS_Hist") {
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

  if (mShowPt && mo->getName() == "mFractionITSTPCmatch_ITS_Hist") {
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
    good->SetMarkerSize(2);
    auto* bad = new TPolyMarker();
    bad->SetMarkerColor(kRed);
    bad->SetMarkerStyle(25);
    bad->SetMarkerSize(2);
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
  }

  if (mShowPhi && mo->getName() == "mFractionITSTPCmatchPhi_ITS_Hist") {
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
  }

  if (mShowEta && mo->getName() == "mFractionITSTPCmatchEta_ITS_Hist") {
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
  }
}

void ITSTPCmatchingCheck::startOfActivity(const Activity& activity)
{
  mActivity = make_shared<Activity>(activity);

  mShowPt = common::getFromExtendedConfig(activity, mCustomParameters, "showPt", true);
  if (mShowPt) {
    mThresholdPt = common::getFromExtendedConfig(activity, mCustomParameters, "thresholdPt", 0.5f);
    mMinPt = common::getFromExtendedConfig(activity, mCustomParameters, "minPt", 1.0f);
    mMaxPt = common::getFromExtendedConfig(activity, mCustomParameters, "maxPt", 1.999f);
  }

  mShowPhi = common::getFromExtendedConfig(activity, mCustomParameters, "showPhi", true);
  if (mShowPt) {
    mThresholdPhi = common::getFromExtendedConfig(activity, mCustomParameters, "thresholdPhi", 0.3f);
  }

  mShowEta = common::getFromExtendedConfig(activity, mCustomParameters, "showEta", false);
  if (mShowEta) {
    mThresholdEta = common::getFromExtendedConfig(activity, mCustomParameters, "thresholdEta", 0.4f);
    mMinEta = common::getFromExtendedConfig(activity, mCustomParameters, "minEta", -0.8f);
    mMaxEta = common::getFromExtendedConfig(activity, mCustomParameters, "maxEta", 0.8f);
  }
}

std::vector<std::pair<int, int>> ITSTPCmatchingCheck::findRanges(const std::vector<int>& nums)
{
  std::vector<std::pair<int, int>> ranges;
  if (nums.empty())
    return ranges;

  int start = nums[0];
  int end = start;

  for (size_t i = 1; i < nums.size(); ++i) {
    if (nums[i] == end + 1) {
      end = nums[i];
    } else {
      ranges.push_back(std::make_pair(start, end));
      start = end = nums[i];
    }
  }
  ranges.push_back(std::make_pair(start, end)); // Add the last range
  return ranges;
}

} // namespace o2::quality_control_modules::glo
