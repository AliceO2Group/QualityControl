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
/// \file    Triggers.h
/// \author  Piotr Konopka
///

#ifndef QUALITYCONTROL_TRIGGERS_H
#define QUALITYCONTROL_TRIGGERS_H

#include <string>
#include <functional>
#include <iosfwd>
#include <utility>
#include "QualityControl/Activity.h"

namespace o2::quality_control::postprocessing
{

// todo: implement the rest
/// \brief Possible triggers
enum TriggerType {
  No = 0, // casts to boolean false
  Once,   // triggers only first time it is asked
  Always, // triggers always
  StartOfRun,
  EndOfRun,
  StartOfFill,
  EndOfFill,
  Periodic,
  NewObject,
  ForEachObject,
  UserOrControl, // reacts start and stop transitions (not an update trigger).
  INVALID
};

struct Trigger {

  /// \brief Constructor. Timestamp is generated from the time of construction.
  Trigger(TriggerType triggerType, bool last = false, core::Activity activity = {}) : triggerType(triggerType), last(last), activity(std::move(activity)), timestamp(msSinceEpoch()){};
  /// \brief Constructor.
  Trigger(TriggerType triggerType, bool last, core::Activity activity, uint64_t timestamp) : triggerType(triggerType), last(last), activity(std::move(activity)), timestamp(timestamp){};
  /// \brief Constructor.
  Trigger(TriggerType triggerType, bool last, uint64_t timestamp) : triggerType(triggerType), last(last), activity(), timestamp(timestamp){};

  operator bool() const { return triggerType != TriggerType::No && triggerType != TriggerType::INVALID; }
  friend std::ostream& operator<<(std::ostream& out, const Trigger& t);
  bool operator==(TriggerType other) const
  {
    return triggerType == other;
  }

  static uint64_t msSinceEpoch();

  TriggerType triggerType;
  bool last;
  core::Activity activity;
  uint64_t timestamp;
};

using TriggerFcn = std::function<Trigger()>;

namespace triggers
{

/// \brief Triggers when it detects a Start Of Run during its uptime (once per each)
TriggerFcn StartOfRun(const core::Activity& = {});
/// \brief Triggers when it detects an End Of Run during its uptime (once per each)
TriggerFcn EndOfRun(const core::Activity& = {});
/// \brief Triggers when it detects Stable Beams during its uptime (once per each)
TriggerFcn StartOfFill(const core::Activity& = {});
/// \brief Triggers when it detects an event dump during its uptime (once per each)
TriggerFcn EndOfFill(const core::Activity& = {});
/// \brief Triggers when a period of time passes
TriggerFcn Periodic(double seconds, const core::Activity& = {});
/// \brief Triggers when it detect a new object in QC repository with given name
TriggerFcn NewObject(std::string databaseUrl, std::string objectPath, const core::Activity& = {});
/// \brief Triggers for each object version in the path which match the activity. It retrieves the available list only once!
TriggerFcn ForEachObject(std::string databaseUrl, std::string objectPath, const core::Activity& = {});
/// \brief Triggers only first time it is executed
TriggerFcn Once(const core::Activity& = {});
/// \brief Triggers always
TriggerFcn Always(const core::Activity& = {});
/// \brief Triggers never
TriggerFcn Never(const core::Activity& = {});

} // namespace triggers

} // namespace o2::quality_control::postprocessing

#endif //QUALITYCONTROL_TRIGGERS_H
