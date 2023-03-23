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
/// \file   PostProcessingConfig.h
/// \author Piotr Konopka
///

#ifndef QUALITYCONTROL_POSTPROCESSINGCONFIG_H
#define QUALITYCONTROL_POSTPROCESSINGCONFIG_H

#include <vector>
#include <string>
#include <boost/property_tree/ptree_fwd.hpp>
#include "QualityControl/Activity.h"

namespace o2::quality_control::postprocessing
{

//todo pretty print

/// \brief  Post-processing configuration structure
struct PostProcessingConfig {
  PostProcessingConfig() = default;
  PostProcessingConfig(const std::string& id, const boost::property_tree::ptree& config);
  ~PostProcessingConfig() = default;
  std::string id;
  std::string taskName;
  std::string moduleName;
  std::string className;
  std::string detectorName = "MISC";
  std::vector<std::string> initTriggers = {};
  std::vector<std::string> updateTriggers = {};
  std::vector<std::string> stopTriggers = {};
  std::string qcdbUrl;
  std::string ccdbUrl;
  std::string consulUrl;
  core::Activity activity;
  bool matchAnyRunNumber = false;
};

} // namespace o2::quality_control::postprocessing

#endif //QUALITYCONTROL_POSTPROCESSINGCONFIG_H
