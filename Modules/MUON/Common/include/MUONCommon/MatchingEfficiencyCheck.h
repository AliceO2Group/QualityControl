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
/// \file   MatchingEfficiencyCheck.h
/// \author Andrea Ferrero
///

#ifndef QC_MODULE_MUON_MATCHINGEFFICIENCYCHECK_H
#define QC_MODULE_MUON_MATCHINGEFFICIENCYCHECK_H

#include "QualityControl/CheckInterface.h"

#include <map>

namespace o2::quality_control_modules::muon
{

/// \brief  Check whether a plot is empty or not.
///
/// \author Andrea Ferrero
class MatchingEfficiencyCheck : public o2::quality_control::checker::CheckInterface
{
 public:
  /// Default constructor
  MatchingEfficiencyCheck() = default;
  /// Destructor
  ~MatchingEfficiencyCheck() override = default;

  void configure() override;
  Quality check(std::map<std::string, std::shared_ptr<MonitorObject>>* moMap) override;
  void beautify(std::shared_ptr<MonitorObject> mo, Quality checkResult = Quality::Null) override;
  std::string getAcceptedType() override;

  void startOfActivity(const Activity& activity) override;
  void endOfActivity(const Activity& activity) override;

  ClassDefOverride(MatchingEfficiencyCheck, 1);

 private:
  template <class T>
  T getParameter(std::string parName, const T defaultValue, const o2::quality_control::core::Activity& activity);
  template <class T>
  T getParameter(std::string parName, const T defaultValue);

  void initRange(std::string key);
  std::optional<std::pair<double, double>> getRange(std::string key);

  Activity mActivity;
  std::map<std::string, std::pair<double, double>> mRanges;
  std::map<std::string, std::vector<std::pair<double, double>>> mIntervals;
  std::map<std::string, Quality> mQualities;
};

template <class T>
T MatchingEfficiencyCheck::getParameter(std::string parName, const T defaultValue, const o2::quality_control::core::Activity& activity)
{
  T result = defaultValue;
  auto parOpt = mCustomParameters.atOptional(parName, activity);
  if (parOpt.has_value()) {
    std::stringstream ss(parOpt.value());
    ss >> result;
  }
  return result;
}

template <class T>
T MatchingEfficiencyCheck::getParameter(std::string parName, const T defaultValue)
{
  T result = defaultValue;
  auto parOpt = mCustomParameters.atOptional(parName);
  if (parOpt.has_value()) {
    std::stringstream ss(parOpt.value());
    ss >> result;
  }
  return result;
}

} // namespace o2::quality_control_modules::muon

#endif // QC_MODULE_MUON_MATCHINGEFFICIENCYCHECK_H
