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
#include <map>
#include <string>
#include "QualityControl/PostProcessingConfig.h"

namespace o2::quality_control_modules::common
{

// todo pretty print
/// \brief  QualityTask configuration structure
struct QualityTaskConfig : quality_control::postprocessing::PostProcessingConfig {
  QualityTaskConfig() = default;
  QualityTaskConfig(std::string name, const boost::property_tree::ptree& config);
  ~QualityTaskConfig() = default;

  const std::string getConfigParameter(std::string name) const;

  struct DataSource {
    std::string type;
    std::string path;
    std::string name;
    long timeStamp{ 0 };
  };

  std::map<std::string, std::string> mConfigParameters;
  std::string mAggregatedQualityName;
  std::string mMessageGood;
  std::string mMessageMedium;
  std::string mMessageBad;
  std::string mMessageNull;

  std::vector<DataSource> dataSources;
};

} // namespace o2::quality_control_modules::common

#endif // QUALITYCONTROL_QUALITYTASKCONFIG_H
