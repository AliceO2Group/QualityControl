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
/// \file    PostProcessingConfigMCH.h
/// \author  Andrea Ferrero andrea.ferrero@cern.ch
/// \brief   Header file for the configuration of MCH post-processing tasks
/// \since   16/06/2022
///

#ifndef QC_MODULE_MCH_TRENDING_CONF_H
#define QC_MODULE_MCH_TRENDING_CONF_H

#include "QualityControl/PostProcessingConfig.h"
#include <vector>
#include <map>
#include <string>
#include <sstream>

using namespace o2::quality_control::postprocessing;
namespace o2::quality_control_modules::muonchambers
{

/// \brief  MCH trending configuration structure
struct PostProcessingConfigMCH : PostProcessingConfig {
  PostProcessingConfigMCH() = default;
  PostProcessingConfigMCH(std::string name, const boost::property_tree::ptree& config);
  ~PostProcessingConfigMCH() = default;

  const bool hasParameter(std::string name) const
  {
    auto entry = parameters.find(name);
    return (entry != parameters.end());
  }

  template <typename T>
  const T getParameter(std::string name) const;

  template <typename T>
  const T getParameter(std::string name, T defaultValue) const;

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

  std::map<std::string, std::string> parameters;
  std::vector<Plot> plots;
  std::vector<DataSource> dataSources;
};

template <typename T>
const T PostProcessingConfigMCH::getParameter(std::string name) const
{
  T result{};
  auto entry = parameters.find(name);
  if (entry != parameters.end()) {
    std::istringstream istr(entry->second);
    istr >> result;
  }

  return result;
}

template <typename T>
const T PostProcessingConfigMCH::getParameter(std::string name, T defaultValue) const
{
  T result = defaultValue;
  auto entry = parameters.find(name);
  if (entry != parameters.end()) {
    std::istringstream istr(entry->second);
    istr >> result;
  }

  return result;
}

} // namespace o2::quality_control_modules::muonchambers

#endif // QC_MODULE_MCH_TRENDING_CONF_H
