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
/// \file   QualityTaskConfig.cxx
/// \author Andrea Ferrero
/// \author Piotr Konopka
///

#include "Common/QualityTaskConfig.h"
#include <boost/property_tree/ptree.hpp>
#include <chrono>

using namespace o2::quality_control::core;

namespace o2::quality_control_modules::common
{

QualityTaskConfig::QualityTaskConfig(std::string name, const boost::property_tree::ptree& config)
  : PostProcessingConfig(name, config)
{
  for (const auto& qualityGroupConfig : config.get_child("qc.postprocessing." + name + ".qualityGroups")) {
    QualityGroup qualityGroup{
      qualityGroupConfig.second.get<std::string>("name"),
      qualityGroupConfig.second.get<std::string>("title", ""),
      qualityGroupConfig.second.get<std::string>("path"),
    };
    if (const auto& ignoredQualities = qualityGroupConfig.second.get_child_optional("ignoreQualitiesDetails"); ignoredQualities.has_value()) {
      for (const auto& ignoredQuality : ignoredQualities.value()) {
        qualityGroup.ignoreQualitiesDetails.emplace_back(Quality::fromString(ignoredQuality.second.get_value<std::string>()));
      }
    }

    if (const auto& inputObjects = qualityGroupConfig.second.get_child_optional("inputObjects"); inputObjects.has_value()) {
      for (const auto& inputObject : inputObjects.value()) {
        QualityConfig qualityConfig{
          inputObject.second.get<std::string>("name"),
          inputObject.second.get<std::string>("title", "")
        };
        qualityConfig.messages[Quality::Good.getName()] = inputObject.second.get<std::string>("messageGood", "");
        qualityConfig.messages[Quality::Medium.getName()] = inputObject.second.get<std::string>("messageMedium", "");
        qualityConfig.messages[Quality::Bad.getName()] = inputObject.second.get<std::string>("messageBad", "");
        qualityConfig.messages[Quality::Null.getName()] = inputObject.second.get<std::string>("messageNull", "");
        qualityGroup.inputObjects.emplace_back(std::move(qualityConfig));
      }
    }

    qualityGroups.emplace_back(std::move(qualityGroup));
  }
}

} // namespace o2::quality_control_modules::common