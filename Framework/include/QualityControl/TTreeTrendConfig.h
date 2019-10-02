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

#ifndef QUALITYCONTROL_TTREETRENDCONFIG_H
#define QUALITYCONTROL_TTREETRENDCONFIG_H

#include <vector>
#include <string>
#include "QualityControl/PostProcessingConfig.h"

namespace o2::quality_control::postprocessing
{

//todo pretty print
/// \brief  TTreeTrend configuration structure
struct TTreeTrendConfig : PostProcessingConfig {
  TTreeTrendConfig() = default;
  TTreeTrendConfig(std::string name, configuration::ConfigurationInterface& config);
  ~TTreeTrendConfig() = default;

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

#endif //QUALITYCONTROL_TTREETRENDCONFIG_H
