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
/// \file   ServicesConfig.h
/// \author Piotr Konopka
///

#ifndef QUALITYCONTROL_SERVICESCONFIG_H
#define QUALITYCONTROL_SERVICESCONFIG_H

#include "QualityControl/Activity.h"
#include "QualityControl/LogDiscardParameters.h"
#include <unordered_map>
#include <string>

namespace o2::quality_control::core
{

struct ServicesConfig {
  std::unordered_map<std::string, std::string> database;
  Activity activity;
  std::string monitoringUrl = "infologger:///debug?qc";
  std::string conditionDBUrl = "http://ccdb-test.cern.ch:8080";
  LogDiscardParameters infologgerDiscardParameters;
  std::string bookkeepingUrl;
  std::string kafkaBrokersUrl;
  std::string kafkaTopicAliECSRun = "aliecs.run";
};

} // namespace o2::quality_control::core

#endif // QUALITYCONTROL_SERVICESCONFIG_H