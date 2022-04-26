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
#include "QualityControl/CcdbDatabase.h"

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

TriggerFcn StartOfRun(const Activity&)
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

TriggerFcn Once(const Activity& activity)
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

TriggerFcn Always(const Activity& activity)
{
  return [activity]() mutable -> Trigger {
    return { TriggerType::Always, false, activity };
  };
}

TriggerFcn Never(const Activity& activity)
{
  return [activity]() mutable -> Trigger {
    return { TriggerType::No, true, activity };
  };
}

TriggerFcn EndOfRun(const Activity&)
{
  return NotImplemented("EndOfRun");
}

TriggerFcn StartOfFill(const Activity&)
{
  return NotImplemented("StartOfFill");
}

TriggerFcn EndOfFill(const Activity&)
{
  return NotImplemented("EndOfFill");
}

TriggerFcn Periodic(double seconds, const Activity& activity)
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

TriggerFcn NewObject(std::string databaseUrl, std::string databaseType, std::string objectPath, const Activity& activity)
{
  // Key names in the header map.
  constexpr auto md5key = "Content-MD5";
  constexpr auto timestampKey = "Valid-From";
  auto fullObjectPath = (databaseType == "qcdb" ? activity.mProvenance + "/" : "") + objectPath;

  // We support only CCDB here.
  auto db = std::make_shared<o2::ccdb::CcdbApi>();
  db->init(databaseUrl);
  if (!db->isHostReachable()) {
    ILOG(Error, Support) << "CCDB at URL '" << databaseUrl << "' is not reachable." << ENDM;
  }

  // We rely on changing MD5 - if the object has changed, it should have a different check sum.
  // If someone reuploaded an old object, it should not have an influence.
  std::string lastMD5;
  auto metadata = repository::database_helpers::asDatabaseMetadata(activity, false);
  if (auto headers = db->retrieveHeaders(fullObjectPath, metadata); headers.count(md5key)) {
    lastMD5 = headers[md5key];
  } else {
    // We don't make a fuss over it, because we might be just waiting for the first version of such object.
    // It should not happen often though, so having a warning makes sense.
    ILOG(Warning, Support) << "No MD5 of the file '" << fullObjectPath << "' in the db '" << databaseUrl << "', probably the file is missing." << ENDM;
  }

  return [db, databaseUrl = std::move(databaseUrl), fullObjectPath = std::move(fullObjectPath), lastMD5, activity, metadata]() mutable -> Trigger {
    if (auto headers = db->retrieveHeaders(fullObjectPath, metadata); headers.count(md5key)) {
      auto newMD5 = headers[md5key];
      if (lastMD5 != newMD5) {
        lastMD5 = newMD5;
        return { TriggerType::NewObject, false, activity, std::stoull(headers[timestampKey]) };
      }
    } else {
      // We don't make a fuss over it, because we might be just waiting for the first version of such object.
      // It should not happen often though, so having a warning makes sense.
      ILOG(Warning, Support) << "No MD5 of the file '" << fullObjectPath << "' in the db '" << databaseUrl << "', probably the file is missing." << ENDM;
    }

    return { TriggerType::No, false };
  };
}

