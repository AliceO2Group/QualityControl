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
/// \file   MeanVertexCheck.cxx
/// \author Andrea Ferrero
///

#include "GLO/MeanVertexCheck.h"
#include "QualityControl/MonitorObject.h"
#include "QualityControl/Quality.h"
#include "QualityControl/QcInfoLogger.h"
#include <CommonUtils/StringUtils.h>
#include <TMath.h>
#include <TGraph.h>
#include <TH1.h>
#include <TCanvas.h>
#include <TLine.h>

namespace o2::quality_control_modules::glo
{
void MeanVertexCheck::configure() {}

void MeanVertexCheck::startOfActivity(const Activity& activity)
{
  mActivity = activity;

  std::string parKey = std::string("nPointsToCheck");
  // search in extended parameters first
  auto parOpt = mCustomParameters.atOptional(parKey, mActivity);
  if (parOpt.has_value()) {
    if (parOpt.value() == "all") {
      mNPointsToCheck = 0;
    } else {
      mNPointsToCheck = std::stoi(parOpt.value());
    }
  } else {
    // search in standard parameters if key not found in extended ones
    parOpt = mCustomParameters.atOptional(parKey);
    if (parOpt.value() == "all") {
      mNPointsToCheck = 0;
    } else {
      mNPointsToCheck = std::stoi(parOpt.value());
    }
  }
  ILOG(Info, Support) << "MeanVertexCheck: checking at most " << mNPointsToCheck << " points " << ENDM;
}

void MeanVertexCheck::endOfActivity(const Activity& activity)
{
  mActivity = Activity{};
  mRanges.clear();
  mQualities.clear();
}

static std::string getBaseName(std::string name)
{
  auto pos = name.rfind("/");
  return ((pos < std::string::npos) ? name.substr(pos + 1) : name);
}

void MeanVertexCheck::initRange(std::string key)
{
  // Get acceptable range for this histogram
  auto iter = mRanges.find(key);
  if (iter != mRanges.end()) {
    return;
  }

  // get configuration parameter associated to the key
  std::string parKey = std::string("range:") + key;
  std::string parValue;
  // search in extended parameters first
  auto parOpt = mCustomParameters.atOptional(parKey, mActivity);
  if (parOpt.has_value()) {
    parValue = parOpt.value();
  } else {
    // search in standard parameters if key not found in extended ones
    parOpt = mCustomParameters.atOptional(parKey);
    if (parOpt.has_value()) {
      parValue = parOpt.value();
    }
  }

  // extract the minimum and maximum values from the configurable parameter
  auto range = o2::utils::Str::tokenize(parValue, ',', false, true);
  if (range.size() == 2) {
    double min = std::stod(range[0]);
    double max = std::stod(range[1]);
    mRanges.insert(std::pair{ key, std::make_pair(min, max) });
    ILOG(Info, Support) << "MeanVertexCheck: range for " << key << " set to [" << min << "," << max << "]" << ENDM;
  }
}

std::optional<std::pair<double, double>> MeanVertexCheck::getRange(std::string key)
{
  std::optional<std::pair<double, double>> result;

  // Get acceptable range for this histogram
  auto iter = mRanges.find(key);
  if (iter != mRanges.end()) {
    result = iter->second;
  }

  return result;
}

Quality MeanVertexCheck::check(std::map<std::string, std::shared_ptr<MonitorObject>>* moMap)
{
  for (auto& [moKey, mo] : *moMap) {

    auto moName = mo->getName();
    auto key = getBaseName(moName);

    // get acceptable range for the current plot
    initRange(key);
    auto range = getRange(key);
    if (!range) {
      continue;
    }

    // check that the object is a TGraph
    TGraph* graph = dynamic_cast<TGraph*>(mo->getObject());
    if (!graph) {
      continue;
    }

    // Quality is good by default, unless one of the points is outside the acceptable range
    mQualities[moName] = Quality::Good;

    // loop over points in reverse order
    int nChecked = 0;
    int nPoints = graph->GetN();
    for (int p = nPoints - 1; p >= 0; p--) {
      double value = graph->GetPointY(p);
      // check that the value is within the acceptable range
      if (value < range->first || value > range->second) {
        mQualities[moName] = Quality::Bad;
        break;
      }

      nChecked += 1;
      // stop if we already checked enough points
      if (mNPointsToCheck > 0 && nChecked >= mNPointsToCheck) {
        break;
      }
    }
  }

  // compute overall quality
  Quality result = mQualities.empty() ? Quality::Null : Quality::Good;
  for (auto& [key, q] : mQualities) {
    if (q.isWorseThan(result)) {
      result = q;
    }
  }

  return result;
}



void MeanVertexCheck::beautify(std::shared_ptr<MonitorObject> mo, Quality checkResult)
{
  auto moName = mo->getName();
  Quality quality = mQualities[moName];

  // get acceptable range for plot, to draw the horizontal lines
  auto key = getBaseName(moName);
  auto range = getRange(key);
  if (!range) {
    return;
  }

  // check that the object is a TGraph
  auto graph = dynamic_cast<TGraph*>(mo->getObject());
  if (!graph || graph->GetN() < 1) {
    return;
  }

  // draw the graph in red if the quality is Bad
  if (quality == Quality::Bad) {
    graph->SetLineColor(kRed);
    graph->SetMarkerColor(kRed);
  }

  // Compute vertical axis range
  double min = TMath::Min(range->first, TMath::MinElement(graph->GetN(), graph->GetY()));
  double max = TMath::Max(range->second, TMath::MaxElement(graph->GetN(), graph->GetY()));
  // put 10% of margin above and below the limits
  auto delta = max - min;
  min -= 0.1 * delta;
  max += 0.1 * delta;

  graph->SetMinimum(min);
  graph->SetMaximum(max);

  // add horizontal lines delimiting the acceptable range
  double xmin = graph->GetXaxis()->GetXmin();
  double xmax = graph->GetXaxis()->GetXmax();
  TLine* l1 = new TLine(xmin, range->first, xmax, range->first);
  TLine* l2 = new TLine(xmin, range->second, xmax, range->second);

  l1->SetLineStyle(kDotted);
  l2->SetLineStyle(kDotted);

  if (quality == Quality::Bad) {
    l1->SetLineColor(kRed);
    l2->SetLineColor(kRed);
  } else {
    l1->SetLineColor(kBlue);
    l2->SetLineColor(kBlue);
  }

  graph->GetListOfFunctions()->Add(l1);
  graph->GetListOfFunctions()->Add(l2);
}

} // namespace o2::quality_control_modules::glo
