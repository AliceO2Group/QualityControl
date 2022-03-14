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
/// \file   TRFCollectionTaskConfig.h
/// \author Piotr Konopka
///

#ifndef QUALITYCONTROL_TIMERANGESTASKCONFIG
#define QUALITYCONTROL_TIMERANGESTASKCONFIG

#include <vector>
#include <string>
#include "QualityControl/PostProcessingConfig.h"

namespace o2::quality_control_modules::common
{

//todo pretty print
/// \brief  TRFCollectionTaskConfig configuration structure
struct TRFCollectionTaskConfig : quality_control::postprocessing::PostProcessingConfig {
  TRFCollectionTaskConfig() = default;
  TRFCollectionTaskConfig(std::string name, const boost::property_tree::ptree& config);
  ~TRFCollectionTaskConfig() = default;

  std::string name;
  std::string detector;
  int runNumber;
  std::string passName;
  std::string periodName;
  std::string provenance;
  std::vector<std::string> qualityObjects;
};

} // namespace o2::quality_control_modules::common

#endif // QUALITYCONTROL_TIMERANGESTASKCONFIG
