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
/// \file   QualityTaskConfig.h
/// \author Andrea Ferrero
///

#ifndef QUALITYCONTROL_QUALITYTASKCONFIG_H
#define QUALITYCONTROL_QUALITYTASKCONFIG_H

#include <vector>
#include <unordered_map>
#include <string>
#include "QualityControl/PostProcessingConfig.h"
#include "QualityControl/Quality.h"

namespace o2::quality_control_modules::common
{

// todo pretty print
/// \brief  QualityTask configuration structure
struct QualityTaskConfig : quality_control::postprocessing::PostProcessingConfig {
  QualityTaskConfig() = default;
  QualityTaskConfig(std::string name, const boost::property_tree::ptree& config);
  ~QualityTaskConfig() = default;

  struct QualityConfig {
    std::string name;
    std::string title;
    std::unordered_map<std::string /* quality */, std::string /* message */> messages = {
      { quality_control::core::Quality::Null.getName(), "" },
      { quality_control::core::Quality::Bad.getName(), "" },
      { quality_control::core::Quality::Medium.getName(), "" },
      { quality_control::core::Quality::Good.getName(), "" }
    };
  };

  struct QualityGroup {
    std::string name;
    std::string title;
    std::string path;
    std::vector<quality_control::core::Quality> ignoreQualitiesDetails{};
    std::vector<QualityConfig> inputObjects{};
  };

  std::vector<QualityGroup> qualityGroups;
};

} // namespace o2::quality_control_modules::common

#endif // QUALITYCONTROL_QUALITYTASKCONFIG_H