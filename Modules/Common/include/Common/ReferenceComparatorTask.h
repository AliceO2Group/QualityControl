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
#include <TCanvas.h>
#include <TPaveText.h>
#include <TText.h>
#include <string>
#include <map>

namespace o2::quality_control_modules::common
{

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

  struct HistoWithRef
  {
    std::shared_ptr<TH1> mPlot;
    std::shared_ptr<TH1> mRefPlot;
    std::shared_ptr<TCanvas> mCanvas;
  };

 private:
  std::shared_ptr<o2::quality_control::core::MonitorObject> getRefPlot(o2::quality_control::repository::DatabaseInterface& qcdb, std::string fullPath, o2::quality_control::core::Activity activity);
  void addHistoWithReference(std::shared_ptr<o2::quality_control::core::MonitorObject> refPlot, ReferenceComparatorTaskConfig::DataGroup group, std::string path);
  void updatePlot(std::string plot, TObject* object);
  void updatePlotRatio(std::string plot, TObject* object);

  template <class HIST, class HISTRATIO>
  void initHistograms1D(TObject* refObject,
      const ReferenceComparatorTaskConfig::DataGroup& group,
      std::string path);
  template <class HIST, class HISTRATIO>
  void initHistograms2D(TObject* refObject,
      const ReferenceComparatorTaskConfig::DataGroup& group,
      std::string path);

  template <class T>
  T getParameter(std::string parName, const T defaultValue, const o2::quality_control::core::Activity& activity);
  template <class T>
  T getParameter(std::string parName, const T defaultValue);

  int mRefRun{ 0 };
  int mNotOlderThan{ 120 };

  /// \brief configuration parameters
  ReferenceComparatorTaskConfig mConfig;
  /// \brief list of plot names, separately for each group
  std::map<std::string, std::vector<std::string>> mPlotNames;
  /// \brief reference MOs
  std::map<std::string, std::shared_ptr<o2::quality_control::core::MonitorObject>> mRefPlots;
  /// \brief wether to auto-scale or not the reference of a given plot
  std::map<std::string, bool> mRefScale;
  /// \brief histograms with references superimposed
  std::map<std::string, HistoWithRef> mHistograms;
  /// \brief histograms with current/reference ratios
  std::map<std::string, std::unique_ptr<TH1>> mHistogramRatios;
};

template <class T>
T ReferenceComparatorTask::getParameter(std::string parName, const T defaultValue, const o2::quality_control::core::Activity& activity)
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
T ReferenceComparatorTask::getParameter(std::string parName, const T defaultValue)
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

#endif // QUALITYCONTROL_REFERENCECOMPARATORTASK_H
