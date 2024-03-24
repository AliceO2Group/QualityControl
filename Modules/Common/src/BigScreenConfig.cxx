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
#include <CommonUtils/StringUtils.h>
#include <boost/property_tree/ptree.hpp>

namespace o2::quality_control_modules::common
{

BigScreenConfig::BigScreenConfig(std::string name, const boost::property_tree::ptree& config)
  : PostProcessingConfig(name, config)
{
  for (const auto& dataSourceConfig : config.get_child("qc.postprocessing." + name + ".dataSources")) {
    if (const auto& sourceNames = dataSourceConfig.second.get_child_optional("names"); sourceNames.has_value()) {
      for (const auto& sourceName : sourceNames.value()) {
        auto tokens = o2::utils::Str::tokenize(sourceName.second.data(), ':', false, true);
        if (tokens.size() != 2 || tokens[0].empty()) {
          continue;
        }
        dataSources.push_back({ tokens[0], tokens[1] });
      }
    } else if (!dataSourceConfig.second.get<std::string>("name").empty()) {
      // "name" : [ "something" ] would return an empty string here
      auto sourceName = dataSourceConfig.second.get<std::string>("name");
      auto tokens = o2::utils::Str::tokenize(sourceName, ':', false, true);
      if (tokens.size() != 2 || tokens[0].empty()) {
        continue;
      }
      dataSources.push_back({ tokens[0], tokens[1] });
    } else {
      throw std::runtime_error("No 'name' value or a 'names' vector in the path 'qc.postprocessing." + name + ".dataSources'");
    }
  }
}

} // namespace o2::quality_control_modules::common
