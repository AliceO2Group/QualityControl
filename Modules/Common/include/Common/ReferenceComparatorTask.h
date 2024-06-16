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
/// \file    ReferenceComparatorTask.h
/// \author  Andrea Ferrero
/// \brief   Post-processing task that compares a given set of plots with reference ones
///

#ifndef QUALITYCONTROL_REFERENCECOMPARATORTASK_H
#define QUALITYCONTROL_REFERENCECOMPARATORTASK_H

#include "Common/ReferenceComparatorTaskConfig.h"
#include "Common/BigScreenCanvas.h"
#include "QualityControl/PostProcessingInterface.h"
#include "QualityControl/DatabaseInterface.h"
#include "QualityControl/Quality.h"
#include <TH1.h>
#include <TPad.h>
#include <TCanvas.h>
#include <TPaveText.h>
#include <TText.h>
#include <string>
#include <map>

namespace o2::quality_control_modules::common
{

class ReferenceComparatorPlot;

/// \brief Post-processing task that compares a given set of plots with reference ones
///
/// For each input plot, the task publishes the ratio between the plot and the corresponding reference.
/// Moreover, for 1-D histograms it also publishes the plot itself with the reference superimposed, for visual comparison.
///
/// \author Andrea Ferrero
class ReferenceComparatorTask : public quality_control::postprocessing::PostProcessingInterface
{
 public:
  ReferenceComparatorTask() = default;
  ~ReferenceComparatorTask() override = default;

  void configure(const boost::property_tree::ptree& config) override;
  void initialize(quality_control::postprocessing::Trigger, framework::ServiceRegistryRef) override;
  void update(quality_control::postprocessing::Trigger, framework::ServiceRegistryRef) override;
  void finalize(quality_control::postprocessing::Trigger, framework::ServiceRegistryRef) override;

  struct HistoWithRef {
    std::shared_ptr<TH1> mPlot;
    std::shared_ptr<TH1> mRefPlot;
    std::shared_ptr<TH1> mRatioPlot;
    std::shared_ptr<TPad> mPadHist;
    std::shared_ptr<TPad> mPadHistRatio;
    std::shared_ptr<TCanvas> mCanvas;
  };

 private:
  std::shared_ptr<o2::quality_control::core::MonitorObject> getRefPlot(o2::quality_control::repository::DatabaseInterface& qcdb, std::string fullPath, o2::quality_control::core::Activity activity);
  void updatePlot(std::string plotName, TObject* object);

  int mRefRun{ 0 };
  int mNotOlderThan{ 120 };

  /// \brief configuration parameters
  ReferenceComparatorTaskConfig mConfig;
  /// \brief list of plot names, separately for each group
  std::map<std::string, std::vector<std::string>> mPlotNames;
  /// \brief reference MOs
  std::map<std::string, std::shared_ptr<o2::quality_control::core::MonitorObject>> mRefPlots;
  /// \brief histograms with comparison to reference
  std::map<std::string, std::shared_ptr<ReferenceComparatorPlot>> mHistograms;
};

} // namespace o2::quality_control_modules::common

#endif // QUALITYCONTROL_REFERENCECOMPARATORTASK_H
