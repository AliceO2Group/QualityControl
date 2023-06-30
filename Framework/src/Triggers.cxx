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
#include "QualityControl/ActivityHelpers.h"
#include "QualityControl/CcdbDatabase.h"
#include "QualityControl/ObjectMetadataKeys.h"

#include <CCDB/CcdbApi.h>
#include <Common/Timer.h>
#include <chrono>
#include <ostream>
#include <tuple>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>

using namespace std::chrono;
using namespace o2::quality_control::core;
using namespace o2::quality_control::repository;

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
      return { TriggerType::No, true, activity, Trigger::msSinceEpoch(), "once" };
    } else {
      hasTriggered = true;
      return { TriggerType::Once, true, activity, Trigger::msSinceEpoch(), "once" };
    }
  };
}

TriggerFcn Always(const Activity& activity)
{
  return [activity]() mutable -> Trigger {
    return { TriggerType::Always, false, activity, Trigger::msSinceEpoch(), "always" };
  };
}

TriggerFcn Never(const Activity& activity)
{
  return [activity]() mutable -> Trigger {
    return { TriggerType::No, true, activity, Trigger::msSinceEpoch(), "never" };
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

TriggerFcn Periodic(double seconds, const Activity& activity, std::string config)
{
  AliceO2::Common::Timer timer;
  timer.reset(static_cast<int>(seconds * 1000000));
  auto resultActivity = activity;
  resultActivity.mValidity = gInvalidValidityInterval;

  return [timer, resultActivity, config]() mutable -> Trigger {
    if (timer.isTimeout()) {
      // We calculate the exact time when timer has passed
      uint64_t timestamp = Trigger::msSinceEpoch() + static_cast<int>(timer.getRemainingTime() * 1000);
      // increment until it is cleared (in case that more than one cycle has passed)
      // let's hope there is no bug, which would make us stay in that loop forever
      while (timer.isTimeout()) {
        timer.increment();
      }
      resultActivity.mValidity.update(timestamp);
      return { TriggerType::Periodic, false, resultActivity, timestamp, config };
    } else {
      return { TriggerType::No, false, resultActivity, Trigger::msSinceEpoch(), config };
    }
  };
}

TriggerFcn NewObject(const std::string& databaseUrl, const std::string& databaseType, const std::string& objectPath, const Activity& activity, const std::string& config)
{
  // Key names in the header map.
  constexpr auto timestampKey = metadata_keys::validFrom;
  auto fullObjectPath = (databaseType == "qcdb" ? activity.mProvenance + "/" : "") + objectPath;
  auto metadata = databaseType == "qcdb" ? activity_helpers::asDatabaseMetadata(activity, false) : std::map<std::string, std::string>();
  auto objectActivity = activity;

  ILOG(Debug, Support) << "Initializing newObject trigger for the object '" << fullObjectPath << "' and Activity '" << activity << "'" << ENDM;
  // We support only CCDB here.
  auto db = std::make_shared<repository::CcdbDatabase>();
  db->connect(databaseUrl, "", "", "");

  // Returns "Valid-From" of an object if there is a new one, otherwise 0.
  auto newObjectValidity = [db, fullObjectPath, metadata, databaseUrl, activity, lastModified = validity_time_t{ 0 }]() mutable -> ValidityInterval {
    const auto listing = db->getListingAsPtree(fullObjectPath, metadata, true);
    if (listing.count("objects") == 0) {
      ILOG(Warning, Support) << "Could not get a valid listing from db '" << databaseUrl << "' for object '" << fullObjectPath << "'" << ENDM;
      return gInvalidValidityInterval;
    }
    const auto& objects = listing.get_child("objects");
    if (objects.empty()) {
      // We don't make a fuss over it, because we might be just waiting for the first version of such object.
      // It should not happen often though, so having a warning makes sense.
      ILOG(Warning, Support) << "Could not find the file '" << fullObjectPath << "' in the db '"
                             << databaseUrl << "' for given Activity settings (" << activity << "). Zeroes and empty strings are treated as wildcards." << ENDM;
      return gInvalidValidityInterval;
    } else if (objects.size() > 1) {
      ILOG(Warning, Support) << "Expected just one metadata entry for object '" << fullObjectPath << "'. Trying to continue by using the first." << ENDM;
    }

    const auto& object = objects.front().second;
    validity_time_t newLastModified = object.get<uint64_t>(metadata_keys::lastModified, 0);
    if (newLastModified > lastModified) {
      lastModified = newLastModified;
      return { object.get<uint64_t>(metadata_keys::validFrom, 0), object.get<uint64_t>(metadata_keys::validUntil) };
    }
    return gInvalidValidityInterval;
  };
  // we execute it once before in order to know about the latest existing object.
  newObjectValidity();

  return [objectActivity, config, newObjectValidity]() mutable -> Trigger {
    if (auto validity = newObjectValidity(); validity.isValid()) {
      objectActivity.mValidity = validity;
      auto timestamp = activity_helpers::isLegacyValidity(validity) ? validity.getMin() : (validity.getMax() - 1);
      return { TriggerType::NewObject, false, objectActivity, timestamp, config };
    }
    objectActivity.mValidity = gInvalidValidityInterval;
    return { TriggerType::No, false, objectActivity, Trigger::msSinceEpoch(), config };
  };
}

TriggerFcn ForEachObject(const std::string& databaseUrl, const std::string& databaseType, const std::string& objectPath, const Activity& activity, const std::string& config)
{
  // Key names in the header map.
  constexpr auto timestampKey = metadata_keys::validFrom;
  auto fullObjectPath = (databaseType == "qcdb" ? activity.mProvenance + "/" : "") + objectPath;

  // We support only CCDB here.
  auto db = std::make_shared<repository::CcdbDatabase>();
  db->connect(databaseUrl, "", "", "");

  auto objects = db->getListingAsPtree(fullObjectPath).get_child("objects");
  ILOG(Info, Support) << "Got " << objects.size() << " objects for the path '" << fullObjectPath << "'" << ENDM;
  auto filteredObjects = std::make_shared<std::vector<boost::property_tree::ptree>>();
  const auto filter = databaseType == "qcdb" ? activity : Activity();

  ILOG(Debug, Devel) << "Filter activity: " << activity << ENDM;

  // As for today, we receive objects in the order of the newest to the oldest.
  // We prefer the other order here.
  for (auto rit = objects.rbegin(); rit != objects.rend(); ++rit) {
    auto objectActivity = activity_helpers::asActivity(rit->second, activity.mProvenance);
    ILOG(Debug, Trace) << "Matching the filter with object's activity: " << objectActivity << ENDM;
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

  return [filteredObjects, activity, currentObject = filteredObjects->begin(), config]() mutable -> Trigger {
    if (currentObject != filteredObjects->end()) {
      auto currentActivity = activity_helpers::asActivity(*currentObject, activity.mProvenance);
      bool last = currentObject + 1 == filteredObjects->end();
      Trigger trigger(TriggerType::ForEachObject, last, currentActivity, currentObject->get<int64_t>(timestampKey));
      ++currentObject;
      return trigger;
    } else {
      return { TriggerType::No, true, activity, Trigger::msSinceEpoch(), config };
    }
  };
}

TriggerFcn ForEachLatest(const std::string& databaseUrl, const std::string& databaseType, const std::string& objectPath, const Activity& activity, const std::string& config)
{
  // Key names in the header map.
  constexpr auto timestampKey = metadata_keys::created;
  auto fullObjectPath = (databaseType == "qcdb" ? activity.mProvenance + "/" : "") + objectPath;

  // We support only CCDB here.
  auto db = std::make_shared<repository::CcdbDatabase>();
  db->connect(databaseUrl, "", "", "");

  auto objects = db->getListingAsPtree(fullObjectPath).get_child("objects");
  ILOG(Info, Support) << "Got " << objects.size() << " objects for the path '" << fullObjectPath << "'" << ENDM;
  auto filteredObjects = std::make_shared<std::vector<std::pair<Activity, boost::property_tree::ptree>>>();
  const auto filter = databaseType == "qcdb" ? activity : Activity();

  ILOG(Debug, Devel) << "Filter activity: " << activity << ENDM;

  // As for today, we receive objects in the order of the newest to the oldest.
  // The inverse order is more likely to follow what we want (ascending by period/pass/run),
  // thus sorting may take less time.
  for (auto rit = objects.rbegin(); rit != objects.rend(); ++rit) {
    auto objectActivity = activity_helpers::asActivity(rit->second, activity.mProvenance);
    ILOG(Debug, Trace) << "Matching the filter with object's activity: " << objectActivity << ENDM;
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

  // Since we select concrete objects per each combination of run/pass/period,
  // we sort the entries in the ascending order by period, pass and run.
  std::sort(filteredObjects->begin(), filteredObjects->end(),
            [](const std::pair<Activity, boost::property_tree::ptree>& a, const std::pair<Activity, boost::property_tree::ptree>& b) {
              return std::forward_as_tuple(a.second.get<std::string>(metadata_keys::periodName, ""),
                                           a.second.get<std::string>(metadata_keys::passName, ""),
                                           a.second.get<int64_t>(metadata_keys::runNumber, 0)) <
                     std::forward_as_tuple(b.second.get<std::string>(metadata_keys::periodName, ""),
                                           b.second.get<std::string>(metadata_keys::passName, ""),
                                           b.second.get<int64_t>(metadata_keys::runNumber, 0));
            });

  return [filteredObjects, activity, currentObject = filteredObjects->begin(), config]() mutable -> Trigger {
    if (currentObject != filteredObjects->end()) {
      const auto& currentActivity = currentObject->first;
      const auto& currentPtree = currentObject->second;
      bool last = currentObject + 1 == filteredObjects->end();
      Trigger trigger(TriggerType::ForEachLatest, last, currentActivity, currentPtree.get<int64_t>(timestampKey), config);
      ++currentObject;
      return trigger;
    } else {
      return { TriggerType::No, true, activity, Trigger::msSinceEpoch(), config };
    }
  };
}

} // namespace triggers

} // namespace o2::quality_control::postprocessing
