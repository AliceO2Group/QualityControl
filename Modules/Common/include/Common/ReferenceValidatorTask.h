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
/// \file    ReferenceValidatorTask.h
/// \author  Andrea Ferrero
/// \brief   Post-processing task that generates a summary of the comparison between given plots and their references
///

#ifndef QUALITYCONTROL_REFERENCEVALIDATORTASK_H
#define QUALITYCONTROL_REFERENCEVALIDATORTASK_H

#include "Common/ReferenceValidatorTaskConfig.h"
#include "Common/BigScreenCanvas.h"
#include "Common/ReferenceComparatorAlgo.h"
#include "QualityControl/PostProcessingInterface.h"
#include "QualityControl/DatabaseInterface.h"
#include "QualityControl/Quality.h"
#include <TH1.h>
#include <TCanvas.h>
#include <TPaveText.h>
#include <TText.h>
#include <string>
#include <map>

namespace o2::quality_control_modules::common
{

/// \brief  Post-processing task that generates a summary of the comparison between given plots and their references
///
/// The input histograms are divided in groups. The result of the comparison is displayed as a matrix of colored boxes,
/// each box correspinding to one of the input groups
///
/// \author Andrea Ferrero
class ReferenceValidatorTask : public quality_control::postprocessing::PostProcessingInterface
{
 public:
  ReferenceValidatorTask() = default;
  ~ReferenceValidatorTask() override = default;

  void configure(const boost::property_tree::ptree& config) override;
  void initialize(quality_control::postprocessing::Trigger, framework::ServiceRegistryRef) override;
  void update(quality_control::postprocessing::Trigger, framework::ServiceRegistryRef) override;
  void finalize(quality_control::postprocessing::Trigger, framework::ServiceRegistryRef) override;

 private:
  template <class T>
  T getParameter(std::string parName, const T defaultValue, const o2::quality_control::core::Activity& activity);
  template <class T>
  T getParameter(std::string parName, const T defaultValue);

  //int mRefRun{ 0 };
  MOCompMethod mCompMethod{ MOCompDeviation };
  double mCompThreshold{ 0 };
  int mNRows{ 4 };
  int mNCols{ 5 };
  int mBorderWidth{ 7 };
  int mNotOlderThan{ 120 };

  /// \brief configuration parameters
  ReferenceValidatorTaskConfig mConfig;
  /// \brief list of plot names, separately for each group
  std::map<std::string, std::vector<std::string>> mPlotNames;

  /// \brief colors associated to each quality state (Good/Medium/Bad/Null)
  std::unordered_map<std::string, int> mColors;

  /// \brief canvas with overall result for each group
  std::unique_ptr<BigScreenCanvas> mSummaryCanvas;
  /// \brief canvases with comparison details, one for each group
  std::map<std::string, std::unique_ptr<TCanvas>> mDetailsCanvas;
  /// \brief TPaveText with comparison details, one for each group
  std::map<std::string, std::unique_ptr<TPaveText>> mDetailsText;
};

template <class T>
T ReferenceValidatorTask::getParameter(std::string parName, const T defaultValue, const o2::quality_control::core::Activity& activity)
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
T ReferenceValidatorTask::getParameter(std::string parName, const T defaultValue)
{
  T result = defaultValue;
  auto parOpt = mCustomParameters.atOptional(parName);
  if (parOpt.has_value()) {
    std::stringstream ss(parOpt.value());
    ss >> result;
  }
  return result;
}

} // namespace o2::quality_control_modules::common

#endif // QUALITYCONTROL_REFERENCEVALIDATORTASK_H
