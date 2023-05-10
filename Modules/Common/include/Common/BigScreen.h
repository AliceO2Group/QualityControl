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
/// \file    BigScreen.h
/// \author  Andrea Ferrero
/// \brief   Quality post-processing task that generates a canvas showing the aggregated quality of each system
///

#ifndef QUALITYCONTROL_BIGSCREEN_H
#define QUALITYCONTROL_BIGSCREEN_H

#include "QualityControl/PostProcessingInterface.h"
#include "Common/BigScreenConfig.h"
#include "QualityControl/Quality.h"
#include <TCanvas.h>
#include <TPaveText.h>
#include <TText.h>
#include <string>
#include <map>

namespace o2::quality_control_modules::common
{

/// \brief  Quality post-processing task that generates a canvas showing the aggregated quality of each system
///
/// The aggregated quality of each system is displayed as a matrix of colored boxes, with the name of the system
/// above the box and the quality string inside the box
///
/// \author Andrea Ferrero
class BigScreen : public quality_control::postprocessing::PostProcessingInterface
{
 public:
  BigScreen() = default;
  ~BigScreen() override = default;

  void configure(const boost::property_tree::ptree& config) override;
  void initialize(quality_control::postprocessing::Trigger, framework::ServiceRegistryRef) override;
  void update(quality_control::postprocessing::Trigger, framework::ServiceRegistryRef) override;
  void finalize(quality_control::postprocessing::Trigger, framework::ServiceRegistryRef) override;

 private:
  /// \brief configuration parameters
  BigScreenConfig mConfig;
  /// \brief colors associated to each quality state (Good/Medium/Bad/Null)
  std::unordered_map<std::string, int> mColors;
  /// \brief
  std::map<std::string, std::unique_ptr<TText>> mLabels;
  /// \brief
  std::map<std::string, std::unique_ptr<TPaveText>> mPaves;
  /// \brief canvas with human-readable quality states and messages
  std::unique_ptr<TCanvas> mCanvas;
};

} // namespace o2::quality_control_modules::common

#endif // QUALITYCONTROL_BIGSCREEN_H
