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
/// \file   TrendingTaskConfigMID.h
/// \author          based on  Piotr Konopkan work
///

#ifndef QUALITYCONTROL_TRENDINGTASKCONFIGMID_H
#define QUALITYCONTROL_TRENDINGTASKCONFIGMID_H

#include <vector>
#include <string>
#include "QualityControl/PostProcessingConfig.h"

namespace o2::quality_control::postprocessing
{

struct TrendingTaskConfigMID : PostProcessingConfig {
  TrendingTaskConfigMID() = default;
  TrendingTaskConfigMID(std::string name, const boost::property_tree::ptree& config);
  ~TrendingTaskConfigMID() = default;

  struct Plot {
    std::string name;
    std::string title;
    std::string varexp;
    std::string selection;
    std::string option;
    std::string graphErrors;
  };

  struct DataSource {
    std::string type;
    std::string path;
    std::string name;
    std::string reductorName;
    std::string moduleName;
  };

  bool producePlotsOnUpdate{};
  bool resumeTrend{};
  std::vector<Plot> plots;
  std::vector<DataSource> dataSources;
};

} // namespace o2::quality_control::postprocessing

#endif // QUALITYCONTROL_TRENDINGTASKCONFIGMID_H
