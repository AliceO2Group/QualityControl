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
/// \file   runnerUtils.cxx
/// \author Barthelemy von Haller
/// \author  Piotr Konopka
///

#include "QualityControl/runnerUtils.h"

#include <boost/property_tree/json_parser.hpp>
#include <CommonUtils/StringUtils.h>
#include <regex>
#include <Configuration/ConfigurationFactory.h>
#include <Common/Exceptions.h>
#include <Framework/DeviceSpec.h>
#include <Framework/ConfigParamRegistry.h>

#include <chrono>
#include <cstdint>

namespace o2::quality_control::core
{

std::string getFirstTaskName(const std::string& configurationSource)
{
  auto config = o2::configuration::ConfigurationFactory::getConfiguration(configurationSource);

  for (const auto& task : config->getRecursive("qc.tasks")) {
    return task.first; // task name;
  }

  throw;
}

std::string getFirstCheckName(const std::string& configurationSource)
{
  auto config = o2::configuration::ConfigurationFactory::getConfiguration(configurationSource);

  if (config->getRecursive("qc").count("checks")) {
    for (const auto& check : config->getRecursive("qc.checks")) {
      return check.first; // check name;
    }
  }

  BOOST_THROW_EXCEPTION(AliceO2::Common::ObjectNotFoundError() << AliceO2::Common::errinfo_details("No checks defined"));
}

bool hasChecks(const std::string& configSource)
{
  auto config = o2::configuration::ConfigurationFactory::getConfiguration(configSource);
  return config->getRecursive("qc").count("checks") > 0;
}

Activity computeActivity(framework::ServiceRegistryRef services, const Activity& fallbackActivity)
{
  auto runNumber = computeActivityField<int>(services, "runNumber", fallbackActivity.mId);
  auto runType = computeActivityField<int>(services, "runType", fallbackActivity.mType);
  auto run_start_time_ms = computeActivityField<unsigned long>(services, "runStartTimeMs", fallbackActivity.mValidity.getMin());
  auto run_stop_time_ms = computeActivityField<unsigned long>(services, "runEndTimeMs", fallbackActivity.mValidity.getMax());
  auto partitionName = services.get<framework::RawDeviceService>().device()->fConfig->GetProperty<std::string>("environment_id", fallbackActivity.mPartitionName);
  auto periodName = services.get<framework::RawDeviceService>().device()->fConfig->GetProperty<std::string>("lhcPeriod", fallbackActivity.mPeriodName);
  auto fillNumber = computeActivityField<int>(services, "fillInfoFillNumber", fallbackActivity.mFillNumber);
  auto beam_type = services.get<framework::RawDeviceService>().device()->fConfig->GetProperty<std::string>("fillInfoBeamType", fallbackActivity.mBeamType);

  Activity activity(
    runNumber,
    runType,
    periodName,
    fallbackActivity.mPassName,
    fallbackActivity.mProvenance,
    { run_start_time_ms, run_stop_time_ms },
    beam_type,
    partitionName,
    fillNumber);

  return activity;
}

std::string indentTree(int level)
{
  return std::string(level * 2, ' ');
}

void printTree(const boost::property_tree::ptree& pt, int level)
{
  std::stringstream ss;
  boost::property_tree::json_parser::write_json(ss, pt);
  for (std::string line; std::getline(ss, line, '\n');)
    ILOG(Debug, Trace) << line << ENDM;
}

std::vector<std::pair<std::string, std::string>> parseOverrideValues(const std::string& input)
{
  std::vector<std::pair<std::string, std::string>> keyValuePairs;
  for (const auto& keyValueToken : utils::Str::tokenize(input, ';', true)) {
    auto keyValue = utils::Str::tokenize(keyValueToken, '=', true);
    if (keyValue.size() == 1) {
      keyValuePairs.emplace_back(keyValue[0], "");
    } else if (keyValue.size() == 2) {
      keyValuePairs.emplace_back(keyValue[0], keyValue[1]);
    } else {
      throw std::runtime_error("Token '" + keyValueToken + "' in the --override-values argument is malformed, use key=value.");
    }
  }
  return keyValuePairs;
}

void overrideValues(boost::property_tree::ptree& tree, const std::vector<std::pair<std::string, std::string>>& keyValues)
{
  for (const auto& [key, value] : keyValues) {
    tree.put(key, value);
  }
}

/**
 * template the param infologgerDiscardFile (_ID_->[device-id])
 * @param originalFile
 * @param iCtx
 * @return
 */
std::string templateILDiscardFile(std::string& originalFile, framework::InitContext& iCtx)
{
  try {
    auto& deviceSpec = iCtx.services().get<o2::framework::DeviceSpec const>();
    return std::regex_replace(originalFile, std::regex("_ID_"), deviceSpec.id);
  } catch (...) {
    ILOG(Error, Devel) << "exception caught and swallowed in templateILDiscardFile : " << boost::current_exception_diagnostic_information() << ENDM;
  }
  return originalFile;
}

uint64_t getCurrentTimestamp()
{
  auto now = std::chrono::system_clock::now();
  auto now_ms = std::chrono::time_point_cast<std::chrono::milliseconds>(now);
  auto epoch = now_ms.time_since_epoch();
  auto value = std::chrono::duration_cast<std::chrono::milliseconds>(epoch);
  return value.count();
}

} // namespace o2::quality_control::core