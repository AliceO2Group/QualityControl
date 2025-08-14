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
/// \file   TrendCheck.cxx
/// \author Andrea Ferrero
///

#include "Common/TrendCheck.h"
#include "Common/CheckerThresholdsConfig.h"
#include "QualityControl/MonitorObject.h"
#include "QualityControl/Quality.h"
#include "QualityControl/QcInfoLogger.h"
#include <CommonUtils/StringUtils.h>
#include <TMath.h>
#include <TGraph.h>
#include <TH1.h>
#include <TCanvas.h>
#include <TPolyLine.h>
#include <TPaveText.h>

namespace o2::quality_control_modules::common
{

void TrendCheck::configure()
{
}

void TrendCheck::startOfActivity(const Activity& activity)
{
  mActivity = activity;

  // initialize the thresholds configuration
  mThresholds = std::make_shared<CheckerThresholdsConfig>(mCustomParameters, mActivity);

  // comparison method between actual values and thresholds
  std::string parKey = "trendCheckMode";
  auto parOpt = mCustomParameters.atOptional(parKey, mActivity);
  if (!parOpt) {
    parOpt = mCustomParameters.atOptional(parKey);
  }
  mTrendCheckMode = ExpectedRange;
  if (parOpt.has_value()) {
    if (parOpt.value() == "DeviationFromMean") {
      mTrendCheckMode = DeviationFromMean;
    } else if (parOpt.value() == "StdDeviation") {
      mTrendCheckMode = StdDeviation;
    } else if (parOpt.value() != "ExpectedRange") {
      ILOG(Warning, Devel) << "unrecognized threshold mode \"" << parOpt.value() << "\", using default \"ExpectedRange\" mode" << ENDM;
    }
  }
  switch (mTrendCheckMode) {
    case ExpectedRange:
      ILOG(Info, Support) << "thresholds mode set to \"ExpectedRange\"" << ENDM;
      break;
    case DeviationFromMean:
      ILOG(Info, Support) << "thresholds mode set to \"DeviationFromMean\"" << ENDM;
      break;
    case StdDeviation:
      ILOG(Info, Support) << "thresholds mode set to \"StdDeviation\"" << ENDM;
      break;
  }

  // number of points, excluding the last one, used to compute the average and standard deviation of the trend
  // all points are considered if the value is smaller or equal to zero
  parKey = "nPointsForAverage";
  parOpt = mCustomParameters.atOptional(parKey, mActivity);
  if (!parOpt) {
    parOpt = mCustomParameters.atOptional(parKey);
  }
  if (parOpt.has_value()) {
    if (parOpt.value() == "all") {
      mNPointsForAverage = 0;
    } else {
      mNPointsForAverage = std::stoi(parOpt.value());
    }
  }

  if (mNPointsForAverage == 0) {
    ILOG(Info, Support) << "using all points for statistics calculation" << ENDM;
  } else {
    ILOG(Info, Support) << "using at most " << mNPointsForAverage << " points for statistics calculation" << ENDM;
  }

  // position and size of the label showing the result of the check
  parKey = "qualityLabelPosition";
  parOpt = mCustomParameters.atOptional(parKey, mActivity);
  if (!parOpt) {
    parOpt = mCustomParameters.atOptional(parKey);
  }
  if (parOpt.has_value()) {
    auto position = o2::utils::Str::tokenize(parOpt.value(), ',', false, true);
    if (position.size() == 2) {
      mQualityLabelPosition = std::make_pair<float, float>(std::stof(position[0]), std::stof(position[1]));
    }
  }

  parKey = "qualityLabelSize";
  parOpt = mCustomParameters.atOptional(parKey, mActivity);
  if (!parOpt) {
    parOpt = mCustomParameters.atOptional(parKey);
  }
  if (parOpt.has_value()) {
    auto position = o2::utils::Str::tokenize(parOpt.value(), ',', false, true);
    if (position.size() == 2) {
      mQualityLabelSize = std::make_pair<float, float>(std::stof(position[0]), std::stof(position[1]));
    }
  }
}

void TrendCheck::endOfActivity(const Activity& activity)
{
  // reset all members
  mActivity = Activity{};
  mAverageTrend.clear();
  mThresholdsTrendBad.clear();
  mThresholdsTrendMedium.clear();
  mQualities.clear();
}

static std::string getBaseName(std::string name)
{
  auto pos = name.rfind("/");
  return ((pos < std::string::npos) ? name.substr(pos + 1) : name);
}

// compute the mean and standard deviation from a given number of points bewfore the last one
// all points are used if nPointsForAverage <= 0
static std::optional<std::pair<double, double>> getGraphStatistics(TGraph* graph, int nPointsForAverage)
{
  std::optional<std::pair<double, double>> result;

  int nPoints = graph->GetN();
  // we need at least two points before the last one
  if (nPoints < 3) {
    return result;
  }

  // index of the last-but-one point
  int pointIndexMax = nPoints - 2;
  // index of the starting point for the mean and stadrad deviation computation
  int pointIndexMin = (nPointsForAverage > 0 && pointIndexMax >= nPointsForAverage) ? pointIndexMax - nPointsForAverage + 1 : 0;
  // number of points used for the computation, must be greater than one
  int nPointsForStats = pointIndexMax - pointIndexMin + 1;
  if (nPointsForStats < 2) {
    return result;
  }

  // compute the mean of the points
  double mean = 0;
  for (int pointIndex = pointIndexMin; pointIndex <= pointIndexMax; pointIndex++) {
    mean += graph->GetPointY(pointIndex);
  }
  mean /= nPointsForStats;

  // compute the standard deviation of the points
  double stdDev = 0;
  for (int pointIndex = pointIndexMin; pointIndex <= pointIndexMax; pointIndex++) {
    double delta = graph->GetPointY(pointIndex) - mean;
    stdDev += delta * delta;
  }
  stdDev /= (nPointsForStats - 1) * nPointsForStats;
  stdDev = std::sqrt(stdDev);

  result = std::make_pair(mean, stdDev);
  return result;
}

double TrendCheck::getInteractionRate()
{
  return 750000;
}

/// \brief compute the thresholds for a given graph, taking into account the current interaction rate
///
/// \param key string identifying some plot-specific threshold values
/// \param graph the graph object to be checked
/// \return an array of optional (min,max) threshold values for Bad and Medium qualities
std::array<std::optional<std::pair<double, double>>, 2> TrendCheck::getThresholds(std::string plotName, TGraph* graph)
{
  double rate = getInteractionRate();
  std::array<std::optional<std::pair<double, double>>, 2> result = mThresholds->getThresholdsForPlot(plotName, rate);
  if (!result[0]) {
    ILOG(Warning, Support) << "Cannot retrieve thresholds for \"" << plotName << "\"" << ENDM;
    return result;
  }

  if (mTrendCheckMode != ExpectedRange) {
    // the thresholds retrieved from the configuration need to be converted into absolute values
    auto graphStatistics = getGraphStatistics(graph, mNPointsForAverage);
    if (!graphStatistics) {
      result[0].reset();
      result[1].reset();
      return result;
    }

    if (mTrendCheckMode == DeviationFromMean) {
      double mean = graphStatistics.value().first;
      // the thresholds retrieved from the configuration are relative to the mean value of the last N points, we need to convert them into absolute values
      if (result[0].has_value()) {
        result[0].value().first = mean + result[0].value().first * std::fabs(mean);
        result[0].value().second = mean + result[0].value().second * std::fabs(mean);
      }

      if (result[1].has_value()) {
        result[1].value().first = mean + result[1].value().first * std::fabs(mean);
        result[1].value().second = mean + result[1].value().second * std::fabs(mean);
      }
    } else if (mTrendCheckMode == StdDeviation) {
      // the thresholds retrieved from the configuration are expressed as number of sigmas from the mean value of the last N points, we need to convert them into absolute values
      double mean = graphStatistics.value().first;
      double stdDevOfMean = graphStatistics.value().second;
      double lastPointValue = graph->GetPointY(graph->GetN() - 1);
      double lastPointError = graph->GetErrorY(graph->GetN() - 1);
      if (lastPointError < 0) {
        lastPointError = 0;
      }
      const double totalError = sqrt(stdDevOfMean * stdDevOfMean + lastPointError * lastPointError);

      result[0].value().first = mean + result[0].value().first * totalError;
      result[0].value().second = mean + result[0].value().second * totalError;

      if (result[1].has_value()) {
        result[1].value().first = mean + result[1].value().first * totalError;
        result[1].value().second = mean + result[1].value().second * totalError;
      }
    }
  }

  return result;
}

// helper function to retrieve a TGraph drawn inside a TPad
TGraph* getGraphFromPad(TPad* pad)
{
  if (!pad) {
    return nullptr;
  }

  TGraph* graph{ nullptr };
  int jList = pad->GetListOfPrimitives()->LastIndex();
  for (; jList >= 0; jList--) {
    graph = dynamic_cast<TGraph*>(pad->GetListOfPrimitives()->At(jList));
    if (graph) {
      break;
    }
  }

  return graph;
}

// retrieve a vector containing all the TGraph objects embedded in a given TObject
// the TObject can also be the TGraph itself
void TrendCheck::getGraphsFromObject(TObject* object, std::vector<TGraph*>& graphs)
{
  TGraph* graph = dynamic_cast<TGraph*>(object);
  if (graph) {
    graphs.push_back(graph);
  } else {
    // extract the TGraph from the canvas
    TCanvas* canvas = dynamic_cast<TCanvas*>(object);
    auto graph = getGraphFromPad(canvas);
    if (graph) {
      graphs.push_back(graph);
    } else {
      ILOG(Info, Devel) << "No TGraph found in the canvas, checking sub-pads" << ENDM;
      // loop over the pads in the canvas and extract the TGraph from each pad
      const int numberPads = canvas->GetListOfPrimitives()->GetEntries();
      for (int iPad = 0; iPad < numberPads; iPad++) {
        auto pad = dynamic_cast<TPad*>(canvas->GetListOfPrimitives()->At(iPad));
        graph = getGraphFromPad(pad);
        if (graph) {
          graphs.push_back(graph);
        }
      }
    }
  }
}

Quality TrendCheck::check(std::map<std::string, std::shared_ptr<MonitorObject>>* moMap)
{
  for (auto& [moKey, mo] : *moMap) {

    std::vector<TGraph*> graphs;
    getGraphsFromObject(mo->getObject(), graphs);
    if (graphs.empty()) {
      continue;
    }

    auto moName = mo->getName();
    auto key = getBaseName(moName);

    for (size_t graphIndex = 0; graphIndex < graphs.size(); graphIndex++) {

      TGraph* graph = graphs[graphIndex];
      if (!graph) {
        continue;
      }

      auto graphName = moName + "_" + std::to_string(graphIndex);

      // check that the graph is not empty
      int nPoints = graph->GetN();
      if (nPoints < 1) {
        continue;
      }

      // get the value for the last point
      double value = graph->GetPointY(nPoints - 1);

      // get acceptable range for the current plot
      auto thresholds = getThresholds(key, graph);
      // check that at least the thresholds for Bad quality are available
      if (!thresholds[0]) {
        continue;
      }

      // Quality is good by default, unless the last point is outside the acceptable range
      mQualities[graphName] = Quality::Good;

      mThresholdsTrendBad[graphName].emplace_back(graph->GetPointX(nPoints - 1), thresholds[0].value());
      if (thresholds[1].has_value()) {
        mThresholdsTrendMedium[graphName].emplace_back(graph->GetPointX(nPoints - 1), thresholds[1].value());
      }

      if (value < thresholds[0]->first || value > thresholds[0]->second) {
        mQualities[graphName] = Quality::Bad;
      } else if (thresholds[1].has_value()) {
        if (value < thresholds[1]->first || value > thresholds[1]->second) {
          mQualities[graphName] = Quality::Medium;
        }
      }
    }
  }

  // we return the worse quality of all the objects we checked, but we preserve all FlagTypes
  Quality result = mQualities.empty() ? Quality::Null : Quality::Good;
  for (auto& [key, quality] : mQualities) {
    (void)key;
    if (quality.isWorseThan(result)) {
      result.set(quality);
    }
    for (const auto& flag : quality.getFlags()) {
      result.addFlag(flag.first, flag.second);
    }
  }

  return result;
}

static void drawThresholds(TGraph* graph, const std::vector<std::pair<double, std::pair<double, double>>>& thresholds, int lineColor, int lineStyle)
{
  if (thresholds.empty()) {
    return;
  }

  double rangeMin = graph->GetMinimum();
  double rangeMax = graph->GetMaximum();

  rangeMin = TMath::Min(rangeMin, TMath::MinElement(graph->GetN(), graph->GetY()));
  rangeMax = TMath::Max(rangeMax, TMath::MaxElement(graph->GetN(), graph->GetY()));

  double* xValues = new double[thresholds.size()];
  double* yValuesMin = new double[thresholds.size()];
  double* yValuesMax = new double[thresholds.size()];

  for (size_t index = 0; index < thresholds.size(); index++) {
    const auto& thresholdsPoint = thresholds[index];
    xValues[index] = thresholdsPoint.first;
    yValuesMin[index] = thresholdsPoint.second.first;
    yValuesMax[index] = thresholdsPoint.second.second;

    // add some margin above an below the threshold values, if needed
    auto delta = yValuesMax[index] - yValuesMin[index];
    rangeMin = TMath::Min(rangeMin, yValuesMin[index] - 0.1 * delta);
    rangeMax = TMath::Max(rangeMax, yValuesMax[index] + 0.1 * delta);
  }

  TPolyLine* lineMin = new TPolyLine(thresholds.size(), xValues, yValuesMin);
  lineMin->SetLineColor(lineColor);
  lineMin->SetLineStyle(lineStyle);
  lineMin->SetLineWidth(2);
  graph->GetListOfFunctions()->Add(lineMin);

  TPolyLine* lineMax = new TPolyLine(thresholds.size(), xValues, yValuesMax);
  lineMax->SetLineColor(lineColor);
  lineMax->SetLineStyle(lineStyle);
  lineMax->SetLineWidth(2);
  graph->GetListOfFunctions()->Add(lineMax);

  graph->SetMinimum(rangeMin);
  graph->SetMaximum(rangeMax);
}

// return the ROOT color index associated to a give quality level
static int getQualityColor(const Quality& q)
{
  if (q == Quality::Null)
    return kViolet - 6;
  if (q == Quality::Bad)
    return kRed;
  if (q == Quality::Medium)
    return kOrange - 3;
  if (q == Quality::Good)
    return kGreen + 2;

  return 0;
}

void TrendCheck::beautify(std::shared_ptr<MonitorObject> mo, Quality checkResult)
{
  std::vector<TGraph*> graphs;
  getGraphsFromObject(mo->getObject(), graphs);
  if (graphs.empty()) {
    return;
  }

  auto moName = mo->getName();

  for (size_t graphIndex = 0; graphIndex < graphs.size(); graphIndex++) {

    TGraph* graph = graphs[graphIndex];
    if (!graph || graph->GetN() < 1) {
      return;
    }

    auto graphName = moName + "_" + std::to_string(graphIndex);

    if (!mQualities.contains(graphName)) {
      continue;
    }
    Quality quality = mQualities[graphName];

    if (mThresholdsTrendMedium.count(graphName) > 0) {
      const auto& thresholds = mThresholdsTrendMedium[graphName];
      drawThresholds(graph, thresholds, kOrange, 9);
    }

    if (mThresholdsTrendBad.count(graphName) > 0) {
      const auto& thresholds = mThresholdsTrendBad[graphName];
      drawThresholds(graph, thresholds, kRed, 9);
    }

    // draw the label with the check result
    TPaveText* label = new TPaveText(mQualityLabelPosition.first,
                                     mQualityLabelPosition.second,
                                     mQualityLabelPosition.first + mQualityLabelSize.first,
                                     mQualityLabelPosition.second + mQualityLabelSize.second,
                                     "brNDC");
    label->SetTextColor(getQualityColor(quality));
    label->AddText(quality.getName().c_str());
    label->SetFillStyle(0);
    label->SetBorderSize(0);
    label->SetTextAlign(12);
    graph->GetListOfFunctions()->Add(label);
  }
}

} // namespace o2::quality_control_modules::common
