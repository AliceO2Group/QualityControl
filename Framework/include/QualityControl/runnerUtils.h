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
#include <QualityControl/QcInfoLogger.h>

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

inline int computeRunNumber(const framework::ServiceRegistry& services, int fallbackRunNumber = 0)
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

inline std::string computePartitionName(const framework::ServiceRegistry& services, const std::string& fallbackPartitionName = "")
{
  std::string partitionName;
  partitionName = services.get<framework::RawDeviceService>().device()->fConfig->GetProperty<std::string>("environment_id", "unspecified");
  ILOG(Info, Devel) << "Got this property partitionName from RawDeviceService: '" << partitionName << "'" << ENDM;
  partitionName = partitionName != "unspecified" /* found it in service */ ? partitionName : fallbackPartitionName;
  ILOG(Debug, Devel) << "Period Name returned by computePeriodName : " << partitionName << ENDM;
  return partitionName;
}

inline std::string computePeriodName(const framework::ServiceRegistry& services, const std::string& fallbackPeriodName = "")
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
  std::string passName;
  passName = fallbackPassName;
  ILOG(Debug, Devel) << "Pass Name returned by computePassName : " << passName << ENDM;
  return passName;
}

inline std::string computeProvenance(const std::string& fallbackProvenance = "")
{
  std::string provenance;
  provenance = fallbackProvenance;
  ILOG(Debug, Devel) << "Provenance returned by computeProvenance : " << provenance << ENDM;
  return provenance;
}

inline std::string indentTree(int level)
{
  std::string s;
  for (int i = 0; i<level; i++) s += "  ";
  return s;
}

inline void printTree(boost::property_tree::ptree &pt, int level=0)
{
  if (pt.empty()) {

    ILOG(Debug, Devel) <<"\"" <<  pt.data() << "\"";
  } else {
    if (level) {
      ILOG(Debug, Devel) << ENDM;
    }

    ILOG(Debug, Devel) << indentTree(level) << "{" << ENDM;

    for (boost::property_tree::ptree::iterator pos = pt.begin(); pos != pt.end();) {
      ILOG(Debug, Devel) << indentTree(level + 1) << "\"" << pos->first << "\": ";

      printTree(pos->second, level + 1);
      ++pos;
      if (pos != pt.end()) {
        ILOG(Debug, Devel) << ",";
      }
      ILOG(Debug, Devel) << ENDM;
    }

    ILOG(Debug, Devel) << indentTree(level) << " }";
  }
  ILOG(Debug, Devel) << ENDM;
  return;
}

  using namespace std;
  
  inline void printTreeToFile(boost::property_tree::ptree &pt, stringstream& ss, int level=0)
{
  // Create and open a text file
  //  ofstream MyFile("/tmp/ptreedump.txt");
  if (pt.empty()) {

    ss << "\"" << pt.data() << "\"";
  } else {
    if (level) {
      ss << "\n";
    }

    ss << indentTree(level) << "{\n";

    for (boost::property_tree::ptree::iterator pos = pt.begin(); pos != pt.end();) {
      ss<< indentTree(level + 1) << "\"" << pos->first << "\": ";

      printTreeToFile(pos->second, ss, level + 1);
      ++pos;
      if (pos != pt.end()) {
        ss <<indentTree(level)<< ",";
      }
      ss << "\n";
    }

    ss << indentTree(level) << " }";
  }
  ss<< "\n";
  // Close the file
  //MyFile.close();
  return;
}


} // namespace o2::quality_control::core

#endif //QUALITYCONTROL_RUNNERUTILS_H
