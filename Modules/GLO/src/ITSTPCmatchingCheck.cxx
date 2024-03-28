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

#include <Rtypes.h>
#include <TH1.h>
#include <TEfficiency.h>
#include <TPaveText.h>
#include <TArrow.h>
#include <TText.h>
#include <TBox.h>
#include <TLine.h>
#include <TLegend.h>
#include <TFile.h>
#include <TPolyMarker.h>

#include <DataFormatsQualityControl/FlagReasons.h>
#include "fmt/format.h"

using namespace std;
using namespace o2::quality_control;

namespace o2::quality_control_modules::glo
{

void ITSTPCmatchingCheck::configure()
{
  if (auto param = mCustomParameters.find("minPt"); param != mCustomParameters.end()) {
    try {
      mMinPt = stof(param->second);
    } catch (const std::exception& e) {
      ILOG(Error) << "Cannot convert param minPt '" << e.what() << "'; using default...";
    }
  }

  if (auto param = mCustomParameters.find("maxPt"); param != mCustomParameters.end()) {
    try {
      mMaxPt = stof(param->second);
    } catch (const std::exception& e) {
      ILOG(Error) << "Cannot convert param maxPt '" << e.what() << "'; using default...";
    }
  }

  if (auto param = mCustomParameters.find("threshold"); param != mCustomParameters.end()) {
    try {
      mThreshold = stof(param->second);
    } catch (const std::exception& e) {
      ILOG(Error) << "Cannot convert param threshold '" << e.what() << "'; using default...";
    }
  }
}

Quality ITSTPCmatchingCheck::check(std::map<std::string, std::shared_ptr<MonitorObject>>* moMap)
{
  Quality result = Quality::Null;

  for (auto& [moName, mo] : *moMap) {
    (void)moName;

    if (mo->getName() == "mFractionITSTPCmatch_ITS") {
      auto* eff = dynamic_cast<TEfficiency*>(mo->getObject());
      auto binLow = eff->FindFixBin(mMinPt), binUp = eff->FindFixBin(mMaxPt);

      result = Quality::Good;

      if (eff->GetTotalHistogram()->GetEntries() == 0) {
        result = Quality::Bad;
        result.addReason(FlagReasonFactory::Unknown(), "No ITS tracks in denumerator!");
      } else if (eff->GetPassedHistogram()->GetEntries() == 0) {
        result = Quality::Bad;
        result.addReason(FlagReasonFactory::Unknown(), "No ITS-TPC tracks in numerator!");
      } else {
        std::vector<int> badBins;
        for (int iBin = binLow; iBin <= binUp; ++iBin) {
          if (eff->GetEfficiency(iBin) < mThreshold) {
            result = Quality::Bad;
            badBins.push_back(iBin);
          }
        }
        auto ranges = findRanges(badBins);
        if (result == Quality::Bad) {
          result.addReason(FlagReasonFactory::BadTracking(), "Check vdrift online calibration and ITS/TPC QC");

          std::string cBins{ "Bad matching efficiency in: " };
          for (const auto& [binLow, binUp] : ranges) {
            float low = eff->GetPassedHistogram()->GetXaxis()->GetBinLowEdge(binLow), up = eff->GetPassedHistogram()->GetXaxis()->GetBinUpEdge(binUp);
            cBins += fmt::format("{:.1f}-{:.1f},", low, up);
          }
          cBins.pop_back(); // remove last `,`
          cBins += " (GeV/c)";
          result.addReason(FlagReasonFactory::BadTracking(), cBins);
        }
      }
    }
  }
  return result;
}

std::string ITSTPCmatchingCheck::getAcceptedType() { return "TH1"; }

void ITSTPCmatchingCheck::beautify(std::shared_ptr<MonitorObject> mo, Quality checkResult)
{
  if (mo->getName() == "mFractionITSTPCmatch_ITS") {
    auto* eff = dynamic_cast<TEfficiency*>(mo->getObject());
    // Draw threshold lines
    float xmin = eff->GetPassedHistogram()->GetXaxis()->GetXmin();
    float xmax = eff->GetPassedHistogram()->GetXaxis()->GetXmax();
    float xminT = mMinPt;
    float xmaxT = mMaxPt;
    auto* l1 = new TLine(xmin, mThreshold, xmax, mThreshold);
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
    auto* tt = new TText(xminT + 0.1, 1.05, "Checked Range");
    tt->SetTextSize(0.04);
    auto* ttt = new TText(xmaxT + 0.9, mThreshold + 0.01, "Threshold");
    auto* aa = new TArrow(xminT + 0.01, 1.02, xmaxT - 0.01, 1.02, 0.02, "<|>");
    if (checkResult == Quality::Good) {
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
    int cG{ 0 }, cB{ 0 };
    auto* good = new TPolyMarker();
    good->SetMarkerColor(kGreen);
    good->SetMarkerStyle(25);
    good->SetMarkerSize(2);
    auto* bad = new TPolyMarker();
    bad->SetMarkerColor(kRed);
    bad->SetMarkerStyle(25);
    bad->SetMarkerSize(2);
    auto* leg = new TLegend(0.7, 0.13, 0.89, 0.3);
    leg->SetNColumns(2);
    leg->SetHeader("Threshold Checks");
    leg->AddEntry(good, "Good", "P");
    leg->AddEntry(bad, "Bad", "P");
    auto binLow = eff->FindFixBin(mMinPt), binUp = eff->FindFixBin(mMaxPt);
    for (int iBin = binLow; iBin <= binUp; ++iBin) {
      auto x = eff->GetPassedHistogram()->GetBinCenter(iBin);
      auto y = eff->GetEfficiency(iBin);
      if (eff->GetEfficiency(iBin) < mThreshold) {
        bad->SetPoint(++cB, x, y);
      } else {
        good->SetPoint(++cG, x, y);
      }
    }
    eff->GetListOfFunctions()->Add(good);
    eff->GetListOfFunctions()->Add(bad);
    eff->GetListOfFunctions()->Add(leg);

    // Quality
    auto* msg = new TPaveText(0.12, 0.12, 0.5, 0.3, "NDC;NB");
    msg->AddText(Form("Quality: %s", checkResult.getName().c_str()));
    if (checkResult == Quality::Good) {
      msg->SetFillColor(kGreen);
    } else {
      msg->SetFillColor(kRed);
      for (const auto& [_, desc] : checkResult.getReasons()) {
        msg->AddText(Form("%s", desc.c_str()));
      }
      msg->SetTextColor(kWhite);
    }
    eff->GetListOfFunctions()->Add(msg);

    if constexpr (false) {
      TFile f("test.root", "RECREATE");
      f.WriteObject(eff, "qcplot");
    }
  }
}

void ITSTPCmatchingCheck::reset()
{
  ILOG(Debug, Devel) << "ITSTPCmatchingCheck::reset" << ENDM;
}

void ITSTPCmatchingCheck::startOfActivity(const Activity& activity)
{
  ILOG(Debug, Devel) << "ITSTPCmatchingCheck::start : " << activity.mId << ENDM;
  mActivity = make_shared<Activity>(activity);
}

void ITSTPCmatchingCheck::endOfActivity(const Activity& activity)
{
  ILOG(Debug, Devel) << "ITSTPCmatchingCheck::end : " << activity.mId << ENDM;
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
