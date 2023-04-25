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
/// \file   BigScreenConfig.cxx
/// \author Andrea Ferrero
///

#include "Common/BigScreenConfig.h"
#include <boost/property_tree/ptree.hpp>
#include <chrono>

static void splitDataSourceName(std::string s, std::string& det, std::string& name)
{
  std::string delimiter = ":";
  size_t pos = s.find(delimiter);
  if (pos != std::string::npos) {
    det = s.substr(0, pos);
    s.erase(0, pos + delimiter.length());
  }
  name = s;
}

namespace o2::quality_control_modules::common
{

BigScreenConfig::BigScreenConfig(std::string name, const boost::property_tree::ptree& config)
  : PostProcessingConfig(name, config)
{
  auto getParInt = [&](std::string name, int& par) {
    if (!getConfigParameter(name).empty()) {
      par = std::stoi(getConfigParameter(name));
    }
  };

  // parameters
  if (const auto& customConfigs = config.get_child_optional("qc.postprocessing." + name + ".customization"); customConfigs.has_value()) {
    for (const auto& customConfig : customConfigs.value()) {
      if (const auto& customNames = customConfig.second.get_child_optional("name"); customNames.has_value()) {
        mConfigParameters.insert(std::make_pair(customConfig.second.get<std::string>("name"), customConfig.second.get<std::string>("value")));
      }
    }
  }

  getParInt("NRows", mNRows);
  getParInt("NCols", mNCols);
  getParInt("BorderWidth", mBorderWidth);
  getParInt("NotOlderThan", mNotOlderThan);
  getParInt("IgnoreActivity", mIgnoreActivity);

  auto now = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();

  for (const auto& dataSourceConfig : config.get_child("qc.postprocessing." + name + ".dataSources")) {
    if (const auto& sourceNames = dataSourceConfig.second.get_child_optional("names"); sourceNames.has_value()) {
      for (const auto& sourceName : sourceNames.value()) {
        std::string sourceDet, sourcePath;
        splitDataSourceName(sourceName.second.data(), sourceDet, sourcePath);
        if (sourceDet.empty()) {
          continue;
        }
        dataSources.push_back({ sourceDet, sourcePath });
      }
    } else if (!dataSourceConfig.second.get<std::string>("name").empty()) {
      // "name" : [ "something" ] would return an empty string here
      auto sourceName = dataSourceConfig.second.get<std::string>("name");
      std::string sourceDet, sourcePath;
      splitDataSourceName(sourceName, sourceDet, sourcePath);
      if (sourceDet.empty()) {
        continue;
      }
      dataSources.push_back({ sourceDet, sourcePath });
    } else {
      throw std::runtime_error("No 'name' value or a 'names' vector in the path 'qc.postprocessing." + name + ".dataSources'");
    }
  }
}

const std::string BigScreenConfig::getConfigParameter(std::string name) const
{
  std::string result;
  auto entry = mConfigParameters.find(name);
  if (entry != mConfigParameters.end()) {
    result = entry->second;
  }

  return result;
}

} // namespace o2::quality_control_modules::common
