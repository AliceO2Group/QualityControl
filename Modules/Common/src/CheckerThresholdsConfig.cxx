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
/// \file   CheckerThresholdsConfig.cxx
/// \author Andrea Ferrero
/// \brief  Utility class handling thresholds and axis ranges and retrieve them from the custom parameters
///

#include "Common/CheckerThresholdsConfig.h"
#include "Common/Utils.h"
#include "QualityControl/QcInfoLogger.h"
#include <CommonUtils/StringUtils.h>

namespace o2::quality_control_modules::common
{

namespace internal
{

class Thresholds
{
 public:
  /// \brief thersholds initialization from custom parameters
  Thresholds(const CustomParameters& customParameters, const std::string& plotName, const std::string& qualityLabel, const Activity& activity);

  /// \brief function to retrieve the thresholds for a given interaction rate, if available
  std::optional<std::pair<double, double>> getThresholdsForRate(double rate);

 private:
  /// vectors of [min,max,rate] tuples. The first two values are the minimum and maximum threshold values,
  /// and the third is the associated reference interaction rate (optional)
  std::vector<std::tuple<double, double, std::optional<double>>> mThresholds;
};

Thresholds::Thresholds(const CustomParameters& customParameters, const std::string& plotName, const std::string& qualityLabel, const Activity& activity)
{
  // get configuration parameter associated to the key
  std::string parKey = std::string("thresholds") + qualityLabel;
  if (!plotName.empty()) {
    parKey += ":";
    parKey += plotName;
  }

  std::string parValue = getFromExtendedConfig<std::string>(activity, customParameters, parKey);
  if (parValue.empty()) {
    return;
  }

  // extract information for each nominal interaction rate value
  auto interactionRatePoints = o2::utils::Str::tokenize(parValue, ';', false, true);
  for (const auto& interactionRatePoint : interactionRatePoints) {
    double thresholdMin = 0;
    double thresholdMax = 0;
    std::optional<double> nominalRate;

    // extract interaction rate and threshold pairs
    auto rateAndThresholds = o2::utils::Str::tokenize(interactionRatePoint, ':', false, true);
    std::string thresholdValues;
    if (rateAndThresholds.size() == 2) {
      // the token contains both the rate and threshold values
      // the rate is comverted from kHz to Hz
      try {
        nominalRate = std::stod(rateAndThresholds[0]) * 1000;
      } catch (std::exception& exception) {
        ILOG(Error, Support) << "Cannot convert values from string to double for " << qualityLabel << " thresholds of plot \"" << plotName << "\", string is " << rateAndThresholds[0] << ENDM;
      }
      thresholdValues = rateAndThresholds[1];
    } else {
      // the token only contains threshold values, which are then valid for any rate
      thresholdValues = rateAndThresholds[0];
    }

    // extract the comma-separated minimum and maximum threshold values
    std::vector<std::string> thresholdMinMax = o2::utils::Str::tokenize(thresholdValues, ',', false, true);
    if (thresholdMinMax.size() == 2) {
      try {
        thresholdMin = std::stod(thresholdMinMax[0]);
        thresholdMax = std::stod(thresholdMinMax[1]);
      } catch (std::exception& exception) {
        ILOG(Error, Support) << "Cannot convert values from string to double for " << qualityLabel << " thresholds of plot \"" << plotName << "\", string is " << thresholdValues << ENDM;
      }
    }

    mThresholds.emplace_back(std::make_tuple(thresholdMin, thresholdMax, nominalRate));
  }
}

static std::pair<double, double> interpolateThresholds(double fraction, const std::tuple<double, double, std::optional<double>>& thresholdsLow, const std::tuple<double, double, std::optional<double>>& thresholdsHigh)
{
  double thresholdMin = std::get<0>(thresholdsLow) * (1.0 - fraction) + std::get<0>(thresholdsHigh) * fraction;
  double thresholdMax = std::get<1>(thresholdsLow) * (1.0 - fraction) + std::get<1>(thresholdsHigh) * fraction;
  return std::make_pair(thresholdMin, thresholdMax);
}

std::optional<std::pair<double, double>> Thresholds::getThresholdsForRate(double rate)
{
  std::optional<std::pair<double, double>> result;

  // index and corresponding rate of the element immediately below the requested rate
  int indexLow = -1;
  double rateLow = -1;

  // index and corresponding rate of the element immediately above the requested rate
  int indexHigh = -1;
  double rateHigh = -1;

  // search the closest element above and below the requested rate
  for (int index = 0; index < mThresholds.size(); index++) {
    const auto& element = mThresholds[index];
    // skip if rate is not specified
    if (!std::get<2>(element)) {
      continue;
    }
    double rateCurrent = std::get<2>(element).value();

    if (rateCurrent <= rate) {
      if (indexLow < 0 || rateLow < rateCurrent) {
        indexLow = index;
        rateLow = rateCurrent;
      }
    }

    if (rateCurrent >= rate) {
      if (indexHigh < 0 || rateHigh > rateCurrent) {
        indexHigh = index;
        rateHigh = rateCurrent;
      }
    }
  }

  if (indexLow >= 0 && indexHigh >= 0 && indexLow != indexHigh) {
    double fraction = (rate - rateLow) / (rateHigh - rateLow);
    result = interpolateThresholds(fraction, mThresholds[indexLow], mThresholds[indexHigh]);
  } else if (indexLow >= 0) {
    result = std::make_pair(std::get<0>(mThresholds[indexLow]), std::get<1>(mThresholds[indexLow]));
  } else if (indexHigh >= 0) {
    result = std::make_pair(std::get<0>(mThresholds[indexHigh]), std::get<1>(mThresholds[indexHigh]));
  } else if (mThresholds.size() > 0) {
    result = std::make_pair(std::get<0>(mThresholds[0]), std::get<1>(mThresholds[0]));
  }

  return result;
}

class XYRanges
{
 public:
  /// \brief range initialization from custom parameters
  XYRanges(const CustomParameters& customParameters, const std::string& plotName, const std::string& axisName, const Activity& activity);

