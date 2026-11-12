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
/// \file    TrendingMachine.h
/// \author  Piotr Konopka
///
/// \brief   Manager class for trending functionality (TTree management, reductors, plot generation)
///
/// This class contains code extracted from TrendingTask to eliminate duplication between
/// TrendingTask and LateTrendingTask.
///
/// Code history can be accessed with:
///   git log --follow Framework/src/TrendingMachine.cxx
///   git blame -C -C -C Framework/src/TrendingMachine.cxx
///

#ifndef QUALITYCONTROL_TRENDINGMACHINE_H
#define QUALITYCONTROL_TRENDINGMACHINE_H

#include "QualityControl/TrendingMachineConfig.h"
#include <string>
#include <vector>
#include <map>
#include <memory>
#include <functional>
#include <unordered_map>

class TTree;
class TCanvas;
class TGraph;
class TAxis;
class TH1;
class TObject;

namespace o2::quality_control::postprocessing
{

class Reductor;

/// \brief A class to generate trends and trending plots
///
/// This class runs reductors to extract data points from objects, stores them
/// in a columnar data storage facility and runs plot (trends and correlations)
/// generation. Historically, it was extracted from TrendingTask.
///
/// \author Piotr Konopka
class TrendingMachine
{
 public:
  explicit TrendingMachine(const TrendingMachineConfig& config);
  ~TrendingMachine() = default;

  // initializes the trend, optionally with an existing tree if user needs to resume an old trend
  /// \brief Prepares the trend (TTree) to be filled.
  /// \param treeName the name of the new tree, if one has to be created
  /// \param existingTree optionally, a tree to continue (e.g. retrieved from a database). The tree is used only if
  ///        its structure matches the data sources, otherwise it is destroyed.
  /// \return true if the existing tree was taken over and the trend continues, false if a new trend was started
  bool initializeTrend(const std::string& treeName, std::unique_ptr<TTree> existingTree = nullptr);
  /// \brief returns a pointer to the trend tree, the ownership is not passed to the caller
  // unfortunately non-const, because of ObjectManager and MonitorObject
  TTree* getTrend() { return mTrend.get(); }

  /// \brief sets the mandatary metadata for the next trend entry
  void setMetadata(UInt_t timestamp, int runNumber);

  /// \brief updates the reductors of all data sources, with the data provided by the caller.
  /// \param updater a function which gets a data source and its reductor, and should update the latter, e.g. by
  ///        retrieving an object from a database. It should return true on success. Failures are logged.
  /// \return true if all data sources were updated successfully
  bool updateReductors(const std::function<bool(const TrendingMachineConfig::DataSource&, Reductor&)>& updater);
  /// \brief returns a const pointer to the reductor, the ownership is not passed to the caller
  const Reductor* getReductor(const std::string& dataSourceName) const;

  /// \brief fills the trend with a new row containing the current state of reductors
  void fillTrend();

  /// \brief generates pre-configured plots out of the latest trend
  std::vector<std::unique_ptr<TCanvas>> generatePlots();

  /// \brief resets the trend, clears the plots, but does not delete the trend tree (trending can resume)
  void reset();

 private:
  static bool canContinueTrend(TTree* tree, const std::vector<TrendingMachineConfig::DataSource>& dataSources);
  static TCanvas* drawPlot(TTree* trend, const TrendingMachineConfig::Plot& plotConfig);
  static void applyStyleToGraph(TGraph* graph, const TrendingMachineConfig::GraphStyle& style);
  static std::string deduceGraphLegendOptions(const TrendingMachineConfig::Graph& graphConfig);
  static void setUserAxesLabels(TAxis* xAxis, TAxis* yAxis, const std::string& graphAxesLabels);
  static void setUserYAxisRange(TH1* hist, const std::string& graphYAxisRange);
  static void formatTimeXAxis(TH1* background);
  static void formatRunNumberXAxis(TH1* background);

 private:
  static constexpr size_t MaxRunNumberStringLength = 6;
  struct {
    // we store run numbers both as an integer and as a string to allow users to select whether they need
    // a trend in integer or label domain (the latter will contain evenly-spaced data points)
    Long64_t runNumber = 0;
    char runNumberStr[MaxRunNumberStringLength + 1] = { 0 };
    static const char* getBranchLeafList()
    {
      return "runNumber/L:runNumberStr/C";
    }
  } mMetaData;

  std::unique_ptr<TTree> mTrend;
  UInt_t mTime;
  std::unordered_map<std::string, std::unique_ptr<Reductor>> mReductors;
  std::vector<TrendingMachineConfig::Plot> mPlotConfigs;
  std::vector<TrendingMachineConfig::DataSource> mDataSources;
};

} // namespace o2::quality_control::postprocessing

#endif // QUALITYCONTROL_TRENDINGMACHINE_H
