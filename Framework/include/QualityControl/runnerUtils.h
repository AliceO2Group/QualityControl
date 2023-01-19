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
using boost::property_tree::ptree;

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

inline int computeRunNumber(framework::ServiceRegistryRef services, int fallbackRunNumber = 0)
{ // Determine run number
  int run = 0;
  try {
    auto temp = services.get<framework::RawDeviceService>().device()->fConfig->GetProperty<std::string>("runNumber", "unspecified");
    ILOG(Info, Devel) << "Got this property runNumber from RawDeviceService: '" << temp << "'" << ENDM;
    run = stoi(temp);
    ILOG(Info, Support) << "   Run number found in options: " << run << ENDM;
  } catch (std::invalid_argument& ia) {
    ILOG(Info, Support) << "   Run number not found in options or is not a number, \n"
                           "   using the one from the config file or 0 as a last resort."
                        << ENDM;
  }
  run = run > 0 /* found it in service */ ? run : fallbackRunNumber;
  ILOG(Debug, Devel) << "Run number returned by computeRunNumber (default) : " << run << ENDM;
  return run;
}

inline int computeRunType(framework::ServiceRegistryRef services, int fallbackRunType = 0)
{ // Determine runType number
  int runType = 0;
  try {
    auto temp = services.get<framework::RawDeviceService>().device()->fConfig->GetProperty<std::string>("runType", "unspecified");
    ILOG(Info, Devel) << "Got this property runType from RawDeviceService: '" << temp << "'" << ENDM;
    runType = stoi(temp);
    ILOG(Info, Support) << "   Run type found in options: " << runType << ENDM;
  } catch (std::invalid_argument& ia) {
    ILOG(Info, Support) << "   Run type not found in options or is not a number, \n"
                           "   using the one from the config file or 0 as a last resort."
                        << ENDM;
  }
  runType = runType > 0 /* found it in service */ ? runType : fallbackRunType;
  ILOG(Debug, Devel) << "Run type returned by computeRunType (default) : " << runType << ENDM;
  return runType;
}

inline std::string computePartitionName(framework::ServiceRegistryRef services, const std::string& fallbackPartitionName = "")
{
  std::string partitionName;
  partitionName = services.get<framework::RawDeviceService>().device()->fConfig->GetProperty<std::string>("environment_id", "unspecified");
  ILOG(Info, Devel) << "Got this property partitionName from RawDeviceService: '" << partitionName << "'" << ENDM;
  partitionName = partitionName != "unspecified" /* found it in service */ ? partitionName : fallbackPartitionName;
  ILOG(Debug, Devel) << "Period Name returned by computePeriodName : " << partitionName << ENDM;
  return partitionName;
}

inline std::string computePeriodName(framework::ServiceRegistryRef services, const std::string& fallbackPeriodName = "")
{ // Determine period
  std::string periodName;
  periodName = services.get<framework::RawDeviceService>().device()->fConfig->GetProperty<std::string>("periodName", "unspecified");
  ILOG(Info, Devel) << "Got this property periodName from RawDeviceService: '" << periodName << "'" << ENDM;
  periodName = periodName != "unspecified" /* found it in service */ ? periodName : fallbackPeriodName;
  ILOG(Debug, Devel) << "Period Name returned by computePeriodName : " << periodName << ENDM;
  return periodName;
}

inline std::string computePassName(const std::string& fallbackPassName = "")
{
  ILOG(Debug, Devel) << "Pass Name returned by computePassName : " << fallbackPassName << ENDM;
  return fallbackPassName;
}

inline std::string computeProvenance(const std::string& fallbackProvenance = "")
{
  ILOG(Debug, Devel) << "Provenance returned by computeProvenance : " << fallbackProvenance << ENDM;
  return fallbackProvenance;
}

inline Activity computeActivity(framework::ServiceRegistryRef services, const Activity& fallbackActivity)
{
  return {
    computeRunNumber(services, fallbackActivity.mId),
    computeRunType(services, fallbackActivity.mType),
    computePeriodName(services, fallbackActivity.mPeriodName),
    computePassName(fallbackActivity.mPassName),
    computeProvenance(fallbackActivity.mProvenance),
    fallbackActivity.mValidity
  };
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

// Create necessary escape sequences from illegal characters
template<class Ch>
std::basic_string<Ch> create_escapes(const std::basic_string<Ch> &s)
{
  std::basic_string<Ch> result;
  typename std::basic_string<Ch>::const_iterator b = s.begin();
  typename std::basic_string<Ch>::const_iterator e = s.end();
  while (b != e)
  {
    if (*b == Ch('\0')) result += Ch('\\'), result += Ch('0');
    else if (*b == Ch('\a')) result += Ch('\\'), result += Ch('a');
    else if (*b == Ch('\b')) result += Ch('\\'), result += Ch('b');
    else if (*b == Ch('\f')) result += Ch('\\'), result += Ch('f');
    else if (*b == Ch('\n')) result += Ch('\\'), result += Ch('n');
    else if (*b == Ch('\r')) result += Ch('\\'), result += Ch('r');
    else if (*b == Ch('\v')) result += Ch('\\'), result += Ch('v');
    else if (*b == Ch('"')) result += Ch('\\'), result += Ch('"');
    else if (*b == Ch('\\')) result += Ch('\\'), result += Ch('\\');
    else
      result += *b;
    ++b;
  }
  return result;
}

// Freely inspired from write_json_helper in boost
template <class Ptree>
void mergeInto(const Ptree& pt, ptree& destination, ptree& parent, const std::string& fullPath , int indent)
{
  typedef typename Ptree::key_type::value_type Ch;
  typedef typename std::basic_string<Ch> Str;

  // Value or object or array
  if (indent > 0 && pt.empty()) { // Handle value
    Str data = /*create_escapes(*/pt.template get_value<Str>()/*)*/;
    parent.add(fullPath, data);
  } else if (indent > 0 && pt.count(Str()) == pt.size()) { // Handle array
    typename Ptree::const_iterator it = pt.begin();
    for (; it != pt.end(); ++it) {
      ptree object;
      std::string newFullPath = fullPath.empty() ? it->first : fullPath + "." + it->first;
      mergeInto(it->second, destination, object, "", indent + 1);
      parent.add_child(newFullPath, object);
    }
  } else { // Handle object
    typename Ptree::const_iterator it = pt.begin();
    for (; it != pt.end(); ++it) {
      std::string newFullPath = fullPath.empty() ? it->first : fullPath + "." + it->first;
      mergeInto(it->second, destination, parent, newFullPath, indent + 1);
    }
  }
}

template <class Ptree>
void mergeInto(const Ptree& pt, ptree& destination)
{
  mergeInto(pt, destination, destination, "", 1);
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

} // namespace o2::quality_control::core

#endif // QUALITYCONTROL_RUNNERUTILS_H
