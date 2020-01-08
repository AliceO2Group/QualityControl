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

#include "QualityControl/TTreeTrendConfig.h"
#include <Configuration/ConfigurationInterface.h>

namespace o2::quality_control::postprocessing
{

TTreeTrendConfig::TTreeTrendConfig(std::string name, configuration::ConfigurationInterface& config)
  : PostProcessingConfig(name, config)
{
  for (const auto& plotConfig : config.getRecursive("qc.postprocessing." + name + ".plots")) {
    plots.push_back({ plotConfig.second.get<std::string>("name"),
                      plotConfig.second.get<std::string>("title", ""),
                      plotConfig.second.get<std::string>("varexp"),
                      plotConfig.second.get<std::string>("selection", ""),
                      plotConfig.second.get<std::string>("option", "") });
  }
  for (const auto& dataSourceConfig : config.getRecursive("qc.postprocessing." + name + ".dataSources")) {
    dataSources.push_back({ dataSourceConfig.second.get<std::string>("type", "repository"),
                            dataSourceConfig.second.get<std::string>("path"),
                            dataSourceConfig.second.get<std::string>("name"),
                            dataSourceConfig.second.get<std::string>("reductorName", ""),
                            dataSourceConfig.second.get<std::string>("moduleName", "") });
  }
}

} // namespace o2::quality_control::postprocessing