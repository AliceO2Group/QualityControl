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
/// \file   MatchingEfficiencyCheck.cxx
/// \author Andrea Ferrero
///

#include "MUONCommon/MatchingEfficiencyCheck.h"
#include "QualityControl/MonitorObject.h"
#include "QualityControl/Quality.h"
#include <TH1.h>
#include <TLine.h>

namespace o2::quality_control_modules::muon
{

template <>
std::string MatchingEfficiencyCheck::getParameter(std::string parName, const std::string defaultValue, const o2::quality_control::core::Activity& activity)
{
  std::string result = defaultValue;
  auto parOpt = mCustomParameters.atOptional(parName, activity);
  if (parOpt.has_value()) {
    result = parOpt.value();
  }
  return result;
}

template <>
std::string MatchingEfficiencyCheck::getParameter(std::string parName, const std::string defaultValue)
{
  std::string result = defaultValue;
  auto parOpt = mCustomParameters.atOptional(parName);
  if (parOpt.has_value()) {
    result = parOpt.value();
  }
  return result;
}

void MatchingEfficiencyCheck::configure() {}

void MatchingEfficiencyCheck::startOfActivity(const Activity& activity)
{
  mActivity = activity;
}

void MatchingEfficiencyCheck::endOfActivity(const Activity& activity)
{
  mActivity = Activity{};
  mRanges.clear();
  mIntervals.clear();
  mQualities.clear();
}

static std::string getBaseName(std::string name)
{
  auto pos = name.rfind("/");
  return ((pos < std::string::npos) ? name.substr(pos + 1) : name);
}

static std::vector<std::string> getTokens(std::string str, std::string sep)
{
  std::vector<std::string> result;
  size_t pos = 0;
  while (pos < str.size()) {
    size_t pos2 = str.find(sep, pos);
    size_t count = pos2 - pos;
    auto token = str.substr(pos, count);
    pos = (pos2 == std::string::npos) ? pos2 : pos2 + 1;
    result.push_back(token);
  }
  return result;
}

void MatchingEfficiencyCheck::initRange(std::string key)
{
  // Get acceptable range for this histogram
  auto iter = mRanges.find(key);
  if (iter != mRanges.end()) {
    return;
  }

  std::string parKey = std::string("range:") + key;
  std::string parValue = getParameter<std::string>(parKey, "", mActivity);

  auto tokens = getTokens(parValue, ":");
  if (tokens.empty()) {
    return;
  }

  auto range = getTokens(tokens[0], ",");
  if (range.size() == 2) {
    double min = std::stod(range[0]);
    double max = std::stod(range[1]);
    iter = mRanges.insert(std::pair{ key, std::make_pair(min, max) }).first;
  }

  if (tokens.size() > 1) {
    std::vector<std::pair<double, double>> intervals;
    for (size_t ti = 1; ti < tokens.size(); ti++) {
      auto interval = getTokens(tokens[ti], ",");
      if (interval.size() == 2) {
        double xmin = std::stod(interval[0]);
        double xmax = std::stod(interval[1]);
        intervals.push_back(std::make_pair(xmin, xmax));
      }
    }
    mIntervals[key] = intervals;
  }
}

std::optional<std::pair<double, double>> MatchingEfficiencyCheck::getRange(std::string key)
{
  std::optional<std::pair<double, double>> result;

  // Get acceptable range for this histogram
  auto iter = mRanges.find(key);
  if (iter != mRanges.end()) {
    result = iter->second;
  }

  return result;
}

Quality MatchingEfficiencyCheck::check(std::map<std::string, std::shared_ptr<MonitorObject>>* moMap)
{
  for (auto& [moKey, mo] : *moMap) {

    auto moName = mo->getName();

    TH1* hist = dynamic_cast<TH1*>(mo->getObject());
    if (!hist) {
      continue;
    }

    auto key = getBaseName(moName);

    initRange(key);
    auto range = getRange(key);
    if (!range) {
      continue;
    }

    // Get list of bin intervals, if any
    std::vector<std::pair<int, int>> binIntervals;
    auto iter = mIntervals.find(key);
    if (iter == mIntervals.end()) {
      // No range specified for this histogram, all bins are considered
      binIntervals.emplace_back(std::make_pair<int, int>(1, hist->GetNbinsX()));
    } else {
      // Collect all bin ranges that have to be checked for this histogram
      double epsilon = 0.001 * hist->GetXaxis()->GetBinWidth(1);
      for (auto i : iter->second) {
        int binMin = hist->GetXaxis()->FindBin(i.first + epsilon);
        int binMax = hist->GetXaxis()->FindBin(i.second - epsilon);
        binIntervals.push_back(std::make_pair(binMin, binMax));
      }
    }

    // Quality is good by default, unless the average is outside the acceptable range
    // in at least one of the bin intervals
    mQualities[moName] = Quality::Good;

    // Loop over bin intervals, compute the average bin value in each interval,
    // and compare it to the acceptable range
    // If outside, set the quality to Bad
    for (auto i : binIntervals) {
      // Set the quality to Null if the interval is invalid
      if (i.second < i.first) {
        mQualities[moName] = Quality::Null;
        break;
      }

      for (int bin = i.first; bin <= i.second; bin++) {
        if (hist->GetBinContent(bin) < range->first || hist->GetBinContent(bin) > range->second) {
          mQualities[moName] = Quality::Bad;
          break;
        }
      }
    }
  }

  Quality result = mQualities.empty() ? Quality::Null : Quality::Good;
  for (auto& [key, q] : mQualities) {
    if (q.isWorseThan(result)) {
      result = q;
    }
  }

  return result;
}

std::string MatchingEfficiencyCheck::getAcceptedType() { return "TH1"; }

void MatchingEfficiencyCheck::beautify(std::shared_ptr<MonitorObject> mo, Quality checkResult)
{
  TH1* hist = dynamic_cast<TH1*>(mo->getObject());
  if (!hist) {
    return;
  }

  hist->SetMinimum(0);
  hist->SetMaximum(1.2);

  auto moName = mo->getName();
  auto quality = mQualities[moName];
  if (quality == Quality::Bad) {
    hist->SetLineColor(kRed);
    hist->SetMarkerColor(kRed);
  }

  auto key = getBaseName(moName);
  auto range = getRange(key);
  if (!range) {
    return;
  }

  // Get list of bin intervals, if any
  std::vector<std::pair<double, double>> intervals;
  auto iter = mIntervals.find(key);
  if (iter == mIntervals.end()) {
    // No range specified for this histogram, full X range considered
    intervals.emplace_back(std::make_pair<double, double>(hist->GetXaxis()->GetXmin(), hist->GetXaxis()->GetXmax()));
  } else {
    // Collect bin ranges that have to be checked for this histogram
    intervals = iter->second;
  }

  for (auto i : intervals) {
    // draw horizontal limits
    double xmin = i.first;
    double xmax = i.second;
    TLine* l1 = new TLine(xmin, range->first, xmax, range->first);
    l1->SetLineColor(kBlue);
    l1->SetLineStyle(kDotted);
    hist->GetListOfFunctions()->Add(l1);

    TLine* l2 = new TLine(xmin, range->second, xmax, range->second);
    l2->SetLineColor(kBlue);
    l2->SetLineStyle(kDotted);
    hist->GetListOfFunctions()->Add(l2);
  }
}

} // namespace o2::quality_control_modules::muon