  /// \brief retrieve the axis range over which the check must be restricted
  std::optional<std::pair<double, double>> getRange() { return mRange; }

 private:
  /// \brief axis range over which the check must be restricted
  std::optional<std::pair<double, double>> mRange;
};

XYRanges::XYRanges(const CustomParameters& customParameters, const std::string& plotName, const std::string& axisName, const Activity& activity)
{
  // build the key associated to the configuration parameter
  std::string parKey = std::string("range") + axisName;
  if (!plotName.empty()) {
    parKey += ":";
    parKey += plotName;
  }

  std::string parValue = getFromExtendedConfig<std::string>(activity, customParameters, parKey);
  if (parValue.empty()) {
    return;
  }

  // extract min and max values of the range
  auto rangeMinMax = o2::utils::Str::tokenize(parValue, ',', false, true);
  if (rangeMinMax.size() != 2) {
    ILOG(Error, Support) << "Cannot parse values for " << axisName << " axis range of plot \"" << plotName << "\", string is " << parValue << ENDM;
    return;
  }

  try {
    mRange = std::make_pair<double, double>(std::stof(rangeMinMax[0]), std::stof(rangeMinMax[1]));
  } catch (std::exception& exception) {
    ILOG(Error, Support) << "Cannot convert values from string to double for " << axisName << " axis range of plot \"" << plotName << "\", string is " << parValue << ENDM;
  }
}

} // namespace internal

CheckerThresholdsConfig::CheckerThresholdsConfig(const CustomParameters& customParameters, const Activity& activity)
  : mCustomParameters(customParameters), mActivity(activity)
{
  mDefaultThresholds[0] = std::make_shared<internal::Thresholds>(mCustomParameters, std::string(), std::string("Bad"), mActivity);
  mDefaultThresholds[1] = std::make_shared<internal::Thresholds>(mCustomParameters, std::string(), std::string("Medium"), mActivity);

  mDefaultRanges[0] = std::make_shared<internal::XYRanges>(mCustomParameters, std::string(), std::string("X"), mActivity);
  mDefaultRanges[1] = std::make_shared<internal::XYRanges>(mCustomParameters, std::string(), std::string("Y"), mActivity);
}

void CheckerThresholdsConfig::initThresholdsForPlot(const std::string& plotName)
{
  // if not done yet, read from the configuration the threshold settings specific to plotName
  using MapElementType = std::unordered_map<std::string, std::shared_ptr<internal::Thresholds>>::value_type;
  if (mThresholds[0].count(plotName) == 0) {
    mThresholds[0].emplace(MapElementType(plotName, std::make_shared<internal::Thresholds>(mCustomParameters, plotName, std::string("Bad"), mActivity)));
    mThresholds[1].emplace(MapElementType(plotName, std::make_shared<internal::Thresholds>(mCustomParameters, plotName, std::string("Medium"), mActivity)));
  }
}

std::array<std::optional<std::pair<double, double>>, 2> CheckerThresholdsConfig::getThresholdsForPlot(const std::string& plotName, double rate)
{
  initThresholdsForPlot(plotName);

  std::array<std::optional<std::pair<double, double>>, 2> result;

  // get thresholds for Bad and Medium quality
  for (size_t index = 0; index < 2; index++) {
    result[index] = mThresholds[index][plotName]->getThresholdsForRate(rate);
    if (!result[index]) {
      // try with default threshold settings if the plot-specific ones are not available
      result[index] = mDefaultThresholds[index]->getThresholdsForRate(rate);
    }
  }

  return result;
}

void CheckerThresholdsConfig::initRangesForPlot(const std::string& plotName)
{
  // if not done yet, read from the configuration the ranges settings specific to plotName
  using MapElementType = std::unordered_map<std::string, std::shared_ptr<internal::XYRanges>>::value_type;
  if (mRanges[0].count(plotName) == 0) {
    mRanges[0].emplace(MapElementType(plotName, std::make_shared<internal::XYRanges>(mCustomParameters, plotName, std::string("X"), mActivity)));
    mRanges[1].emplace(MapElementType(plotName, std::make_shared<internal::XYRanges>(mCustomParameters, plotName, std::string("Y"), mActivity)));
  }
}

std::array<std::optional<std::pair<double, double>>, 2> CheckerThresholdsConfig::getRangesForPlot(const std::string& plotName)
{
  initRangesForPlot(plotName);

  std::array<std::optional<std::pair<double, double>>, 2> result;

  // get ranges for X and Y axes
  for (size_t index = 0; index < 2; index++) {
    result[index] = mRanges[index][plotName]->getRange();
    if (!result[index]) {
      // try with default threshold settings if the plot-specific ones are not available
      result[index] = mDefaultRanges[index]->getRange();
    }
  }

  return result;
}

} // namespace o2::quality_control_modules::common
