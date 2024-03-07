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
/// \file    TracksPostProcessingConfig.cxx
/// \author  Andrea Ferrero andrea.ferrero@cern.ch
/// \brief   File for the configuration of MCH post-processing tasks
/// \since   05/10/2021
///

#include "MUONCommon/TracksPostProcessingConfig.h"
#include <boost/property_tree/ptree.hpp>
#include <iostream>

using namespace o2::quality_control::postprocessing;
namespace o2::quality_control_modules::muon
{

TracksPostProcessingConfig::TracksPostProcessingConfig(std::string name, const boost::property_tree::ptree& config)
  : PostProcessingConfig(name, config)
{
  // parameters
  if (const auto& customConfigs = config.get_child_optional("qc.postprocessing." + name + ".customization"); customConfigs.has_value()) {
    for (const auto& customConfig : customConfigs.value()) {
      if (const auto& customNames = customConfig.second.get_child_optional("name"); customNames.has_value()) {
        parameters.insert(std::make_pair(customConfig.second.get<std::string>("name"), customConfig.second.get<std::string>("value")));
      }
    }
  }

  // Data source configuration
  for (const auto& dataSourceConfig : config.get_child("qc.postprocessing." + name + ".dataSources")) {
    if (const auto& sourceNames = dataSourceConfig.second.get_child_optional("names"); sourceNames.has_value()) {
      auto plotsPath = dataSourceConfig.second.get<std::string>("plotsPath");
      auto refsPath = dataSourceConfig.second.get<std::string>("refsPath");
      auto outputPath = dataSourceConfig.second.get<std::string>("outputPath");
      for (const auto& sourceName : sourceNames.value()) {
        int rebin = 1;
        std::string nameFull = sourceName.second.data();
        std::string name = nameFull;
        auto pos = name.find(":");
        if (pos < std::string::npos) {
          name = nameFull.substr(0, pos);
          auto rebinStr = nameFull.substr(pos + 1);
          rebin = std::stoi(rebinStr);
        }
        dataSources.push_back({ plotsPath, refsPath, outputPath, name, rebin });
      }
    } else {
      throw std::runtime_error("No 'name' value or a 'names' vector in the path 'qc.postprocessing." + name + ".dataSources'");
    }
  }
}

template <>
const std::string TracksPostProcessingConfig::getParameter(std::string name) const
{
  std::string result;
  auto entry = parameters.find(name);
  if (entry != parameters.end()) {
    result = entry->second;
  }

  return result;
}

template <>
const std::string TracksPostProcessingConfig::getParameter(std::string name, std::string defaultValue) const
{
  std::string result = defaultValue;
  auto entry = parameters.find(name);
  if (entry != parameters.end()) {
    result = entry->second;
  }

  return result;
}

} // namespace o2::quality_control_modules::muon
