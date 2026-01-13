//
// Created by pkonopka on 04/12/2025.
//

#ifndef QUALITYCONTROL_SERVICESCONFIG_H
#define QUALITYCONTROL_SERVICESCONFIG_H

#include "QualityControl/Activity.h"
#include "QualityControl/LogDiscardParameters.h"
#include <unordered_map>
#include <string>

namespace o2::quality_control::core {

struct ServicesConfig
{
  std::unordered_map<std::string, std::string> database;
  Activity activity;
  std::string monitoringUrl = "infologger:///debug?qc";
  std::string conditionDBUrl = "http://ccdb-test.cern.ch:8080";
  LogDiscardParameters infologgerDiscardParameters;
  std::string bookkeepingUrl;
  std::string kafkaBrokersUrl;
  std::string kafkaTopicAliECSRun = "aliecs.run";
};

}

#endif //QUALITYCONTROL_SERVICESCONFIG_H