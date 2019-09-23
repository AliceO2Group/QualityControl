// Copyright CERN and copyright holders of ALICE O2. This software is
// distributed under the terms of the GNU General Public License v3 (GPL
// Version 3), copied verbatim in the file "COPYING".
//
// See http://alice-o2.web.cern.ch/license for full licensing information.
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.


#ifndef QUALITYCONTROL_TRENDINGTASKCONFIG_H
#define QUALITYCONTROL_TRENDINGTASKCONFIG_H

#include <vector>
#include <string>
#include <unordered_map>
#include "QualityControl/PostProcessingConfig.h"

namespace o2::quality_control::postprocessing {


//todo pretty print
struct TrendingTaskConfig : PostProcessingConfig {
  TrendingTaskConfig() = default;
  TrendingTaskConfig(std::string name, configuration::ConfigurationInterface& config) {} //todo implement
  ~TrendingTaskConfig() = default;

};

} // namespace o2::quality_control::postprocessing

#endif //QUALITYCONTROL_TRENDINGTASKCONFIG_H
