// Copyright 2019-2022 CERN and copyright holders of ALICE O2.
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
/// \file   runnerUtils.h
/// \author Barthelemy von Haller
///

#ifndef QUALITYCONTROL_RUNNERUTILS_H
#define QUALITYCONTROL_RUNNERUTILS_H

#include <string>
#include <Configuration/ConfigurationFactory.h>
#include <Common/Exceptions.h>
#include <Framework/RawDeviceService.h>
#include <Framework/DeviceSpec.h>
#include <fairmq/Device.h>
#include <Framework/ConfigParamRegistry.h>
#include <QualityControl/QcInfoLogger.h>
#include <boost/property_tree/json_parser.hpp>
#include <CommonUtils/StringUtils.h>
#include "QualityControl/Activity.h"
#include <regex>
#include "QualityControl/Bookkeeping.h"

// TODO: most of these could be moved to a source file with minimal performance impact
//  and noticable compilation time improvement (we bring boost, regex, FairMQ headers to 10 other source files)

namespace o2::quality_control::core
{

/**
 * Returns the name of the first task encountered in the config file.
 * Ad-hoc solution to avoid hard-coding the task when we create the printer (he needs it to know the data description
 * of the data coming out of the checker).
 * @param config
 * @return The name of the first task in the config file.
 */
inline std::string getFirstTaskName(std::string configurationSource)
{
  auto config = o2::configuration::ConfigurationFactory::getConfiguration(configurationSource);

  for (const auto& task : config->getRecursive("qc.tasks")) {
    return task.first; // task name;
  }

  throw;
}

inline std::string getFirstCheckName(std::string configurationSource)
{
  auto config = o2::configuration::ConfigurationFactory::getConfiguration(configurationSource);

  if (config->getRecursive("qc").count("checks")) {
    for (const auto& check : config->getRecursive("qc.checks")) {
      return check.first; // check name;
    }
  }

  BOOST_THROW_EXCEPTION(AliceO2::Common::ObjectNotFoundError() << AliceO2::Common::errinfo_details("No checks defined"));
}

inline bool hasChecks(std::string configSource)
{
  auto config = o2::configuration::ConfigurationFactory::getConfiguration(configSource);
  return config->getRecursive("qc").count("checks") > 0;
}

template <typename T>
inline T computeActivityField(framework::ServiceRegistryRef services, const std::string& name, T fallbackNumber = 0)
{
  int result = 0;

  try {
    auto temp = services.get<framework::RawDeviceService>().device()->fConfig->GetProperty<std::string>(name, "unspecified");
    ILOG(Debug, Devel) << "Got this property " << name << " from RawDeviceService: '" << temp << "'" << ENDM;
    result = boost::lexical_cast<T>(temp);
  } catch (std::invalid_argument& ia) {
    ILOG(Debug, Devel) << "   " << name << " not found in options or is not a number, using the fallback." << ENDM;
  }
  result = result > 0 /* found it in service */ ? result : fallbackNumber;
  ILOG(Info, Devel) << name << " returned by computeRunNumber (default) : " << result << ENDM;
  return result;
}

inline Activity computeActivity(framework::ServiceRegistryRef services, const Activity& fallbackActivity)
{
  auto runNumber = computeActivityField<int>(services, "runNumber", fallbackActivity.mId);
  auto runType = computeActivityField<int>(services, "runType", fallbackActivity.mType);
  auto run_start_time_ms = computeActivityField<unsigned long>(services, "run_start_time_ms", fallbackActivity.mValidity.getMin());
  auto run_stop_time_ms = computeActivityField<unsigned long>(services, "run_stop_time_ms", fallbackActivity.mValidity.getMax());
  auto partitionName = services.get<framework::RawDeviceService>().device()->fConfig->GetProperty<std::string>("environment_id", fallbackActivity.mPartitionName);
  auto periodName = services.get<framework::RawDeviceService>().device()->fConfig->GetProperty<std::string>("periodName", "unspecified");
  auto fillNumber = computeActivityField<int>(services, "fill_info_fill_number", fallbackActivity.mFillNumber);
  auto beam_type = services.get<framework::RawDeviceService>().device()->fConfig->GetProperty<std::string>("beam_type", fallbackActivity.mBeamType);

  Activity activity( 
    runNumber,
    runType,
    fallbackActivity.mPeriodName,
    fallbackActivity.mPassName,
    fallbackActivity.mProvenance,
    { run_start_time_ms, run_stop_time_ms },
    beam_type,
    partitionName,
    fillNumber);

  return activity;
}

inline std::string indentTree(int level)
{
  return std::string(level * 2, ' ');
}

inline void printTree(const boost::property_tree::ptree& pt, int level = 0)
{
  std::stringstream ss;
  boost::property_tree::json_parser::write_json(ss, pt);
  for (std::string line; std::getline(ss, line, '\n');)
    ILOG(Debug, Trace) << line << ENDM;
}

inline std::vector<std::pair<std::string, std::string>> parseOverrideValues(const std::string& input)
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

inline void overrideValues(boost::property_tree::ptree& tree, std::vector<std::pair<std::string, std::string>> keyValues)
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
inline std::string templateILDiscardFile(std::string& originalFile, framework::InitContext& iCtx)
{
  try {
    auto& deviceSpec = iCtx.services().get<o2::framework::DeviceSpec const>();
    return std::regex_replace(originalFile, std::regex("_ID_"), deviceSpec.id);
  } catch (...) {
    ILOG(Error, Devel) << "exception caught and swallowed in templateILDiscardFile : " << boost::current_exception_diagnostic_information() << ENDM;
  }
  return originalFile;
}

uint64_t getCurrentTimestamp();

} // namespace o2::quality_control::core

#endif // QUALITYCONTROL_RUNNERUTILS_H
