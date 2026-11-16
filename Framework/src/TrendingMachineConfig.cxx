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
/// \file   TrendingMachineConfig.cxx
/// \author Piotr Konopka
///

#include "QualityControl/TrendingMachineConfig.h"
#include <boost/property_tree/ptree.hpp>

namespace o2::quality_control::postprocessing
{

TrendingMachineConfig::TrendingMachineConfig(const boost::property_tree::ptree& config)
  : plots(parsePlots(config.get_child("plots"))),
    dataSources(parseDataSources(config.get_child("dataSources")))
{
}

TrendingMachineConfig::GraphStyle TrendingMachineConfig::parseGraphStyle(const boost::property_tree::ptree& styleTree)
{
  GraphStyle style;
  style.lineColor = styleTree.get<int>("lineColor", -1);
  style.lineStyle = styleTree.get<int>("lineStyle", -1);
  style.lineWidth = styleTree.get<int>("lineWidth", -1);
  style.markerColor = styleTree.get<int>("markerColor", -1);
  style.markerStyle = styleTree.get<int>("markerStyle", -1);
  style.markerSize = styleTree.get<float>("markerSize", -1.f);
  style.fillColor = styleTree.get<int>("fillColor", -1);
  style.fillStyle = styleTree.get<int>("fillStyle", -1);
  return style;
}

TrendingMachineConfig::LegendConfig TrendingMachineConfig::parseLegend(const boost::property_tree::ptree& legendTree)
{
  LegendConfig leg;
  leg.nColumns = legendTree.get<int>("nColumns", 1);
  leg.x1 = legendTree.get<float>("x1", -1.f);
  leg.y1 = legendTree.get<float>("y1", -1.f);
  leg.x2 = legendTree.get<float>("x2", -1.f);
  leg.y2 = legendTree.get<float>("y2", -1.f);
  return leg;
}

TrendingMachineConfig::Graph TrendingMachineConfig::parseGraph(const boost::property_tree::ptree& graphTree, const std::string& fallbackName)
{
  Graph graph;
  graph.name = graphTree.get<std::string>("name", graphTree.get<std::string>("title", fallbackName));
  graph.title = graphTree.get<std::string>("title", "");
  graph.varexp = graphTree.get<std::string>("varexp");
  graph.selection = graphTree.get<std::string>("selection", "");
  graph.option = graphTree.get<std::string>("option", "");
  graph.errors = graphTree.get<std::string>("graphErrors", "");

  // Parse style if present
  if (auto styleTree = graphTree.get_child_optional("style"); styleTree.has_value()) {
    graph.style = parseGraphStyle(styleTree.value());
  }

  return graph;
}

std::vector<TrendingMachineConfig::Plot> TrendingMachineConfig::parsePlots(const boost::property_tree::ptree& plotsTree)
{
  std::vector<Plot> plots;

  for (const auto& [_, plotConfig] : plotsTree) {
    std::vector<Graph> graphs;

    // Support both new format (with graphs array) and old format (direct plot config)
    if (const auto& graphsConfig = plotConfig.get_child_optional("graphs"); graphsConfig.has_value()) {
      for (const auto& [_, graphConfig] : graphsConfig.value()) {
        graphs.push_back(parseGraph(graphConfig, plotConfig.get<std::string>("name")));
      }
    } else {
      // Old format: plot config is also graph config
      graphs.push_back(parseGraph(plotConfig, plotConfig.get<std::string>("name", "")));
    }

    Plot plot;
    plot.name = plotConfig.get<std::string>("name");
    plot.title = plotConfig.get<std::string>("title", "");
    plot.graphAxisLabel = plotConfig.get<std::string>("graphAxisLabel", "");
    plot.graphYRange = plotConfig.get<std::string>("graphYRange", "");
    plot.colorPalette = plotConfig.get<int>("colorPalette", 0);

    if (auto legendTree = plotConfig.get_child_optional("legend"); legendTree.has_value()) {
      plot.legend = parseLegend(legendTree.value());
    }

    plot.graphs = graphs;
    plots.push_back(plot);
  }

  return plots;
}

std::vector<TrendingMachineConfig::DataSource> TrendingMachineConfig::parseDataSources(const boost::property_tree::ptree& dataSourcesTree)
{
  std::vector<DataSource> dataSources;

  const auto extractReductorParams = [](const boost::property_tree::ptree& dataSourceConfig) -> core::CustomParameters {
    core::CustomParameters result;
    if (const auto reductorParams = dataSourceConfig.get_child_optional("reductorParameters"); reductorParams.has_value()) {
      result.populateCustomParameters(reductorParams.value());
    }
    return result;
  };

  for (const auto& dataSourceEntry : dataSourcesTree) {
    const auto& dataSourceConfig = dataSourceEntry.second;

    // Support both "names" array and single "name"
    if (const auto& sourceNames = dataSourceConfig.get_child_optional("names"); sourceNames.has_value()) {
      for (const auto& sourceName : sourceNames.value()) {
        DataSource ds;
        ds.type = dataSourceConfig.get<std::string>("type", "repository");
        ds.path = dataSourceConfig.get<std::string>("path", "");
        ds.name = sourceName.second.data();
        ds.reductorName = dataSourceConfig.get<std::string>("reductorName");
        ds.reductorParameters = extractReductorParams(dataSourceConfig);
        ds.moduleName = dataSourceConfig.get<std::string>("moduleName");
        dataSources.push_back(ds);
      }
    } else {
      std::string nameValue = dataSourceConfig.get<std::string>("name", "");
      if (!nameValue.empty()) {
        DataSource ds;
        ds.type = dataSourceConfig.get<std::string>("type", "repository");
        ds.path = dataSourceConfig.get<std::string>("path", "");
        ds.name = nameValue;
        ds.reductorName = dataSourceConfig.get<std::string>("reductorName");
        ds.reductorParameters = extractReductorParams(dataSourceConfig);
        ds.moduleName = dataSourceConfig.get<std::string>("moduleName");
        dataSources.push_back(ds);
      } else {
        throw std::runtime_error("No 'name' value or a 'names' vector in dataSources configuration");
      }
    }
  }

  return dataSources;
}

} // namespace o2::quality_control::postprocessing
