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
/// \file   CheckerThresholdsConfig.h
/// \author Andrea Ferrero
/// \brief  Utility class handling thresholds and axis ranges and retrieve them from the custom parameters
///

#ifndef QC_MODULE_COMMON_CHECKERTHRESHOLDSCONFIG_H
#define QC_MODULE_COMMON_CHECKERTHRESHOLDSCONFIG_H

#include "QualityControl/Activity.h"
#include "QualityControl/CustomParameters.h"

#include <string>
#include <optional>
#include <array>
#include <vector>

using namespace o2::quality_control::core;

namespace o2::quality_control_modules::common
{

namespace internal
{
class Thresholds;
class XYRanges;
} // namespace internal

class CheckerThresholdsConfig
{
 public:
  CheckerThresholdsConfig(const CustomParameters& customParameters, const Activity& activity);
  ~CheckerThresholdsConfig() = default;

  /// \brief function to retrieve the thresholds for a given interaction rate, if available
  std::array<std::optional<std::pair<double, double>>, 2> getThresholdsForPlot(const std::string& plotName, double rate);

  /// \brief optional X-Y ranges over which the check must be restricted
  std::array<std::optional<std::pair<double, double>>, 2> getRangesForPlot(const std::string& plotName);

 private:
  void initThresholdsForPlot(const std::string& plotName);
  void initRangesForPlot(const std::string& plotName);

  CustomParameters mCustomParameters;
  Activity mActivity;

  /// vectors of [min,max,rate] tuples. The first two values are the minimum and maximum threshold values,
  /// and the third is the associated reference interaction rate (optional)
  std::array<std::shared_ptr<internal::Thresholds>, 2> mDefaultThresholds;
  std::array<std::unordered_map<std::string, std::shared_ptr<internal::Thresholds>>, 2> mThresholds;

  /// \brief optional X-Y ranges over which the check must be restricted
  std::array<std::shared_ptr<internal::XYRanges>, 2> mDefaultRanges;
  std::array<std::unordered_map<std::string, std::shared_ptr<internal::XYRanges>>, 2> mRanges;
};

} // namespace o2::quality_control_modules::common

#endif // QC_MODULE_COMMON_CHECKERTHRESHOLDSCONFIG_H
