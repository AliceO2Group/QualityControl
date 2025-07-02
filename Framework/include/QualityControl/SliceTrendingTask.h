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
/// \file     SliceTrendingTask.h
/// \author   Marcel Lesch
/// \author   Cindy Mordasini
/// \author   Based on the work from Piotr Konopka
///

#ifndef QUALITYCONTROL_SLICETRENDINGTASK_H
#define QUALITYCONTROL_SLICETRENDINGTASK_H

#include "QualityControl/PostProcessingInterface.h"
#include "QualityControl/SliceReductor.h"
#include "QualityControl/SliceTrendingTaskConfig.h"

#include <memory>
#include <map>
#include <string>
#include <vector>
#include <unordered_map>
#include <TTree.h>

class TCanvas;
class TObject;
class TLegend;

namespace o2::quality_control::repository
{
class DatabaseInterface;
} // namespace o2::quality_control::repository

namespace o2::quality_control::postprocessing
{
/// \brief  A extended version of the trending post-processing task.
///
/// A post-processing task which trends objects inside QC database (QCDB).
/// It extracts some values of one or multiple objects using the Reductor classes,
/// then stores them inside (a vector of) stuct "SliceInfo" which is saved in a TTree.
/// This class extended the trending with a subrange slicer of histograms,
/// and input/output canvas can be dealt with alongside normal histograms.
///

class SliceTrendingTask : public PostProcessingInterface
{
 public:
  /// \brief Constructor.
  SliceTrendingTask() = default;
  /// \brief Destructor.
  ~SliceTrendingTask() final = default;

  /// \brief Post-processing methods inherited from 'PostProcessingInterface'.
  void configure(const boost::property_tree::ptree& config) final;
  void initialize(Trigger, framework::ServiceRegistryRef) final;
  void update(Trigger, framework::ServiceRegistryRef) final;
  void finalize(Trigger, framework::ServiceRegistryRef) final;

 private:
  static constexpr size_t MaxRunNumberStringLength = 6;
  struct MetaData {
    // we store run numbers both as an integer and as a string to allow users to select whether they need
    // a trend in integer or label domain (the latter will contain evenly-spaced data points)
    Int_t runNumber = 0;
    char runNumberStr[MaxRunNumberStringLength + 1] = { 0 }; // 6 characters + null terminator
  };
  struct TitleSettings {
    std::string observableX;
    std::string observableY;
    std::string unitX;
    std::string unitY;
    std::string centmodeX;
    std::string centmodeY;
  };

  /// \brief Methods specific to the trending itself.
  void trendValues(const Trigger& t, o2::quality_control::repository::DatabaseInterface&);
  void generatePlots();
  void drawCanvasMO(TCanvas* thisCanvas, const std::string& var,
                    const std::string& name, const std::string& opt, const std::string& err, const std::vector<std::vector<float>>& axis, const std::vector<std::vector<std::string>>& sliceLabels, const TitleSettings& titlesettings);
  void getUserAxisRange(const std::string& graphAxisRange, float& limitLow, float& limitUp);
  void setUserAxisLabel(TAxis* xAxis, TAxis* yAxis, const std::string& graphAxisLabel);
  void getTrendVariables(const std::string& inputvar, std::string& sourceName, std::string& variableName, std::string& trend);
  void getTrendErrors(const std::string& inputvar, std::string& errorX, std::string& errorY);
  void saveObjectToPrimitives(TCanvas* canvas, const int padNumber, TObject* object);

  template <typename T>
  void beautifyGraph(T& graph, const SliceTrendingTaskConfig::Plot& plotconfig, TCanvas* canv); // beautify function for TGraphs and TMultiGraphs
  void beautifyLegend(TLegend* geg, const SliceTrendingTaskConfig::Plot& plotconfig, TCanvas* canv);
  std::string beautifyTitle(const std::string_view rawtitle, const TitleSettings& titleSettings);

  SliceTrendingTaskConfig mConfig;
  MetaData mMetaData;
  UInt_t mTime;
  std::unique_ptr<TTree> mTrend;
  std::map<std::string, TObject*> mPlots;
  std::unordered_map<std::string, std::unique_ptr<SliceReductor>> mReductors;
  std::unordered_map<std::string, std::vector<SliceInfo>*> mSources;
  std::unordered_map<std::string, int> mNumberPads;
  std::unordered_map<std::string, std::vector<std::vector<float>>> mAxisDivision;
  std::unordered_map<std::string, std::vector<std::vector<std::string>>> mSliceLabel;
};

} // namespace o2::quality_control::postprocessing

#endif // QUALITYCONTROL_SLICETRENDINGTASK_H
