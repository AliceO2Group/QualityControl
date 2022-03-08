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
/// \file    Triggers.cxx
/// \author  Piotr Konopka
///

#include "QualityControl/Triggers.h"
#include "QualityControl/QcInfoLogger.h"
#include "QualityControl/DatabaseHelpers.h"

#include <CCDB/CcdbApi.h>
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
  ILOG(Warning, Support) << "TriggerType '" << triggerName << "' is not implemented yet. It will always return TriggerType::No" << ENDM;
  return [triggerName]() mutable -> Trigger {
    return { TriggerType::No, true };
  };
}

TriggerFcn StartOfRun(const core::Activity&)
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

TriggerFcn Once(const core::Activity& activity)
{
  return [hasTriggered = false, activity]() mutable -> Trigger {
    if (hasTriggered) {
      return { TriggerType::No, true, activity };
    } else {
      hasTriggered = true;
      return { TriggerType::Once, true, activity };
    }
  };
}

TriggerFcn Always(const core::Activity& activity)
{
  return [activity]() mutable -> Trigger {
    return { TriggerType::Always, false, activity };
  };
}

TriggerFcn Never(const core::Activity& activity)
{
  return [activity]() mutable -> Trigger {
    return { TriggerType::No, true, activity };
  };
}

TriggerFcn EndOfRun(const core::Activity&)
{
  return NotImplemented("EndOfRun");
}

TriggerFcn StartOfFill(const core::Activity&)
{
  return NotImplemented("StartOfFill");
}

TriggerFcn EndOfFill(const core::Activity&)
{
  return NotImplemented("EndOfFill");
}

TriggerFcn Periodic(double seconds, const core::Activity& activity)
{
  AliceO2::Common::Timer timer;
  timer.reset(static_cast<int>(seconds * 1000000));

  return [timer, activity]() mutable -> Trigger {
    if (timer.isTimeout()) {
      // We calculate the exact time when timer has passed
      uint64_t timestamp = Trigger::msSinceEpoch() + static_cast<int>(timer.getRemainingTime() * 1000);
      // increment until it is cleared (in case that more than one cycle has passed)
      // let's hope there is no bug, which would make us stay in that loop forever
      while (timer.isTimeout()) {
        timer.increment();
      }
      return { TriggerType::Periodic, false, activity, timestamp };
    } else {
      return { TriggerType::No, false };
    }
  };
}

TriggerFcn NewObject(std::string databaseUrl, std::string objectPath, const core::Activity& activity)
{
  // Key names in the header map.
  constexpr auto md5key = "Content-MD5";
  constexpr auto timestampKey = "Valid-From";

  // We support only CCDB here.
  auto db = std::make_shared<o2::ccdb::CcdbApi>();
  db->init(databaseUrl);
  if (!db->isHostReachable()) {
    ILOG(Error, Support) << "CCDB at URL '" << databaseUrl << "' is not reachable." << ENDM;
  }

  // We rely on changing MD5 - if the object has changed, it should have a different check sum.
  // If someone reuploaded an old object, it should not have an influence.
  std::string lastMD5;
  auto metadata = repository::database_helpers::asDatabaseMetadata(activity);
  if (auto headers = db->retrieveHeaders(objectPath, metadata); headers.count(md5key)) {
    lastMD5 = headers[md5key];
  } else {
    // We don't make a fuss over it, because we might be just waiting for the first version of such object.
    // It should not happen often though, so having a warning makes sense.
    ILOG(Warning, Support) << "No MD5 of the file '" << objectPath << "' in the db '" << databaseUrl << "', probably the file is missing." << ENDM;
  }

  return [db, databaseUrl = std::move(databaseUrl), objectPath = std::move(objectPath), lastMD5, activity, metadata]() mutable -> Trigger {
    if (auto headers = db->retrieveHeaders(objectPath, metadata); headers.count(md5key)) {
      auto newMD5 = headers[md5key];
      if (lastMD5 != newMD5) {
        lastMD5 = newMD5;
        return { TriggerType::NewObject, false, activity, std::stoull(headers[timestampKey]) };
      }
    } else {
      // We don't make a fuss over it, because we might be just waiting for the first version of such object.
      // It should not happen often though, so having a warning makes sense.
      ILOG(Warning, Support) << "No MD5 of the file '" << objectPath << "' in the db '" << databaseUrl << "', probably the file is missing." << ENDM;
    }

    return { TriggerType::No, false };
  };
}

} // namespace triggers

} // namespace o2::quality_control::postprocessing
