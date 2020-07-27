// Copyright CERN and copyright holders of ALICE O2. This software is
// distributed under the terms of the GNU General Public License v3 (GPL
// Version 3), copied verbatim in the file "COPYING".
//
// See http://alice-o2.web.cern.ch/license for full licensing information.
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.

///
/// \file   TrendingTaskConfig.cxx
/// \author Piotr Konopka
///

#include "ITS/TrendingTaskConfigITS.h"
#include <boost/property_tree/ptree.hpp>

using boost::property_tree::ptree;

namespace o2::quality_control::postprocessing
{

TrendingTaskConfigITS::TrendingTaskConfigITS(
  std::string name, const boost::property_tree::ptree& config)
  : PostProcessingConfig(name, config)
{
  for (const auto& plotConfig :
       config.get_child("qc.postprocessing." + name + ".plots")) {
    if (const auto& sourceNames = plotConfig.second.get_child_optional("names");
        sourceNames.has_value()) {
      const auto& sourceVarexps =
        plotConfig.second.get_child_optional("varexp"); // take all varexps
      const auto& sourceTitles =
        plotConfig.second.get_child_optional("title"); // take all titles
      ptree::const_iterator itname = sourceNames.value().begin();
      ptree::const_iterator itexp = sourceVarexps.value().begin();
      ptree::const_iterator ittitle = sourceTitles.value().begin();

      // for (const auto& sourceName : sourceNames.value() && const auto&
      // sourceVarexp : sourceVarexps.value() && const auto& sourceTitle :
      // sourceTitles.value()) {
      while (itname != sourceNames.value().end() ||
             itexp != sourceVarexps.value().end() ||
             ittitle != sourceTitles.value().end()) {
        plots.push_back({ itname->second.data(), ittitle->second.data(),
                          itexp->second.data(),
                          plotConfig.second.get<std::string>("selection", ""),
                          plotConfig.second.get<std::string>("option", "") });
        itname++;
        itexp++;
        ittitle++;
      }
    }
  }
  for (const auto& dataSourceConfig :
       config.get_child("qc.postprocessing." + name + ".dataSources")) {
    if (const auto& sourceNames = dataSourceConfig.second.get_child_optional("names");
        sourceNames.has_value()) {
      const auto& sourcePaths = dataSourceConfig.second.get_child_optional("paths"); // take all paths
      ptree::const_iterator itname = sourceNames.value().begin();
      ptree::const_iterator itpath = sourcePaths.value().begin();
      while (itname != sourceNames.value().end() ||
             itpath != sourcePaths.value().end()) {
        dataSources.push_back({ dataSourceConfig.second.get<std::string>("type",""),
                                itpath->second.data(),
                                itname->second.data(),
                                dataSourceConfig.second.get<std::string>("reductorName",""),
                                dataSourceConfig.second.get<std::string>("moduleName","") });
        itpath++;
        itname++;
      }
    } else if (!dataSourceConfig.second.get<std::string>("name").empty()) {
      // "name" : [ "something" ] would return an empty string here
      dataSources.push_back(
        { dataSourceConfig.second.get<std::string>("type", "repository"),
          dataSourceConfig.second.get<std::string>("path"),
          dataSourceConfig.second.get<std::string>("name"),
          dataSourceConfig.second.get<std::string>("reductorName"),
          dataSourceConfig.second.get<std::string>("moduleName") });
    } else {
      throw std::runtime_error(
        "No 'name' value or a 'names' vector in the "
        "path 'qc.postprocessing." +
        name + ".dataSources'");
    }
  }
}

} // namespace o2::quality_control::postprocessing
