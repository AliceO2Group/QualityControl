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
  for (const auto& [_, plotConfig] : config.get_child("qc.postprocessing." + id + ".plots")) {
    // since QC-1155 we allow for more than one graph in a single plot (canvas). we support both the new and old ways
    // of configuring the expected plots.
    std::vector<Graph> graphs;
    if (const auto& graphsConfig = plotConfig.get_child_optional("graphs"); graphsConfig.has_value()) {
      for (const auto& [_, graphConfig] : graphsConfig.value()) {
        // first we use name of the graph, if absent, we use graph title, if absent, we use plot (object) name.
        const auto& name = graphConfig.get<std::string>("name", graphConfig.get<std::string>("title", plotConfig.get<std::string>("name")));
        graphs.push_back({ name,
                           graphConfig.get<std::string>("title", ""),
                           graphConfig.get<std::string>("varexp"),
                           graphConfig.get<std::string>("selection", ""),
                           graphConfig.get<std::string>("option", ""),
                           graphConfig.get<std::string>("graphErrors", "") });
      }
    } else {
      graphs.push_back({ plotConfig.get<std::string>("name", ""),
                         plotConfig.get<std::string>("title", ""),
                         plotConfig.get<std::string>("varexp"),
                         plotConfig.get<std::string>("selection", ""),
                         plotConfig.get<std::string>("option", ""),
                         plotConfig.get<std::string>("graphErrors", "") });
    }
    plots.push_back({ plotConfig.get<std::string>("name"),
                      plotConfig.get<std::string>("title", ""),
                      plotConfig.get<std::string>("graphAxisLabel", ""),
                      plotConfig.get<std::string>("graphYRange", ""),
                      plotConfig.get<int>("colorPalette", 0),
                      graphs });
  }
  for (const auto& dataSourceConfig : config.get_child("qc.postprocessing." + id + ".dataSources")) {
    if (const auto& sourceNames = dataSourceConfig.second.get_child_optional("names"); sourceNames.has_value()) {
      for (const auto& sourceName : sourceNames.value()) {
        dataSources.push_back({ dataSourceConfig.second.get<std::string>("type", "repository"),
                                dataSourceConfig.second.get<std::string>("path"),
                                sourceName.second.data(),
                                dataSourceConfig.second.get<std::string>("reductorName"),
                                dataSourceConfig.second.get<std::string>("moduleName") });
      }
    } else if (!dataSourceConfig.second.get<std::string>("name").empty()) {
      // "name" : [ "something" ] would return an empty string here
      dataSources.push_back({ dataSourceConfig.second.get<std::string>("type", "repository"),
                              dataSourceConfig.second.get<std::string>("path"),
                              dataSourceConfig.second.get<std::string>("name"),
                              dataSourceConfig.second.get<std::string>("reductorName"),
                              dataSourceConfig.second.get<std::string>("moduleName") });
    } else {
      throw std::runtime_error("No 'name' value or a 'names' vector in the path 'qc.postprocessing." + id + ".dataSources'");
    }
  }
}

} // namespace o2::quality_control::postprocessing
