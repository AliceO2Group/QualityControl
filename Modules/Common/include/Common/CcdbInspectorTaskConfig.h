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
/// \brief   Header file for the configuration of CCDB inspector post-processing task
///

#ifndef QC_MODULE_COMMON_CCDB_INSPECTOR_PP_CONF_H
#define QC_MODULE_COMMON_CCDB_INSPECTOR_PP_CONF_H

#include "QualityControl/PostProcessingConfig.h"
#include <vector>
#include <string>

using namespace o2::quality_control::postprocessing;
namespace o2::quality_control_modules::common
{

/// \brief configuration structure for the CcdbInspectorTask
struct CcdbInspectorTaskConfig : PostProcessingConfig {
  CcdbInspectorTaskConfig() = default;
  CcdbInspectorTaskConfig(std::string name, const boost::property_tree::ptree& config);
  ~CcdbInspectorTaskConfig() = default;

  /// update policy associated to each data source
  enum class ObjectUpdatePolicy {
    updatePolicyPeriodic, ///< the object is updated periodically at fixed time intervals
    updatePolicyAtSOR,    ///< the object is updated only once at start-of-run
    updatePolicyAtEOR     ///< the object is updated only once at end-of-run
  };

  /// CCDB object description and associated variables
  struct DataSource {
    std::string name;                ///< mnemonic name of the object
    std::string path;                ///< object path in the database
    std::string validatorName;       ///< name of the optional validator class (can be empty)
    std::string moduleName;          ///< module containing the validator class (mandatory if validatorName is not empty)
    ObjectUpdatePolicy updatePolicy; ///< object's update policy
    int cycleDuration;               ///< time interval between updates for periodic objects
    uint64_t lastCreationTimestamp;  ///< creation time-stamp of the last valid object that was found
    int validObjectsCount;           ///< number of valid objects that have been found
    int binNumber;                   ///< bin number associated to this object in the output 2-D plot
  };

  std::vector<DataSource> dataSources;
};

} // namespace o2::quality_control_modules::common

#endif // QC_MODULE_COMMON_CCDB_INSPECTOR_PP_CONF_H
