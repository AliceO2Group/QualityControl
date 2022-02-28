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
/// \file    TrendingConfigTOF.h
/// \author  Nicolò Jacazio nicolo.jacazio@cern.ch
/// \author  Francesca Ercolessi francesca.ercolessi@cern.ch
/// \brief   Header file for the trending task configuration for the number of hits in TOF
/// \since   05/10/2021
///

#ifndef QC_MODULE_TOF_TRENDING_CONF_H
#define QC_MODULE_TOF_TRENDING_CONF_H

#include <vector>
#include <string>
#include "QualityControl/PostProcessingConfig.h"

using namespace o2::quality_control::postprocessing;
namespace o2::quality_control_modules::tof
{

/// \brief  TOF trending configuration structure
struct TrendingConfigTOF : PostProcessingConfig {
  TrendingConfigTOF() = default;
  TrendingConfigTOF(std::string name, const boost::property_tree::ptree& config);
  ~TrendingConfigTOF() = default;

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

  std::vector<Plot> plots;
  std::vector<DataSource> dataSources;
};

} // namespace o2::quality_control_modules::tof

#endif // QC_MODULE_TOF_TRENDING_CONF_H
