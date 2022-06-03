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
/// \file   CheckRunnerConfig.h
/// \author Piotr Konopka
///
#ifndef QUALITYCONTROL_CHECKRUNNERCONFIG_H
#define QUALITYCONTROL_CHECKRUNNERCONFIG_H

#include <string>
#include <unordered_map>

namespace o2::quality_control::checker
{

struct CheckRunnerConfig {
  std::unordered_map<std::string, std::string> database;
  std::string consulUrl{};
  std::string monitoringUrl{};
  bool infologgerFilterDiscardDebug = false;
  int infologgerDiscardLevel = 21;
  int fallbackRunNumber = 0;
  int fallbackRunType = 0;
  std::string fallbackPeriodName{};
  std::string fallbackPassName{};
  std::string fallbackProvenance{};
  framework::Options options{};
};

} // namespace o2::quality_control::checker

#endif //QUALITYCONTROL_CHECKRUNNERCONFIG_H
