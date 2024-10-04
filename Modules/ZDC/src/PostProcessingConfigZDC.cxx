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
/// \file    PostProcessingConfigZDC.cxx
/// \author  Andrea Ferrero andrea.ferrero@cern.ch
/// \brief   File for the configuration of ZDC post-processing tasks
/// \since   30/08/2023
///

#include "ZDC/PostProcessingConfigZDC.h"
#include <boost/property_tree/ptree.hpp>

using namespace o2::quality_control::postprocessing;
namespace o2::quality_control_modules::zdc
{

PostProcessingConfigZDC::PostProcessingConfigZDC(std::string name, const boost::property_tree::ptree& config)
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
  // ADC
  for (const auto& dataSourceConfig : config.get_child("qc.postprocessing." + name + ".dataSourcesADC")) {
    if (const auto& sourceNames = dataSourceConfig.second.get_child_optional("names"); sourceNames.has_value()) {
      for (const auto& sourceName : sourceNames.value()) {
        dataSourcesADC.push_back({ dataSourceConfig.second.get<std::string>("path"),
                                   sourceName.second.data() });
      }
    } else if (!dataSourceConfig.second.get<std::string>("name").empty()) {
      // "name" : [ "something" ] would return an empty string here
      dataSourcesADC.push_back({ dataSourceConfig.second.get<std::string>("path"),
                                 dataSourceConfig.second.get<std::string>("name") });
    } else {
      throw std::runtime_error("No 'name' value or a 'names' vector in the path 'qc.postprocessing." + name + ".dataSourcesADC'");
    }
  }
  // TDC Time
  for (const auto& dataSourceConfig : config.get_child("qc.postprocessing." + name + ".dataSourcesTDC")) {
    if (const auto& sourceNames = dataSourceConfig.second.get_child_optional("names"); sourceNames.has_value()) {
      for (const auto& sourceName : sourceNames.value()) {
        dataSourcesTDC.push_back({ dataSourceConfig.second.get<std::string>("path"),
                                   sourceName.second.data() });
      }
    } else if (!dataSourceConfig.second.get<std::string>("name").empty()) {
      // "name" : [ "something" ] would return an empty string here
      dataSourcesTDC.push_back({ dataSourceConfig.second.get<std::string>("path"),
                                 dataSourceConfig.second.get<std::string>("name") });
    } else {
      throw std::runtime_error("No 'name' value or a 'names' vector in the path 'qc.postprocessing." + name + ".dataSourcesTDC'");
    }
  }
  // TDC Amplitude
  for (const auto& dataSourceConfig : config.get_child("qc.postprocessing." + name + ".dataSourcesTDCA")) {
    if (const auto& sourceNames = dataSourceConfig.second.get_child_optional("names"); sourceNames.has_value()) {
      for (const auto& sourceName : sourceNames.value()) {
        dataSourcesTDCA.push_back({ dataSourceConfig.second.get<std::string>("path"),
                                    sourceName.second.data() });
      }
    } else if (!dataSourceConfig.second.get<std::string>("name").empty()) {
      // "name" : [ "something" ] would return an empty string here
      dataSourcesTDCA.push_back({ dataSourceConfig.second.get<std::string>("path"),
                                  dataSourceConfig.second.get<std::string>("name") });
    } else {
      throw std::runtime_error("No 'name' value or a 'names' vector in the path 'qc.postprocessing." + name + ".dataSourcesTDCA'");
    }
  }
  // Peak 1n TDC Amplitude
  for (const auto& dataSourceConfig : config.get_child("qc.postprocessing." + name + ".dataSourcesPeak1n")) {
    if (const auto& sourceNames = dataSourceConfig.second.get_child_optional("names"); sourceNames.has_value()) {
      for (const auto& sourceName : sourceNames.value()) {
        dataSourcesPeak1n.push_back({ dataSourceConfig.second.get<std::string>("path"),
                                      sourceName.second.data() });
      }
    } else if (!dataSourceConfig.second.get<std::string>("name").empty()) {
      // "name" : [ "something" ] would return an empty string here
      dataSourcesPeak1n.push_back({ dataSourceConfig.second.get<std::string>("path"),
                                    dataSourceConfig.second.get<std::string>("name") });
    } else {
      throw std::runtime_error("No 'name' value or a 'names' vector in the path 'qc.postprocessing." + name + ".dataSourcesPeak1n'");
    }
  }
  // Peak 1p TDC Amplitude
  for (const auto& dataSourceConfig : config.get_child("qc.postprocessing." + name + ".dataSourcesPeak1p")) {
    if (const auto& sourceNames = dataSourceConfig.second.get_child_optional("names"); sourceNames.has_value()) {
      for (const auto& sourceName : sourceNames.value()) {
        dataSourcesPeak1p.push_back({ dataSourceConfig.second.get<std::string>("path"),
                                      sourceName.second.data() });
      }
    } else if (!dataSourceConfig.second.get<std::string>("name").empty()) {
      // "name" : [ "something" ] would return an empty string here
      dataSourcesPeak1p.push_back({ dataSourceConfig.second.get<std::string>("path"),
                                    dataSourceConfig.second.get<std::string>("name") });
    } else {
      throw std::runtime_error("No 'name' value or a 'names' vector in the path 'qc.postprocessing." + name + ".dataSourcesPeak1p'");
    }
  }
}

} // namespace o2::quality_control_modules::zdc
