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
/// \file   runnerUtils.h
/// \author Barthelemy von Haller
///

#ifndef QUALITYCONTROL_RUNNERUTILS_H
#define QUALITYCONTROL_RUNNERUTILS_H

#include <string>
#include <Configuration/ConfigurationFactory.h>
#include <Common/Exceptions.h>
#include <Framework/RawDeviceService.h>
#include <FairMQDevice.h>
#include <Framework/ConfigParamRegistry.h>

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

inline int computeRunNumber(const framework::ServiceRegistry& services, const boost::property_tree::ptree& config)
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
  run = run > 0 /* found it in service */ ? run : config.get<int>("qc.config.Activity.number", 0);
  ILOG(Debug, Devel) << "Run number returned by computeRunNumber (tree) : " << run << ENDM;
  return run;
}

inline std::string computePeriodName(const framework::ServiceRegistry& services, const boost::property_tree::ptree& config)
{ // Determine period
  std::string periodName;
  periodName = services.get<framework::RawDeviceService>().device()->fConfig->GetProperty<std::string>("periodName", "unspecified");
  ILOG(Info, Devel) << "Got this property periodName from RawDeviceService: '" << periodName << "'" << ENDM;
  periodName = periodName != "unspecified" /* found it in service */ ? periodName : config.get<std::string>("qc.config.Activity.periodName", "");
  ILOG(Debug, Devel) << "Period Name returned by computePeriodName : " << periodName << ENDM;
  return periodName;
}

inline std::string computePassType(const boost::property_tree::ptree& config)
{
  std::string passType;
  passType = config.get<std::string>("qc.config.Activity.passType", "");
  ILOG(Debug, Devel) << "Period Name returned by computePassType : " << passType << ENDM;
  return passType;
}

inline std::string computeProvenance(const boost::property_tree::ptree& config)
{
  std::string provenance;
  provenance = config.get<std::string>("qc.config.Activity.provenance", "qc");
  ILOG(Debug, Devel) << "Period Name returned by computeProvenance : " << provenance << ENDM;
  return provenance;
}

inline int computeRunNumber(const framework::ServiceRegistry& services, int defaultRunNumber = 0)
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
  run = run > 0 /* found it in service */ ? run : defaultRunNumber;
  ILOG(Debug, Devel) << "Run number returned by computeRunNumber (default) : " << run << ENDM;
  return run;
}

} // namespace o2::quality_control::core

#endif //QUALITYCONTROL_RUNNERUTILS_H
