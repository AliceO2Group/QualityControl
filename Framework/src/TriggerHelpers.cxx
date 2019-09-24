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
#include <boost/algorithm/string.hpp>

namespace o2::quality_control::postprocessing::trigger_helpers
{

TriggerFcn TriggerFactory(std::string trigger)
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
  } else if (trigger.find("sec") != std::string::npos ||
             trigger.find("min") != std::string::npos ||
             trigger.find("hour") != std::string::npos) {
    // todo: extract time
    //  it might be in a form of "10sec", "5min", "1hour", etc...
    return triggers::Periodic(-1.0);
  } else {
    throw std::runtime_error("unknown trigger: " + trigger);
  }
}

Trigger tryTrigger(std::vector<TriggerFcn>& triggerFcns)
{
  for (auto& triggerFcn : triggerFcns) {
    if (Trigger trigger = triggerFcn()) {
      return trigger;
    }
  }
  return Trigger::No;
}

std::vector<TriggerFcn> createTriggers(const std::vector<std::string>& triggerNames)
{
  std::vector<TriggerFcn> triggerFcns;
  for (const auto& triggerName : triggerNames) {
    triggerFcns.push_back(TriggerFactory(triggerName));
  }
  return std::move(triggerFcns);
}

} // namespace o2::quality_control::postprocessing::trigger_helpers
