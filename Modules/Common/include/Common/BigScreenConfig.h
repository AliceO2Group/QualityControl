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
/// \file   BigScreenConfig.h
/// \author Andrea Ferrero
///

#ifndef QUALITYCONTROL_BIGSCREENCONFIG_H
#define QUALITYCONTROL_BIGSCREENCONFIG_H

#include "QualityControl/PostProcessingConfig.h"
#include <vector>
#include <string>

namespace o2::quality_control_modules::common
{

// todo pretty print
/// \brief  BigScreen configuration structure
struct BigScreenConfig : quality_control::postprocessing::PostProcessingConfig {
  BigScreenConfig() = default;
  BigScreenConfig(std::string name, const boost::property_tree::ptree& config);
  ~BigScreenConfig() = default;

  struct DataSource {
    std::string detector;
    std::string path;
  };

  std::vector<DataSource> dataSources;
};

} // namespace o2::quality_control_modules::common

#endif // QUALITYCONTROL_BIGSCREENCONFIG_H
