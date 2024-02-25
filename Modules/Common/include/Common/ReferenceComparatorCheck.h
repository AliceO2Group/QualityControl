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
/// \file   ReferenceComparatorCheck.h
/// \author Andrea Ferrero
///

#ifndef QUALITYCONTROL_ReferenceComparatorCheck_H
#define QUALITYCONTROL_ReferenceComparatorCheck_H

#include "Common/ReferenceComparatorAlgo.h"
#include "QualityControl/CheckInterface.h"

#include <sstream>

namespace o2::quality_control_modules::common
{

/// \brief  A generic QC check that compares a given set of histograms with their corresponding references
/// \author Andrea Ferrero
class ReferenceComparatorCheck : public o2::quality_control::checker::CheckInterface
{
 public:
  /// Default constructor
  ReferenceComparatorCheck() = default;
  /// Destructor
  ~ReferenceComparatorCheck() override = default;

  // Override interface
  void configure() override;
  Quality check(std::map<std::string, std::shared_ptr<MonitorObject>>* moMap) override;
  void reset() override;
  void beautify(std::shared_ptr<MonitorObject> mo, Quality checkResult = Quality::Null) override;
  std::string getAcceptedType() override;

  void startOfActivity(const Activity& activity) override;
  void endOfActivity(const Activity& activity) override;

 private:
  template <class T> T getParameter(std::string parName, const T defaultValue, const o2::quality_control::core::Activity& activity);
  template <class T> T getParameter(std::string parName, const T defaultValue);

  MOCompMethod mPlotCompMethod{ MOCompDeviation };
  double mCompThreshold{ 0.01 };
  std::map<std::string, Quality> mQualityFlags;
};

template <class T>
T ReferenceComparatorCheck::getParameter(std::string parName, const T defaultValue, const o2::quality_control::core::Activity& activity)
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
T ReferenceComparatorCheck::getParameter(std::string parName, const T defaultValue)
{
  T result = defaultValue;
  auto parOpt = mCustomParameters.atOptional(parName);
  if (parOpt.has_value()) {
    std::stringstream ss(parOpt.value());
    ss >> result;
  }
  return result;
}


} // namespace o2::quality_control_modules::skeleton

#endif // QC_MODULE_SKELETON_ReferenceComparatorCheck_H
