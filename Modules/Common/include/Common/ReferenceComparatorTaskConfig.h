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
/// \file   ReferenceComparatorTaskConfig.h
/// \author Andrea Ferrero
///

#ifndef QUALITYCONTROL_REFERENCECOMPARATORTASKCONFIG_H
#define QUALITYCONTROL_REFERENCECOMPARATORTASKCONFIG_H

#include <vector>
#include <map>
#include <string>
#include "QualityControl/PostProcessingConfig.h"

namespace o2::quality_control_modules::common
{

// todo pretty print
/// \brief  ReferenceComparatorTask configuration structure
struct ReferenceComparatorTaskConfig : quality_control::postprocessing::PostProcessingConfig {
  ReferenceComparatorTaskConfig() = default;
  ReferenceComparatorTaskConfig(std::string name, const boost::property_tree::ptree& config);
  ~ReferenceComparatorTaskConfig() = default;

  struct DataGroup {
    std::string name;
    std::string inputPath;
    std::string outputPath;
    std::vector<std::pair<std::string, bool>> objects;
  };

  std::vector<DataGroup> dataGroups;
};

} // namespace o2::quality_control_modules::common

#endif // QUALITYCONTROL_REFERENCECOMPARATORTASKCONFIG_H
