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
/// \file   UserCodeConfig.h
/// \author Barthelemy von Haller
///

#ifndef QUALITYCONTROL_USERCODECONFIG_H
#define QUALITYCONTROL_USERCODECONFIG_H

#include "QualityControl/CustomParameters.h"
#include "QualityControl/stringUtils.h"

namespace o2::quality_control::core
{

/// \brief  Container for the configuration of a Task
struct UserCodeConfig {
  std::string moduleName;
  std::string className;
  std::string detectorName = "MISC"; // intended to be the 3 letters code;
  std::string consulUrl;
  CustomParameters customParameters;
  std::string conditionUrl{};
  std::unordered_map<std::string, std::string> database;
};

} // namespace o2::quality_control::core

#endif // QUALITYCONTROL_USERCODECONFIG_H
