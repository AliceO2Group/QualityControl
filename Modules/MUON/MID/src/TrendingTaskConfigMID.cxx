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
/// \file   TrendingTaskConfigMID.cxx
/// \author Valerie Ramillien       based on Piotr Konopka work
///

#include "MID/TrendingTaskConfigMID.h"
#include <boost/property_tree/ptree.hpp>
using boost::property_tree::ptree;
namespace o2::quality_control::postprocessing
{

TrendingTaskConfigMID::TrendingTaskConfigMID(std::string id, const boost::property_tree::ptree& config)
  : PostProcessingConfig(id, config)
{

  for (const auto& plotConfig : config.get_child("qc.postprocessing." + id + ".plots")) {

    if (const auto& sourceNames = plotConfig.second.get_child_optional("names");
        sourceNames.has_value()) {
      const auto& sourceVarexps =
        plotConfig.second.get_child_optional("varexp"); // take all varexps
      const auto& sourceTitles =
        plotConfig.second.get_child_optional("title"); // take all titles

      ptree::const_iterator itname = sourceNames.value().begin();
      ptree::const_iterator itexp = sourceVarexps.value().begin();
      ptree::const_iterator ittitle = sourceTitles.value().begin();

      while (itname != sourceNames.value().end() ||
             itexp != sourceVarexps.value().end() ||
             ittitle != sourceTitles.value().end()) {
        plots.push_back({ itname->second.data(), ittitle->second.data(),
                          itexp->second.data(),
                          plotConfig.second.get<std::string>("selection", ""),
                          plotConfig.second.get<std::string>("option", ""),
                          plotConfig.second.get<std::string>("graphErrors", "") });

        itname++;
        itexp++;
        ittitle++;
      }
    }
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
