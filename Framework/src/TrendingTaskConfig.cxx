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
/// \file   TrendingTaskConfig.cxx
/// \author Piotr Konopka
///

#include "QualityControl/TrendingTaskConfig.h"
#include <boost/property_tree/ptree.hpp>

namespace o2::quality_control::postprocessing
{

TrendingTaskConfig::TrendingTaskConfig(std::string id, const boost::property_tree::ptree& config)
  : PostProcessingConfig(id, config)
{
  producePlotsOnUpdate = config.get<bool>("qc.postprocessing." + id + ".producePlotsOnUpdate", true);
  resumeTrend = config.get<bool>("qc.postprocessing." + id + ".resumeTrend", false);
  trendIfAllInputs = config.get<bool>("qc.postprocessing." + id + ".trendIfAllInputs", false);
  trendingTimestamp = config.get<std::string>("qc.postprocessing." + id + ".trendingTimestamp", "validUntil");

  machine = TrendingMachineConfig(config.get_child("qc.postprocessing." + id));
}

} // namespace o2::quality_control::postprocessing
