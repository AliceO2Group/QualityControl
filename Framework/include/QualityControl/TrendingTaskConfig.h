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
/// \file   TrendingTaskConfig.h
/// \author Piotr Konopka
///

#ifndef QUALITYCONTROL_TRENDINGTASKCONFIG_H
#define QUALITYCONTROL_TRENDINGTASKCONFIG_H

#include <string>
#include "QualityControl/PostProcessingConfig.h"
#include "QualityControl/TrendingMachineConfig.h"

namespace o2::quality_control::postprocessing
{

// todo pretty print
/// \brief  TrendingTask configuration structure
struct TrendingTaskConfig : PostProcessingConfig {
  TrendingTaskConfig() = default;
  TrendingTaskConfig(std::string name, const boost::property_tree::ptree& config);
  ~TrendingTaskConfig() = default;

  bool producePlotsOnUpdate{};
  bool resumeTrend{};
  bool trendIfAllInputs{ false };
  std::string trendingTimestamp;
  TrendingMachineConfig machine;
};

} // namespace o2::quality_control::postprocessing

#endif // QUALITYCONTROL_TRENDINGTASKCONFIG_H
