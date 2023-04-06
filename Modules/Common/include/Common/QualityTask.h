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
/// \file    QualityTask.h
/// \author  Andrea Ferrero
/// \brief   Post-processing of quality flags
///

#ifndef QUALITYCONTROL_QUALITYTASK_H
#define QUALITYCONTROL_QUALITYTASK_H

#include "QualityControl/PostProcessingInterface.h"
#include "Common/QualityTaskConfig.h"
#include "QualityControl/Quality.h"
#include <TH1F.h>
#include <TGraph.h>
#include <TCanvas.h>
#include <TText.h>
#include <string>
#include <map>

namespace o2::quality_control::core
{
class QualityObject;
}

namespace o2::quality_control::repository
{
class DatabaseInterface;
}

namespace o2::quality_control::postprocessing
{
struct Trigger;
}

namespace o2::quality_control_modules::common
{

/// \brief  A post-processing task which shows and trends a given list of quality flags
///
/// A post-processing task which shows and trends a given list of quality flags.
/// The list of quality objects to be monitored is passed through the task's dataSources.
/// The task produces the following output:
/// * A canvas with the value of the quality objects in human-readable format.
///   The aggregated quality, whose name can be specified via configuration keys,
///   is shown at the top of the canvas.
///   Configurable messages can also be associated to the various possible values of
///   the aggregated quality (Good/Medium/Bad/Null)
/// * An histogram with the distribution of the values for each quality object
/// * A trend plot for each of the quality objects, showing the evolution of the values over time
///
/// \author Andrea Ferrero
class QualityTask : public quality_control::postprocessing::PostProcessingInterface
{
 public:
  /// \brief helper class to trend the values of a given Quality Object
  struct QualityTrendGraph : public TCanvas {
    QualityTrendGraph(std::string name, std::string title);
    void update(uint64_t time, o2::quality_control::core::Quality q);
    static std::string distributionName(const std::string& groupName, const std::string& qualityName);
    static std::string trendName(const std::string& groupName, const std::string& qualityName);

    std::unique_ptr<TGraph> mGraph;
    std::unique_ptr<TGraph> mGraphHist;
    std::array<std::unique_ptr<TText>, 4> mLabels;
  };

  QualityTask() = default;
  ~QualityTask() override = default;

  void configure(const boost::property_tree::ptree& config) override;
  void initialize(quality_control::postprocessing::Trigger, framework::ServiceRegistryRef) override;
  void update(quality_control::postprocessing::Trigger, framework::ServiceRegistryRef) override;
  void finalize(quality_control::postprocessing::Trigger, framework::ServiceRegistryRef) override;

 private:
  std::pair<std::shared_ptr<quality_control::core::QualityObject>, bool> getQO(
    quality_control::repository::DatabaseInterface& qcdb, const quality_control::postprocessing::Trigger& t, const std::string& fullPath, const std::string& group);

 private:
  /// \brief configuration parameters
  QualityTaskConfig mConfig;
  /// \brief latest creation timestamp of each tracked QO
  std::unordered_map<std::string /* full path */, uint64_t> mLatestTimestamps;
  /// \brief colors associated to each quality state (Good/Medium/Bad/Null)
  std::unordered_map<std::string, int> mColors;
  /// \brief numerical IDs associated to each quality state (Good/Medium/Bad/Null)
  std::unordered_map<std::string, int> mQualityIDs;
  /// \brief messages associated to each quality state (Good/Medium/Bad/Null)
  std::unordered_map<std::string, std::string> mCheckerMessages;
  /// \brief Quality Objects histograms
  std::unordered_map<std::string, std::unique_ptr<TH1F>> mHistograms;
  /// \brief Quality Objects trends
  std::unordered_map<std::string, std::unique_ptr<QualityTrendGraph>> mTrends;
  /// \brief canvas with human-readable quality states and messages
  std::unique_ptr<TCanvas> mQualityCanvas;
};

} // namespace o2::quality_control_modules::common

#endif // QUALITYCONTROL_QUALITYTASK_H