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
/// \file   ReferenceValidatorTaskConfig.cxx
/// \author Andrea Ferrero
///

#include "Common/ReferenceValidatorTaskConfig.h"
#include <boost/property_tree/ptree.hpp>
#include <chrono>

namespace o2::quality_control_modules::common
{

ReferenceValidatorTaskConfig::ReferenceValidatorTaskConfig(std::string name, const boost::property_tree::ptree& config)
  : PostProcessingConfig(name, config)
{
  for (const auto& dataGroupConfig : config.get_child("qc.postprocessing." + name + ".dataGroups")) {
    DataGroup dataGroup{
      dataGroupConfig.second.get<std::string>("name"),
      dataGroupConfig.second.get<std::string>("path")
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
