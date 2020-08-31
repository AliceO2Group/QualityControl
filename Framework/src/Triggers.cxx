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
/// \file    Triggers.cxx
/// \author  Piotr Konopka
///

#include "QualityControl/Triggers.h"
#include "QualityControl/QcInfoLogger.h"

#include <Common/Timer.h>
#include <chrono>
#include <ostream>

using namespace std::chrono;
using namespace o2::quality_control::core;
namespace o2::quality_control::postprocessing
{

std::ostream& operator<<(std::ostream& out, const Trigger& t)
{
  out << "triggerType: " << t.triggerType << ", timestamp: " << t.timestamp;
  return out;
}

uint64_t Trigger::msSinceEpoch()
{
  return duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
}

namespace triggers
{

TriggerFcn NotImplemented(std::string triggerName)
{
  ILOG << LogWarningSupport << "TriggerType '" << triggerName << "' is not implemented yet. It will always return TriggerType::No" << ENDM;
  return [triggerName]() mutable -> Trigger {
    return { TriggerType::No };
  };
}

TriggerFcn StartOfRun()
{
  return NotImplemented("StartOfRun");

  // FIXME: it has to be initialized before the SOR, to actually catch it. Is it a problem?
  //  bool runStarted = false; // runOngoing();
  //  int runNumber = false;   // getRunNumber();
  //
  //  return [runStarted, runNumber]() mutable -> TriggerType {
  //    bool trigger = false;

  // trigger if:
  // - previously the run was not started
  //    trigger |= !runStarted && runOngoing();
  // - previously there was a different run number (we missed a stop and start sequence)
  //    trigger |= runOngoing() && runNumber != getRunNumber();

  //    runStarted = runOngoing();
  //    runNumber = getRunNumber();
  //
  //    return trigger ? TriggerType::StartOfRun : TriggerType::No;
  //  };
}

TriggerFcn Once()
{
  return [hasTriggered = false]() mutable -> Trigger {
    if (hasTriggered) {
      return { TriggerType::No };
    } else {
      hasTriggered = true;
      return { TriggerType::Once };
    }
  };
}

TriggerFcn Always()
{
  return []() mutable -> Trigger {
    return { TriggerType::Always };
  };
}

TriggerFcn Never()
{
  return []() mutable -> Trigger {
    return { TriggerType::No };
  };
}

TriggerFcn EndOfRun()
{
  return NotImplemented("EndOfRun");
}

TriggerFcn StartOfFill()
{
  return NotImplemented("StartOfFill");
}

TriggerFcn EndOfFill()
{
  return NotImplemented("EndOfFill");
}

TriggerFcn Periodic(double seconds)
{
  AliceO2::Common::Timer timer;
  timer.reset(static_cast<int>(seconds * 1000000));

  return [timer]() mutable -> Trigger {
    if (timer.isTimeout()) {
      // We calculate the exact time when timer has passed
      uint64_t timestamp = Trigger::msSinceEpoch() + static_cast<int>(timer.getRemainingTime() * 1000);
      // increment until it is cleared (in case that more than one cycle has passed)
      // let's hope there is no bug, which would make us stay in that loop forever
      while (timer.isTimeout()) {
        timer.increment();
      }
      return { TriggerType::Periodic, timestamp };
    } else {
      return { TriggerType::No };
    }
  };
}

TriggerFcn NewObject(std::string /*name*/)
{
  return NotImplemented("NewObject");
}

} // namespace triggers

} // namespace o2::quality_control::postprocessing
