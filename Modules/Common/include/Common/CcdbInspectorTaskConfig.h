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
/// \file    CcdbInspectorTaskConfig.h
/// \author  Andrea Ferrero andrea.ferrero@cern.ch
/// \brief   Header file for the configuration of MCH post-processing tasks
/// \since   16/06/2022
///

#ifndef QC_MODULE_COMMON_CCDB_INSPECTOR_PP_CONF_H
#define QC_MODULE_COMMON_CCDB_INSPECTOR_PP_CONF_H

#include "QualityControl/PostProcessingConfig.h"
#include <vector>
#include <string>

using namespace o2::quality_control::postprocessing;
namespace o2::quality_control_modules::common
{

/// \brief  MCH trending configuration structure
struct CcdbInspectorTaskConfig : PostProcessingConfig {
  CcdbInspectorTaskConfig() = default;
  CcdbInspectorTaskConfig(std::string name, const boost::property_tree::ptree& config);
  ~CcdbInspectorTaskConfig() = default;

  enum ObjectUpdatePolicy {
    updatePolicyPeriodic,
    updatePolicyAtSOR,
    updatePolicyAtEOR
  };

  struct DataSource {
    std::string name;
    std::string path;
    std::string validatorName;
    std::string moduleName;
    ObjectUpdatePolicy updatePolicy;
    int cycleDuration;
    uint64_t lastCreationTimestamp;
    int validObjectsCount;
    int binNumber;
  };

  std::vector<DataSource> dataSources;
};

} // namespace o2::quality_control_modules::common

#endif // QC_MODULE_COMMON_CCDB_INSPECTOR_PP_CONF_H
