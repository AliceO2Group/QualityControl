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
/// \file     SliceTrendingTaskConfig.cxx
/// \author   Marcel Lesch
/// \author   Cindy Mordasini
/// \author   Based on the work from Piotr Konopka
///

#include "QualityControl/SliceTrendingTaskConfig.h"
#include <boost/property_tree/ptree.hpp>

namespace o2::quality_control::postprocessing
{

SliceTrendingTaskConfig::SliceTrendingTaskConfig(const std::string& id,
                                                 const boost::property_tree::ptree& config)
  : PostProcessingConfig(id, config)
{
  producePlotsOnUpdate = config.get<bool>("qc.postprocessing." + id + ".producePlotsOnUpdate", true);
  resumeTrend = config.get<bool>("qc.postprocessing." + id + ".resumeTrend", false);
  for (const auto& plotConfig : config.get_child("qc.postprocessing." + id + ".plots")) {
    plots.push_back({ plotConfig.second.get<std::string>("name"),
                      plotConfig.second.get<std::string>("title", ""),
                      plotConfig.second.get<std::string>("varexp"),
                      plotConfig.second.get<std::string>("selection", ""),
                      plotConfig.second.get<std::string>("option", ""),
                      plotConfig.second.get<std::string>("graphErrors", ""),
                      plotConfig.second.get<std::string>("graphYRange", ""),
                      plotConfig.second.get<std::string>("graphXRange", ""),
                      plotConfig.second.get<std::string>("graphAxisLabel", ""),
                      plotConfig.second.get<std::string>("legendNColums", "2"),
                      plotConfig.second.get<std::string>("legendTextSize", "2.0"),
                      plotConfig.second.get<std::string>("legendObservableX", ""),
                      plotConfig.second.get<std::string>("legendObservableY", ""),
                      plotConfig.second.get<std::string>("legendUnitX", ""),
                      plotConfig.second.get<std::string>("legendUnitY", ""),
                      plotConfig.second.get<std::string>("legendCentmodeX", "False"),
                      plotConfig.second.get<std::string>("legendCentmodeY", "False") });
  }

  // Loop over all the data sources to trend.
  for (const auto& dataSourceConfig : config.get_child("qc.postprocessing." + id + ".dataSources")) {
    // Prepare the vector(vector) for the slicing.
    std::vector<std::vector<float>> axisBoundaries;
    std::vector<float> singleAxis;

    if (const auto& multiAxisValues = dataSourceConfig.second.get_child_optional("axisDivision"); multiAxisValues.has_value()) {
      for (const auto& multiAxisValue : multiAxisValues.value()) {
        for (const auto& axis : multiAxisValue.second) {
          singleAxis.push_back(std::stof(axis.second.data()));
        }
        axisBoundaries.push_back(singleAxis);
        singleAxis.clear();
      }
    }

    // Parse the vector of "names" or just get the "name" of sources.
    if (const auto& sourceNames = dataSourceConfig.second.get_child_optional("names"); sourceNames.has_value()) {
      for (const auto& sourceName : sourceNames.value()) {
        dataSources.push_back({ dataSourceConfig.second.get<std::string>("type", "repository"),
                                dataSourceConfig.second.get<std::string>("path"),
                                sourceName.second.data(),
                                dataSourceConfig.second.get<std::string>("reductorName"),
                                axisBoundaries,
                                dataSourceConfig.second.get<std::string>("moduleName") });
      }
    } else if (!dataSourceConfig.second.get<std::string>("name").empty()) {
      // "name" : [ "something" ] would return an empty string here.
      dataSources.push_back({ dataSourceConfig.second.get<std::string>("type", "repository"),
                              dataSourceConfig.second.get<std::string>("path"),
                              dataSourceConfig.second.get<std::string>("name"),
                              dataSourceConfig.second.get<std::string>("reductorName"),
                              axisBoundaries,
                              dataSourceConfig.second.get<std::string>("moduleName") });
    } else {
      throw std::runtime_error("No 'name' value or a 'names' vector in the path 'qc.postprocessing." + id + ".dataSources'");
    }

    axisBoundaries.clear();
  }
}

} // namespace o2::quality_control::postprocessing
