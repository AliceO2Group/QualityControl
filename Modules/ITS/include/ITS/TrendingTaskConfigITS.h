// Copyright CERN and copyright holders of ALICE O2. This software is
// distributed under the terms of the GNU General Public License v3 (GPL
// Version 3), copied verbatim in the file "COPYING".
//
// See http://alice-o2.web.cern.ch/license for full licensing information.
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.

///
/// \file   TrendingTaskConfig.h
/// \author Piotr Konopka
///

#ifndef QUALITYCONTROL_TRENDINGTASKCONFIGITS_H
#define QUALITYCONTROL_TRENDINGTASKCONFIGITS_H

#include <vector>
#include <string>
#include "QualityControl/PostProcessingConfig.h"

namespace o2::quality_control::postprocessing
{

//todo pretty print
/// \brief  TrendingTask configuration structure
struct TrendingTaskConfigITS : PostProcessingConfig {
  TrendingTaskConfigITS() = default;
  TrendingTaskConfigITS(std::string name, configuration::ConfigurationInterface& config);
  ~TrendingTaskConfigITS() = default;

  struct Plot {
    std::string name;
    std::string title;
    std::string varexp;
    std::string selection;
    std::string option;
  };

  struct DataSource {
    std::string type;
    std::string path;
    std::string name;
    std::string reductorName;
    std::string moduleName;
  };

  std::vector<Plot> plots;
  std::vector<DataSource> dataSources;
};

} // namespace o2::quality_control::postprocessing

#endif //QUALITYCONTROL_TRENDINGTASKCONFIG_H
