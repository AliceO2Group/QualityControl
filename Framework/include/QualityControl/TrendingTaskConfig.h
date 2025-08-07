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
/// \file   TrendingTaskConfig.h
/// \author Piotr Konopka
///

#ifndef QUALITYCONTROL_TRENDINGTASKCONFIG_H
#define QUALITYCONTROL_TRENDINGTASKCONFIG_H

#include <vector>
#include <string>
#include "CustomParameters.h"
#include "QualityControl/PostProcessingConfig.h"

namespace o2::quality_control::postprocessing
{

// todo pretty print
/// \brief  TrendingTask configuration structure
struct TrendingTaskConfig : PostProcessingConfig {
  TrendingTaskConfig() = default;
  TrendingTaskConfig(std::string name, const boost::property_tree::ptree& config);
  ~TrendingTaskConfig() = default;

  // graph style configuration
  // colors as defined by ROOT TColor class:
  // https://root.cern/doc/master/classTColor.html
  // marker colors and styles are as defined by ROOT TAttMarker class
  // https://root.cern/doc/master/classTAttMarker.html
  // line styles are as defined by ROOT TAttLine class
  // https://root.cern/doc/master/classTAttLine.html
  // WARNING: Any parameters in this struct will override colliding parameters in option
  struct GraphStyle {
    int   lineColor   = -1;
    int   lineStyle   = -1;
    int   lineWidth   = -1;
    int   markerColor = -1;
    int   markerStyle = -1;
    float markerSize  = -1.f;
    int   fillColor   = -1;
    int   fillStyle   = -1;
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
    bool  enabled{true};
    int   nColumns{1};
    float x1{0.30f}, y1{0.20f}, x2{0.55f}, y2{0.35f}; // NDC coords
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

  bool producePlotsOnUpdate{};
  bool resumeTrend{};
  bool trendIfAllInputs{ false };
  std::string trendingTimestamp;
  std::vector<Plot> plots;
  std::vector<DataSource> dataSources;
};

} // namespace o2::quality_control::postprocessing

#endif // QUALITYCONTROL_TRENDINGTASKCONFIG_H
