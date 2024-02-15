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
/// \file    PostProcessingConfigZDC.h
/// \author  Andrea Ferrero andrea.ferrero@cern.ch
/// \brief   Header file for the configuration of ZDC post-processing tasks
/// \since   30/08/2023
///

#ifndef QC_MODULE_ZDC_PPCONFIG_H
#define QC_MODULE_ZDC_PPCONFIG_H

#include "QualityControl/PostProcessingConfig.h"
#include <vector>
#include <map>
#include <string>
#include <sstream>

using namespace o2::quality_control::postprocessing;
namespace o2::quality_control_modules::zdc
{

/// \brief  MCH trending configuration structure
struct PostProcessingConfigZDC : PostProcessingConfig {
  PostProcessingConfigZDC() = default;
  PostProcessingConfigZDC(std::string name, const boost::property_tree::ptree& config);
  ~PostProcessingConfigZDC() = default;

  const bool hasParameter(std::string name) const
  {
    auto entry = parameters.find(name);
    return (entry != parameters.end());
  }

  struct DataSource {
    std::string path;
    std::string name;
  };

  std::map<std::string, std::string> parameters;
  std::vector<DataSource> dataSourcesADC;
  std::vector<DataSource> dataSourcesTDC;
};

} // namespace o2::quality_control_modules::zdc

#endif // QC_MODULE_ZDC_PPCONFIG_H