TriggerFcn ForEachObject(std::string databaseUrl, std::string databaseType, std::string objectPath, const Activity& activity)
{
  // Key names in the header map.
  constexpr auto md5key = "Content-MD5";
  constexpr auto timestampKey = "Valid-From";
  auto fullObjectPath = (databaseType == "qcdb" ? activity.mProvenance + "/" : "") + objectPath;

  // We support only CCDB here.
  auto db = std::make_shared<repository::CcdbDatabase>();
  db->connect(databaseUrl, "", "", "");

  auto objects = db->getListingAsPtree(fullObjectPath).get_child("objects");
  ILOG(Info, Support) << "Got " << objects.size() << " objects for the path '" << fullObjectPath << "'" << ENDM;
  auto filteredObjects = std::make_shared<std::vector<boost::property_tree::ptree>>();
  const auto& filter = activity;

  ILOG(Debug, Devel) << "Filter activity: " << activity << ENDM;

  // As for today, we receive objects in the order of the newest to the oldest.
  // We prefer the other order here.
  for (auto rit = objects.rbegin(); rit != objects.rend(); ++rit) {
    auto objectActivity = repository::database_helpers::asActivity(rit->second);
    if (filter.matches(objectActivity)) {
      filteredObjects->emplace_back(rit->second);
      ILOG(Debug, Devel) << "Matched an object with activity: " << activity << ENDM;
    }
  }
  ILOG(Info, Support) << filteredObjects->size() << " objects matched the specified activity" << ENDM;

  // we make sure it is sorted. If it is already, it shouldn't cost much.
  std::sort(filteredObjects->begin(), filteredObjects->end(),
            [](const boost::property_tree::ptree& a, const boost::property_tree::ptree& b) {
              return a.get<int64_t>(timestampKey) < b.get<int64_t>(timestampKey);
            });

  return [filteredObjects, activity, currentObject = filteredObjects->begin()]() mutable -> Trigger {
    if (currentObject != filteredObjects->end()) {
      auto currentActivity = repository::database_helpers::asActivity(*currentObject);
      bool last = currentObject + 1 == filteredObjects->end();
      Trigger trigger(TriggerType::ForEachObject, last, currentActivity, currentObject->get<int64_t>(timestampKey));
      ++currentObject;
      return trigger;
    } else {
      return { TriggerType::No, true, activity };
    }
  };
}

TriggerFcn ForEachLatest(std::string databaseUrl, std::string databaseType, std::string objectPath, const Activity& activity)
{
  // Key names in the header map.
  constexpr auto md5key = "Content-MD5";
  constexpr auto timestampKey = "Created";
  auto fullObjectPath = (databaseType == "qcdb" ? activity.mProvenance + "/" : "") + objectPath;

  // We support only CCDB here.
  auto db = std::make_shared<repository::CcdbDatabase>();
  db->connect(databaseUrl, "", "", "");

  auto objects = db->getListingAsPtree(fullObjectPath).get_child("objects");
  ILOG(Info, Support) << "Got " << objects.size() << " objects for the path '" << fullObjectPath << "'" << ENDM;
  auto filteredObjects = std::make_shared<std::vector<std::pair<Activity, boost::property_tree::ptree>>>();
  const auto& filter = activity;

  ILOG(Debug, Devel) << "Filter activity: " << activity << ENDM;

  // As for today, we receive objects in the order of the newest to the oldest.
  // We prefer the other order here.
  for (auto rit = objects.rbegin(); rit != objects.rend(); ++rit) {
    auto objectActivity = repository::database_helpers::asActivity(rit->second);
    if (filter.matches(objectActivity)) {
      auto latestObject = std::find_if(filteredObjects->begin(), filteredObjects->end(), [&](const std::pair<Activity, boost::property_tree::ptree>& entry) {
        return entry.first.same(objectActivity);
      });
      if (latestObject != filteredObjects->end() && latestObject->second.get<int64_t>(timestampKey) < rit->second.get<int64_t>(timestampKey)) {
        *latestObject = { objectActivity, rit->second };
        ILOG(Debug, Devel) << "Updated the object with activity: " << objectActivity << ENDM;
      } else {
        filteredObjects->emplace_back(objectActivity, rit->second);
        ILOG(Debug, Devel) << "Matched an object with activity: " << objectActivity << ENDM;
      }
    }
  }
  ILOG(Info, Support) << filteredObjects->size() << " objects matched the specified activity" << ENDM;

  // we make sure it is sorted. If it is already, it shouldn't cost much.
  std::sort(filteredObjects->begin(), filteredObjects->end(),
            [](const std::pair<Activity, boost::property_tree::ptree>& a, const std::pair<Activity, boost::property_tree::ptree>& b) {
              return a.second.get<int64_t>(timestampKey) < b.second.get<int64_t>(timestampKey);
            });

  return [filteredObjects, activity, currentObject = filteredObjects->begin()]() mutable -> Trigger {
    if (currentObject != filteredObjects->end()) {
      const auto& currentActivity = currentObject->first;
      const auto& currentPtree = currentObject->second;
      bool last = currentObject + 1 == filteredObjects->end();
      Trigger trigger(TriggerType::ForEachLatest, last, currentActivity, currentPtree.get<int64_t>(timestampKey));
      ++currentObject;
      return trigger;
    } else {
      return { TriggerType::No, true, activity };
    }
  };
}

} // namespace triggers

} // namespace o2::quality_control::postprocessing
