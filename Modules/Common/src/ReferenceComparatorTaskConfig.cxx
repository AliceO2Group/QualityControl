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
/// \file   ReferenceComparatorTaskConfig.cxx
/// \author Andrea Ferrero
///

#include "Common/ReferenceComparatorTaskConfig.h"
#include <boost/property_tree/ptree.hpp>
#include <chrono>

namespace o2::quality_control_modules::common
{

static std::vector<std::string> getTokens(std::string str, std::string sep)
{
  std::vector<std::string> result;
  size_t pos = 0;
  while (pos < str.size()) {
    size_t pos2 = str.find(sep, pos);
    size_t count = pos2 - pos;
    auto token = str.substr(pos, count);
    pos = (pos2 == std::string::npos) ? pos2 : pos2 + 1;
    result.push_back(token);
  }
  return result;
}

ReferenceComparatorTaskConfig::ReferenceComparatorTaskConfig(std::string name, const boost::property_tree::ptree& config)
  : PostProcessingConfig(name, config)
{
  for (const auto& dataGroupConfig : config.get_child("qc.postprocessing." + name + ".dataGroups")) {
    DataGroup dataGroup{
      dataGroupConfig.second.get<std::string>("name"),
      dataGroupConfig.second.get<std::string>("inputPath"),
      dataGroupConfig.second.get<std::string>("outputPath")
    };
    if (const auto& inputObjects = dataGroupConfig.second.get_child_optional("inputObjects"); inputObjects.has_value()) {
      for (const auto& inputObject : inputObjects.value()) {
        auto tokens = getTokens(inputObject.second.data(), ":");
        bool scale = true;
        if (tokens.size() == 2 && tokens[1] == "noscale") {
          scale = false;
        }
        dataGroup.objects.emplace_back(std::make_pair(tokens[0], scale));
      }
    }

    dataGroups.emplace_back(std::move(dataGroup));
  }
}

} // namespace o2::quality_control_modules::common
