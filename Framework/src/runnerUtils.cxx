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
#include "QualityControl/stringUtils.h"

#include <boost/property_tree/json_parser.hpp>
#include <CommonUtils/StringUtils.h>
#include <regex>
#include <Configuration/ConfigurationFactory.h>
#include <Common/Exceptions.h>
#include <Framework/DeviceSpec.h>
#include <Framework/ConfigParamRegistry.h>

#include <chrono>
#include <cstdint>
#include <DataFormatsParameters/ECSDataAdapters.h>

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

/**
 * If the runType is of legacy type, i.e. an integer, the corresponding string representation is returned.
 * In case we cannot find a string representation we use "NONE".
 * @param runType
 */
std::string_view translateIntegerRunType(const std::string& runType)
{
  // runType used to be an integer. If we find an integer in a config file, the risk is that it is translated directly to a string (2->"2").
  // We must rather translate the integer into the corresponding run type string if at all possible.
  if (isUnsignedInteger(runType)) {
    try {
      ILOG(Warning, Ops) << "Activity type was provided as an integer. A matching activity type could be found: " << parameters::GRPECS::RunTypeNames.at(std::stoi(runType)) << ". Consider using the string representation of the run type." << ENDM;
      return parameters::GRPECS::RunTypeNames.at(std::stoi(runType));
    } catch (std::out_of_range& exc) {
      ILOG(Warning, Ops) << "Activity type was provided as an integer. No matching activity type could be find. Using 'NONE'." << ENDM;
      return "NONE";
    }
  }
  return runType;
}

std::string computeStringActivityField(framework::ServiceRegistryRef services, const std::string& name, const std::string& fallBack)
{
  auto property = services.get<framework::RawDeviceService>().device()->fConfig->GetProperty<std::string>(name, fallBack);
  ILOG(Info, Devel) << "Got this property '" << name << "' from RawDeviceService (fallback was " << fallBack << ") : '" << property << "'" << ENDM;
  return property;
}

std::string translateBeamType(const std::string& pdpBeamType)
{
  // convert the beam type received from pdp into the format we use in flp/ecs
  std::string result = "";
  if (pdpBeamType == "pp") {
    result = "PROTON-PROTON";
  } else if (pdpBeamType == "PbPb") {
    result = "Pb-Pb";
  } else if (pdpBeamType == "pPb") {
    result = "Pb-PROTON";
  } else {
    ILOG(Warning, Ops) << "Failed to convert the pdp beam type ('" << pdpBeamType << "'), returning an empty string" << ENDM;
  }
  ILOG(Debug, Devel) << "Translated pdp beam type '" << pdpBeamType << "' to '" << result << "'" << ENDM;
  return result;
}

Activity computeActivity(framework::ServiceRegistryRef services, const Activity& fallbackActivity)
{
  // for a complete list of the properties provided by ECS, see here: https://github.com/AliceO2Group/Control/blob/master/docs/handbook/configuration.md#variables-pushed-to-controlled-tasks
  auto runNumber = computeNumericalActivityField<int>(services, "runNumber", fallbackActivity.mId);
  std::string runType = computeStringActivityField(services, "run_type", fallbackActivity.mType);
  runType = translateIntegerRunType(runType);
  auto runStartTimeMs = computeNumericalActivityField<unsigned long>(services, "run_start_time_ms", fallbackActivity.mValidity.getMin());
  auto runEndTimeMs = computeNumericalActivityField<unsigned long>(services, "run_end_time_ms", fallbackActivity.mValidity.getMax());
  auto partitionName = computeStringActivityField(services, "environment_id", fallbackActivity.mPartitionName);
  auto periodName = computeStringActivityField(services, "lhc_period", fallbackActivity.mPeriodName);
  auto fillNumber = computeNumericalActivityField<int>(services, "fill_info_fill_number", fallbackActivity.mFillNumber);
  auto beam_type = computeStringActivityField(services, "pdp_beam_type", fallbackActivity.mBeamType);
  beam_type = translateBeamType(beam_type);

  Activity activity(
    runNumber,
    runType,
    periodName,
    fallbackActivity.mPassName,
    fallbackActivity.mProvenance,
    { runStartTimeMs, runEndTimeMs },
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

void initInfologger(framework::InitContext& iCtx, core::LogDiscardParameters infologgerDiscardParameters, std::string facility, std::string detectorName)
{
  AliceO2::InfoLogger::InfoLoggerContext* ilContext = nullptr;
  AliceO2::InfoLogger::InfoLogger* il = nullptr;
  try {
    ilContext = &iCtx.services().get<AliceO2::InfoLogger::InfoLoggerContext>();
    il = &iCtx.services().get<AliceO2::InfoLogger::InfoLogger>();
  } catch (const framework::RuntimeErrorRef& err) {
    ILOG(Error, Devel) << "Could not find the DPL InfoLogger" << ENDM;
  }

  infologgerDiscardParameters.file = templateILDiscardFile(infologgerDiscardParameters.file, iCtx);
  QcInfoLogger::init(facility,
                     infologgerDiscardParameters,
                     il,
                     ilContext);
  if (!detectorName.empty()) {
    QcInfoLogger::setDetector(detectorName);
  }
}

} // namespace o2::quality_control::core
