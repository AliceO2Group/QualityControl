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
/// \file    TrendingConfigTOF.cxx
/// \author  Nicolò Jacazio nicolo.jacazio@cern.ch
/// \author  Francesca Ercolessi francesca.ercolessi@cern.ch
/// \brief   File for the trending task configuration for the number of hits in TOF
/// \since   05/10/2021
///

#include "TOF/TrendingConfigTOF.h"
#include "QualityControl/QcInfoLogger.h"
#include <boost/property_tree/ptree.hpp>

using namespace o2::quality_control::postprocessing;
namespace o2::quality_control_modules::tof
{

TrendingConfigTOF::TrendingConfigTOF(std::string id, const boost::property_tree::ptree& config)
  : PostProcessingConfig(id, config)
{
  if (const auto& customConfigs = config.get_child_optional("qc.postprocessing." + id + ".customization"); customConfigs.has_value()) {
    for (const auto& customConfig : customConfigs.value()) { // Plot configuration
      ILOG(Info, Support) << "Reading configuration " << customConfig.second.get<std::string>("name") << ENDM;
      if (const auto& customNames = customConfig.second.get_child_optional("name"); customNames.has_value()) {
        if (customConfig.second.get<std::string>("name") == "ThresholdSgn") {
          mConfigTrendingRate.thresholdSignal = customConfig.second.get<float>("value");
          ILOG(Info, Support) << "Setting thresholdSignal to " << mConfigTrendingRate.thresholdSignal << ENDM;
        } else if (customConfig.second.get<std::string>("name") == "ThresholdBkg") {
          mConfigTrendingRate.thresholdBackground = customConfig.second.get<float>("value");
          ILOG(Info, Support) << "Setting thresholdBackground to " << mConfigTrendingRate.thresholdBackground << ENDM;
        }
      }
    }
  }
  for (const auto& plotConfig : config.get_child("qc.postprocessing." + id + ".plots")) { // Plot configuration
    plots.push_back({ plotConfig.second.get<std::string>("name"),
                      plotConfig.second.get<std::string>("title", ""),
                      plotConfig.second.get<std::string>("varexp"),
                      plotConfig.second.get<std::string>("selection", ""),
                      plotConfig.second.get<std::string>("option", ""),
                      plotConfig.second.get<std::string>("graphErrors", "") });
  }
  for (const auto& dataSourceConfig : config.get_child("qc.postprocessing." + id + ".dataSources")) { // Data source configuration
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

} // namespace o2::quality_control_modules::tof
