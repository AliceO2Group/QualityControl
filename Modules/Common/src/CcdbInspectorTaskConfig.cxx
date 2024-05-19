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
/// \file    CcdbInspectorTaskConfig.cxx
/// \author  Andrea Ferrero andrea.ferrero@cern.ch
/// \brief   File for the configuration of CCDB inspector post-processing task
///

#include "Common/CcdbInspectorTaskConfig.h"
#include <boost/property_tree/ptree.hpp>
#include <iostream>

using namespace o2::quality_control::postprocessing;
namespace o2::quality_control_modules::common
{

CcdbInspectorTaskConfig::CcdbInspectorTaskConfig(std::string name, const boost::property_tree::ptree& config)
  : PostProcessingConfig(name, config)
{
  // Data source configuration
  int bin = 1;
  for (const auto& dataSourceConfig : config.get_child("qc.postprocessing." + name + ".dataSources")) {
    auto name = dataSourceConfig.second.get<std::string>("name");
    auto path = dataSourceConfig.second.get<std::string>("path");
    auto validatorName = dataSourceConfig.second.get("validatorName", std::string(""));
    auto moduleName = dataSourceConfig.second.get("moduleName", std::string(""));

    auto updatePolicyString = dataSourceConfig.second.get("updatePolicy", std::string("periodic"));
    ObjectUpdatePolicy updatePolicy;
    if (updatePolicyString == "periodic") {
      updatePolicy = ObjectUpdatePolicy::updatePolicyPeriodic;
    }
    if (updatePolicyString == "atSOR") {
      updatePolicy = ObjectUpdatePolicy::updatePolicyAtSOR;
    }
    if (updatePolicyString == "atEOR") {
      updatePolicy = ObjectUpdatePolicy::updatePolicyAtEOR;
    }

    auto cycleDuration = dataSourceConfig.second.get<int>("cycleDuration", 0);
    uint64_t timestamp = 0;
    int valibObjectCount = 0;
    dataSources.push_back({ name, path, validatorName, moduleName, updatePolicy, cycleDuration, timestamp, valibObjectCount, bin });
    bin += 1;
  }
}

} // namespace o2::quality_control_modules::common
