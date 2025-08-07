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
/// \file   TrendingTaskConfig.cxx
/// \author Piotr Konopka
///

#include "QualityControl/TrendingTaskConfig.h"
#include <boost/property_tree/ptree.hpp>

namespace o2::quality_control::postprocessing
{

TrendingTaskConfig::TrendingTaskConfig(std::string id, const boost::property_tree::ptree& config)
  : PostProcessingConfig(id, config)
{
  producePlotsOnUpdate = config.get<bool>("qc.postprocessing." + id + ".producePlotsOnUpdate", true);
  resumeTrend = config.get<bool>("qc.postprocessing." + id + ".resumeTrend", false);
  trendIfAllInputs = config.get<bool>("qc.postprocessing." + id + ".trendIfAllInputs", false);
  trendingTimestamp = config.get<std::string>("qc.postprocessing." + id + ".trendingTimestamp", "validUntil");

  for (const auto& [_, plotConfig] : config.get_child("qc.postprocessing." + id + ".plots")) {
    // since QC-1155 we allow for more than one graph in a single plot (canvas). we support both the new and old ways
    // of configuring the expected plots.
    std::vector<Graph> graphs;
    if (const auto& graphsConfig = plotConfig.get_child_optional("graphs"); graphsConfig.has_value()) {
      for (const auto& [_, graphConfig] : graphsConfig.value()) {
        // first we use name of the graph, if absent, we use graph title, if absent, we use plot (object) name.
        const auto& name = graphConfig.get<std::string>("name", graphConfig.get<std::string>("title", plotConfig.get<std::string>("name")));
        GraphStyle style;
        style.lineColor = graphConfig.get<int>("style.lineColor", -1);
        style.lineStyle = graphConfig.get<int>("style.lineStyle", -1);
        style.lineWidth = graphConfig.get<int>("style.lineWidth", -1);
        style.markerColor = graphConfig.get<int>("style.markerColor", -1);
        style.markerStyle = graphConfig.get<int>("style.markerStyle", -1);
        style.markerSize = graphConfig.get<float>("style.markerSize", -1.f);
        style.fillColor = graphConfig.get<int>("style.fillColor", -1);
        style.fillStyle = graphConfig.get<int>("style.fillStyle", -1);

        graphs.push_back({ name,
                           graphConfig.get<std::string>("title", ""),
                           graphConfig.get<std::string>("varexp"),
                           graphConfig.get<std::string>("selection", ""),
                           graphConfig.get<std::string>("option", ""),
                           graphConfig.get<std::string>("graphErrors", ""),
                           style });
      }
    } else {
      GraphStyle style;
      style.lineColor = plotConfig.get<int>("style.lineColor", -1);
      style.lineStyle = plotConfig.get<int>("style.lineStyle", -1);
      style.lineWidth = plotConfig.get<int>("style.lineWidth", -1);
      style.markerColor = plotConfig.get<int>("style.markerColor", -1);
      style.markerStyle = plotConfig.get<int>("style.markerStyle", -1);
      style.markerSize = plotConfig.get<float>("style.markerSize", -1.f);
      style.fillColor = plotConfig.get<int>("style.fillColor", -1);
      style.fillStyle = plotConfig.get<int>("style.fillStyle", -1);
      graphs.push_back({ plotConfig.get<std::string>("name", ""),
                         plotConfig.get<std::string>("title", ""),
                         plotConfig.get<std::string>("varexp"),
                         plotConfig.get<std::string>("selection", ""),
                         plotConfig.get<std::string>("option", ""),
                         plotConfig.get<std::string>("graphErrors", "") });
    }

    LegendConfig leg;
    leg.nColumns = plotConfig.get<int>("legend.nColumns", 1);
    leg.x1 = plotConfig.get<float>("legend.x1", 0.30f);
    leg.y1 = plotConfig.get<float>("legend.y1", 0.20f);
    leg.x2 = plotConfig.get<float>("legend.x2", 0.55f);
    leg.y2 = plotConfig.get<float>("legend.y2", 0.35f);

    plots.push_back({ plotConfig.get<std::string>("name"),
                      plotConfig.get<std::string>("title", ""),
                      plotConfig.get<std::string>("graphAxisLabel", ""),
                      plotConfig.get<std::string>("graphYRange", ""),
                      plotConfig.get<int>("colorPalette", 0),
                      leg,
                      graphs });
  }

  const auto extractReductorParams = [](const boost::property_tree::ptree& dataSourceConfig) -> core::CustomParameters {
    core::CustomParameters result;
    if (const auto reductorParams = dataSourceConfig.get_child_optional("reductorParameters"); reductorParams.has_value()) {
      result.populateCustomParameters(reductorParams.value());
    }
    return result;
  };

  for (const auto& dataSourceConfig : config.get_child("qc.postprocessing." + id + ".dataSources")) {
    if (const auto& sourceNames = dataSourceConfig.second.get_child_optional("names"); sourceNames.has_value()) {
      for (const auto& sourceName : sourceNames.value()) {
        dataSources.push_back({ dataSourceConfig.second.get<std::string>("type", "repository"),
                                dataSourceConfig.second.get<std::string>("path"),
                                sourceName.second.data(),
                                dataSourceConfig.second.get<std::string>("reductorName"),
                                extractReductorParams(dataSourceConfig.second),
                                dataSourceConfig.second.get<std::string>("moduleName") });
      }
    } else if (!dataSourceConfig.second.get<std::string>("name").empty()) {
      // "name" : [ "something" ] would return an empty string here
      dataSources.push_back({ dataSourceConfig.second.get<std::string>("type", "repository"),
                              dataSourceConfig.second.get<std::string>("path"),
                              dataSourceConfig.second.get<std::string>("name"),
                              dataSourceConfig.second.get<std::string>("reductorName"),
                              extractReductorParams(dataSourceConfig.second),
                              dataSourceConfig.second.get<std::string>("moduleName") });
    } else {
      throw std::runtime_error("No 'name' value or a 'names' vector in the path 'qc.postprocessing." + id + ".dataSources'");
    }
  }
}

} // namespace o2::quality_control::postprocessing
