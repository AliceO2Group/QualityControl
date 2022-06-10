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
/// \file     TrendingTaskExtended.h
/// \author   Marcel Lesch
/// \author   Cindy Mordasini
/// \author   Based on the work from Piotr Konopka
///

#ifndef QUALITYCONTROL_TRENDINGTASKEXTENDED_H
#define QUALITYCONTROL_TRENDINGTASKEXTENDED_H

#include "QualityControl/PostProcessingInterface.h"
#include "QualityControl/ReductorExtended.h"
#include "QualityControl/SliceInfoTrending.h"
#include "QualityControl/TrendingTaskExtendedConfig.h"

#include <memory>
#include <unordered_map>
#include <TCanvas.h>
#include <TTree.h>

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

class TrendingTaskExtended : public PostProcessingInterface
{
 public:
  /// \brief Constructor.
  TrendingTaskExtended() = default;
  /// \brief Destructor.
  ~TrendingTaskExtended() final = default;

  /// \brief Post-processing methods inherited from 'PostProcessingInterface'.
  void configure(std::string name, const boost::property_tree::ptree& config) final;
  void initialize(Trigger, framework::ServiceRegistry&) final;
  void update(Trigger, framework::ServiceRegistry&) final;
  void finalize(Trigger, framework::ServiceRegistry&) final;

 private:
  struct MetaData {
    Int_t runNumber = 0;
  };

  /// \brief Methods specific to the trending itself.
  void trendValues(const Trigger& t, o2::quality_control::repository::DatabaseInterface&);
  void generatePlots();
  void drawCanvasMO(TCanvas* thisCanvas, const std::string& var,
                    const std::string& name, const std::string& opt, const std::string& err, const std::vector<std::vector<float>>& axis);
  void drawCanvasQO(TCanvas* thisCanvas, const std::string& var, const std::string& name, const std::string& opt);
  void getUserAxisRange(const std::string graphAxisRange, float& limitLow, float& limitUp);
  void setUserAxisLabel(TAxis* xAxis, TAxis* yAxis, const std::string graphAxisLabel);
  void getTrendVariables(const std::string& inputvar, std::string& sourceName, std::string& variableName, std::string& trend);
  void getTrendErrors(const std::string& inputvar, std::string& errorX, std::string& errorY);
  void saveObjectToPrimitives(TCanvas* canvas, const int padNumber, TObject* object);

  template <typename T>
  void beautifyGraph(T& graph, const TrendingTaskExtendedConfig::Plot& plotconfig, TCanvas* canv); // beautify function for TGraphs and TMultiGraphs

  TrendingTaskExtendedConfig mConfig;
  MetaData mMetaData;
  UInt_t mTime;
  std::unique_ptr<TTree> mTrend;
  std::map<std::string, TObject*> mPlots;
  std::unordered_map<std::string, std::unique_ptr<ReductorExtended>> mReductors;
  std::unordered_map<std::string, std::vector<SliceInfo>> mSources;
  std::unordered_map<std::string, SliceInfoQuality> mSourcesQuality;
  std::unordered_map<std::string, bool> mIsMoObject;
  std::unordered_map<std::string, int> mNumberPads;
  std::unordered_map<std::string, std::vector<std::vector<float>>> mAxisDivision;
};

} // namespace o2::quality_control::postprocessing

#endif // QUALITYCONTROL_TRENDINGTASKEXTENDED_H
