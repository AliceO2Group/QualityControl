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
/// \file   TrendingMachineConfig.h
/// \author Piotr Konopka
///

#ifndef QUALITYCONTROL_TRENDINGMACHINECONFIG_H
#define QUALITYCONTROL_TRENDINGMACHINECONFIG_H

#include <vector>
#include <string>
#include <boost/property_tree/ptree_fwd.hpp>
#include "CustomParameters.h"

namespace o2::quality_control::postprocessing
{

/// \brief  Configuration of the TrendingMachine: what to trend (data sources) and what to plot.
/// It does not contain anything related to when and how the machine is triggered or fed -
/// that is up to the callers (e.g. TrendingTaskConfig).
struct TrendingMachineConfig {
  TrendingMachineConfig() = default;
  /// \param config a node which contains the "plots" and "dataSources" children
  explicit TrendingMachineConfig(const boost::property_tree::ptree& config);
  ~TrendingMachineConfig() = default;

  // graph style configuration
  // colors as defined by ROOT TColor class:
  // https://root.cern/doc/master/classTColor.html
  // marker colors and styles are as defined by ROOT TAttMarker class
  // https://root.cern/doc/master/classTAttMarker.html
  // line styles are as defined by ROOT TAttLine class
  // https://root.cern/doc/master/classTAttLine.html
  // WARNING: Any parameters in this struct will override colliding parameters in option
  struct GraphStyle {
    int lineColor = -1;
    int lineStyle = -1;
    int lineWidth = -1;
    int markerColor = -1;
    int markerStyle = -1;
    float markerSize = -1.f;
    int fillColor = -1;
    int fillStyle = -1;
  };

  // this corresponds to one TTree::Draw() call, i.e. one graph or histogram drawing
  struct Graph {
    std::string name;
    std::string title;
    std::string varexp;
    std::string selection;
    std::string option; // the list of possible options are documented in TGraphPainter and THistPainter
    std::string errors;
    GraphStyle style;
  };

  // legend configuration
  struct LegendConfig {
    int nColumns{ 1 };
    float x1{ -1.f }, y1{ -1.f }, x2{ -1.f }, y2{ -1.f }; // NDC coords
  };

  // this corresponds to one canvas which can include multiple graphs
  struct Plot {
    std::string name;
    std::string title;
    std::string graphAxisLabel;
    std::string graphYRange;
    int colorPalette = 0;
    LegendConfig legend;
    std::vector<Graph> graphs;
  };

  struct DataSource {
    std::string type;
    std::string path;
    std::string name;
    std::string reductorName;
    core::CustomParameters reductorParameters;
    std::string moduleName;
  };

  std::vector<Plot> plots;
  std::vector<DataSource> dataSources;

  // Static parsing methods, usable also when plots and data sources are not siblings in one tree
  static std::vector<Plot> parsePlots(const boost::property_tree::ptree& plotsTree);
  static std::vector<DataSource> parseDataSources(const boost::property_tree::ptree& dataSourcesTree);
  static Graph parseGraph(const boost::property_tree::ptree& graphTree, const std::string& fallbackName = "");
  static GraphStyle parseGraphStyle(const boost::property_tree::ptree& styleTree);
  static LegendConfig parseLegend(const boost::property_tree::ptree& legendTree);
};

} // namespace o2::quality_control::postprocessing

#endif // QUALITYCONTROL_TRENDINGMACHINECONFIG_H
