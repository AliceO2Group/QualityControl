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
/// \file   AggregatorRunnerConfig.h
/// \author Piotr Konopka
///

#ifndef QUALITYCONTROL_AGGREGATORRUNNERCONFIG_H
#define QUALITYCONTROL_AGGREGATORRUNNERCONFIG_H

#include <unordered_map>
#include <string>
#include <Framework/DataProcessorSpec.h>
#include "QualityControl/Activity.h"

namespace o2::quality_control::checker
{

struct AggregatorRunnerConfig {
  std::unordered_map<std::string, std::string> database;
  std::string consulUrl{};
  std::string monitoringUrl{};
  bool infologgerFilterDiscardDebug = false;
  int infologgerDiscardLevel = 21;
  std::string infologgerDiscardFile{};
  core::Activity fallbackActivity;
  framework::Options options{};
};

} // namespace o2::quality_control::checker

#endif //QUALITYCONTROL_AGGREGATORRUNNERCONFIG_H
