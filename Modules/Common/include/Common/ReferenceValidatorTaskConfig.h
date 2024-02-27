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
/// \file   ReferenceValidatorTaskConfig.h
/// \author Andrea Ferrero
///

#ifndef QUALITYCONTROL_REFERENCEVALIDATORTASKCONFIG_H
#define QUALITYCONTROL_REFERENCEVALIDATORTASKCONFIG_H

#include <vector>
#include <map>
#include <string>
#include "QualityControl/PostProcessingConfig.h"

namespace o2::quality_control_modules::common
{

// todo pretty print
/// \brief  ReferenceValidatorTask configuration structure
struct ReferenceValidatorTaskConfig : quality_control::postprocessing::PostProcessingConfig {
  ReferenceValidatorTaskConfig() = default;
  ReferenceValidatorTaskConfig(std::string name, const boost::property_tree::ptree& config);
  ~ReferenceValidatorTaskConfig() = default;

  struct DataGroup {
    std::string name;
    std::string path;
    std::vector<std::string> objects;
  };

  std::vector<DataGroup> dataGroups;
};

} // namespace o2::quality_control_modules::common

#endif // QUALITYCONTROL_REFERENCEVALIDATORTASKCONFIG_H
