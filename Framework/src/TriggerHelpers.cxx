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
/// \file    TriggerHelpers.cxx
/// \author  Piotr Konopka
///

#include "QualityControl/TriggerHelpers.h"
#include "QualityControl/QcInfoLogger.h"
#include <boost/algorithm/string.hpp>
#include <optional>

using namespace o2::quality_control::core;

namespace o2::quality_control::postprocessing::trigger_helpers
{

std::optional<double> string2Seconds(std::string str)
{
  constexpr static char secondsStr[] = "sec";
  constexpr static char minutesStr[] = "min";
  constexpr static char hoursStr[] = "hour";

  try {
    if (size_t p = str.find(secondsStr); p != std::string::npos) {
      return 1.0 * std::stod(str.substr(0, p));
    } else if (size_t p = str.find(minutesStr); p != std::string::npos) {
      return 60.0 * std::stod(str.substr(0, p));
    } else if (size_t p = str.find(hoursStr); p != std::string::npos) {
      return 3600.0 * std::stod(str.substr(0, p));
    } else {
      return {};
    }
  } catch (std::invalid_argument& ex) {
    ILOG(Error, Ops) << "Unexpected format of string describing time '" << str << "'" << ENDM;
    throw ex;
  } catch (std::out_of_range& ex) {
    ILOG(Error, Ops) << "Trying to convert time, which is out of supported range '" << str << "'" << ENDM;
    throw ex;
  }
}

TriggerFcn triggerFactory(std::string trigger, const PostProcessingConfig& config)
{
  // todo: should we accept many versions of trigger names?
  std::string triggerLowerCase = trigger;
  boost::algorithm::to_lower(triggerLowerCase);
  auto& activity = config.activity;

  if (triggerLowerCase == "once") {
    return triggers::Once(activity);
  } else if (triggerLowerCase == "always") {
    return triggers::Always(activity);
  } else if (triggerLowerCase == "sor" || triggerLowerCase == "startofrun") {
    return triggers::StartOfRun(activity);
  } else if (triggerLowerCase == "eor" || triggerLowerCase == "endofrun") {
    return triggers::EndOfRun(activity);
  } else if (triggerLowerCase == "sof" || triggerLowerCase == "startoffill") {
    return triggers::StartOfFill(activity);
  } else if (triggerLowerCase == "eof" || triggerLowerCase == "endoffill") {
    return triggers::EndOfFill(activity);
  } else if (triggerLowerCase.find("newobject") != std::string::npos) {
    // we expect the config string to be:
    // newobject:[qcdb/ccdb]:qc/path/to/object
    std::vector<std::string> tokens;
    boost::split(tokens, trigger, boost::is_any_of(":"));

    if (tokens.size() != 3) {
      throw std::invalid_argument(
        "The new object trigger is configured incorrectly. The expected format is "
        "'newobject:[qcdb/ccdb]:qc/path/to/object', received `" +
        trigger + "'");
    }

    std::string dbUrl;
    boost::algorithm::to_lower(tokens[1]);
    if (tokens[1] == "qcdb") {
      dbUrl = config.qcdbUrl;
    } else if (tokens[1] == "ccdb") {
      dbUrl = config.ccdbUrl;
    } else {
      throw std::invalid_argument("The second token in '" + trigger + "' should be either qcdb or ccdb");
    }

    if (tokens[2].empty()) {
      throw std::invalid_argument("The third token in '" + trigger + "' is empty, but it should contain the object path");
    }
    const auto& objectPath = tokens[2];
    return triggers::NewObject(dbUrl, objectPath, activity);
  } else if (triggerLowerCase.find("foreachobject") != std::string::npos) {
    // we expect the config string to be:
    // foreachobjects:[qcdb/ccdb]:qc/path/to/object
    std::vector<std::string> tokens;
    boost::split(tokens, trigger, boost::is_any_of(":"));

    if (tokens.size() != 3) {
      throw std::invalid_argument(
        "The 'for each' trigger is configured incorrectly. The expected format is "
        "'foreachobjects:[qcdb/ccdb]:qc/path/to/object', received `" +
        trigger + "'");
    }

    std::string dbUrl;
    boost::algorithm::to_lower(tokens[1]);
    if (tokens[1] == "qcdb") {
      dbUrl = config.qcdbUrl;
    } else if (tokens[1] == "ccdb") {
      dbUrl = config.ccdbUrl;
    } else {
      throw std::invalid_argument("The second token in '" + trigger + "' should be either qcdb or ccdb");
    }

    if (tokens[2].empty()) {
      throw std::invalid_argument("The third token in '" + trigger + "' is empty, but it should contain the object path");
    }
    const auto& objectPath = tokens[2];
    return triggers::ForEachObject(dbUrl, objectPath, activity);
  } else if (auto seconds = string2Seconds(triggerLowerCase); seconds.has_value()) {
    if (seconds.value() < 0) {
      throw std::invalid_argument("negative number of seconds in trigger '" + trigger + "'");
    }
    return triggers::Periodic(seconds.value(), activity);
  } else if (triggerLowerCase.find("user") != std::string::npos || triggerLowerCase.find("control") != std::string::npos) {
    return triggers::Never(activity);
  } else {
    throw std::invalid_argument("unknown trigger: " + trigger);
  }
}

Trigger tryTrigger(std::vector<TriggerFcn>& triggerFcns)
{
  Trigger result(TriggerType::No);
  auto it = triggerFcns.begin();
  while (it != triggerFcns.end()) {
    auto trigger = (*it)();
    it = trigger.last ? triggerFcns.erase(it) : it + 1;
    if (trigger) {
      return trigger;
    }
  }
  return { TriggerType::No };
}

std::vector<TriggerFcn> createTriggers(const std::vector<std::string>& triggerNames, const PostProcessingConfig& config)
{
  std::vector<TriggerFcn> triggerFcns;
  triggerFcns.reserve(triggerNames.size());
  for (const auto& triggerName : triggerNames) {
    triggerFcns.push_back(triggerFactory(triggerName, config));
  }
  return triggerFcns;
}

bool hasUserOrControlTrigger(const std::vector<std::string>& triggerNames)
{
  return std::find_if(triggerNames.begin(), triggerNames.end(), [](std::string name) {
           boost::algorithm::to_lower(name);
           return name.find("user") != std::string::npos || name.find("control") != std::string::npos;
         }) != triggerNames.end();
}

} // namespace o2::quality_control::postprocessing::trigger_helpers
