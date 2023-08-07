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
#include <Framework/ServiceRegistryRef.h>
#include <Framework/RawDeviceService.h>
#include <fairmq/Device.h>
#include "QualityControl/QcInfoLogger.h"
#include "QualityControl/Activity.h"

namespace o2::quality_control::core
{

/**
 * Returns the name of the first task encountered in the config file.
 * Ad-hoc solution to avoid hard-coding the task when we create the printer (he needs it to know the data description
 * of the data coming out of the checker).
 * @param config
 * @return The name of the first task in the config file.
 */
std::string getFirstTaskName(const std::string& configurationSource);
std::string getFirstCheckName(const std::string& configurationSource);
bool hasChecks(const std::string& configSource);

template <typename T> // TODO we should probably limit T to numbers somehow
T computeActivityField(framework::ServiceRegistryRef services, const std::string& name, T fallbackNumber = 0)
{
  T result = 0;

  try {
    auto temp = services.get<framework::RawDeviceService>().device()->fConfig->GetProperty<std::string>(name);
    ILOG(Info, Devel) << "Got this property " << name << " from RawDeviceService: '" << temp << "'" << ENDM;
    result = boost::lexical_cast<T>(temp);
  } catch (std::invalid_argument& ia) {
    ILOG(Info, Devel) << "   " << name << " is not a number, using the fallback." << ENDM;
  } catch (fair::mq::PropertyNotFoundError& err) {
    ILOG(Info, Devel) << "   " << name << " not found in options, using the fallback." << ENDM;
  } catch (boost::bad_lexical_cast& err) {
    ILOG(Info, Devel) << "   " << name << " could not be cast to a number (" << err.what() << "), using the fallback." << ENDM;
  }
  result = result > 0 /* found it in service */ ? result : fallbackNumber;
  ILOG(Info, Devel) << name << " returned by computeActivityField (default) : " << result << ENDM;
  return result;
}

Activity computeActivity(framework::ServiceRegistryRef services, const Activity& fallbackActivity);

std::string indentTree(int level);
void printTree(const boost::property_tree::ptree& pt, int level = 0);

std::vector<std::pair<std::string, std::string>> parseOverrideValues(const std::string& input);
void overrideValues(boost::property_tree::ptree& tree, const std::vector<std::pair<std::string, std::string>>& keyValues);

/**
 * template the param infologgerDiscardFile (_ID_->[device-id])
 * @param originalFile
 * @param iCtx
 * @return
 */
std::string templateILDiscardFile(std::string& originalFile, framework::InitContext& iCtx);

uint64_t getCurrentTimestamp();

} // namespace o2::quality_control::core

#endif // QUALITYCONTROL_RUNNERUTILS_H
