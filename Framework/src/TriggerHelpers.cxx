// Copyright CERN and copyright holders of ALICE O2. This software is
// distributed under the terms of the GNU General Public License v3 (GPL
// Version 3), copied verbatim in the file "COPYING".
//
// See http://alice-o2.web.cern.ch/license for full licensing information.
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
    ILOG << LogErrorSupport << "Unexpected format of string describing time '" << str << "'" << ENDM;
    throw ex;
  } catch (std::out_of_range& ex) {
    ILOG << LogErrorSupport << "Trying to convert time, which is out of supported range '" << str << "'" << ENDM;
    throw ex;
  }
}

TriggerFcn triggerFactory(std::string trigger)
{
  // todo: should we accept many versions of trigger names?
  boost::algorithm::to_lower(trigger);

  if (trigger == "once") {
    return triggers::Once();
  } else if (trigger == "always") {
    return triggers::Always();
  } else if (trigger == "sor" || trigger == "startofrun") {
    return triggers::StartOfRun();
  } else if (trigger == "eor" || trigger == "endofrun") {
    return triggers::EndOfRun();
  } else if (trigger == "sof" || trigger == "startoffill") {
    return triggers::StartOfFill();
  } else if (trigger == "eof" || trigger == "endoffill") {
    return triggers::EndOfFill();
  } else if (trigger.find("newobject") != std::string::npos) {
    // todo: extract object path
    //  it might be in a form of "newobject:/qc/ASDF/ZXCV"
    return triggers::NewObject("");
  } else if (auto seconds = string2Seconds(trigger); seconds.has_value()) {
    if (seconds.value() < 0) {
      throw std::invalid_argument("negative number of seconds in trigger '" + trigger + "'");
    }
    return triggers::Periodic(seconds.value());
  } else if (trigger.find("user") != std::string::npos || trigger.find("control") != std::string::npos) {
    return triggers::Never();
  } else {
    throw std::invalid_argument("unknown trigger: " + trigger);
  }
}

Trigger tryTrigger(std::vector<TriggerFcn>& triggerFcns)
{
  for (auto& triggerFcn : triggerFcns) {
    if (auto trigger = triggerFcn()) {
      return trigger;
    }
  }
  return { TriggerType::No };
}

std::vector<TriggerFcn> createTriggers(const std::vector<std::string>& triggerNames)
{
  std::vector<TriggerFcn> triggerFcns;
  for (const auto& triggerName : triggerNames) {
    triggerFcns.push_back(triggerFactory(triggerName));
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
