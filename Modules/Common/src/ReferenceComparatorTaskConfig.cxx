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

ReferenceComparatorTaskConfig::ReferenceComparatorTaskConfig(std::string name, const boost::property_tree::ptree& config)
  : PostProcessingConfig(name, config)
{
  for (const auto& dataGroupConfig : config.get_child("qc.postprocessing." + name + ".dataGroups")) {
    auto inputPath = dataGroupConfig.second.get<std::string>("inputPath");
    auto referencePath = dataGroupConfig.second.get<std::string>("referencePath", inputPath);
    DataGroup dataGroup{
      dataGroupConfig.second.get<std::string>("name"),
      inputPath,
      referencePath,
      dataGroupConfig.second.get<std::string>("outputPath"),
      dataGroupConfig.second.get<bool>("normalizeReference", false),
      dataGroupConfig.second.get<bool>("drawRatioOnly", false),
      dataGroupConfig.second.get<double>("legendHeight", 0.2),
      dataGroupConfig.second.get<std::string>("drawOption1D", "HIST"),
      dataGroupConfig.second.get<std::string>("drawOption2D", "COLZ")
    };
    if (const auto& inputObjects = dataGroupConfig.second.get_child_optional("inputObjects"); inputObjects.has_value()) {
      for (const auto& inputObject : inputObjects.value()) {
        dataGroup.objects.emplace_back(inputObject.second.data());
      }
    }

    dataGroups.emplace_back(std::move(dataGroup));
  }
}

} // namespace o2::quality_control_modules::common
