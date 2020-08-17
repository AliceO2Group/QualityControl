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
/// \file    Triggers.h
/// \author  Piotr Konopka
///

#ifndef QUALITYCONTROL_TRIGGERS_H
#define QUALITYCONTROL_TRIGGERS_H

#include <string>
#include <functional>
#include <iosfwd>

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
  UserOrControl, // reacts start and stop transitions (not an update trigger).
  INVALID
};

struct Trigger {

  /// \brief Constructor. Timestamp is generated from the time of construction.
  Trigger(TriggerType triggerType) : triggerType(triggerType), timestamp(msSinceEpoch()) {};
  /// \brief Constructor.
  Trigger(TriggerType triggerType, uint64_t timestamp) : triggerType(triggerType), timestamp(timestamp) {};

  operator bool() const { return triggerType == TriggerType::No || triggerType == TriggerType::INVALID; }
  friend std::ostream& operator<<(std::ostream& out, const Trigger &t);
  bool operator==(TriggerType other) const {
    return triggerType == other;
  }

  static uint64_t msSinceEpoch();

  TriggerType triggerType;
  uint64_t timestamp;
};

using TriggerFcn = std::function<Trigger()>;

namespace triggers
{

/// \brief Triggers when it detects a Start Of Run during its uptime (once per each)
TriggerFcn StartOfRun();
/// \brief Triggers when it detects an End Of Run during its uptime (once per each)
TriggerFcn EndOfRun();
/// \brief Triggers when it detects Stable Beams during its uptime (once per each)
TriggerFcn StartOfFill();
/// \brief Triggers when it detects an event dump during its uptime (once per each)
TriggerFcn EndOfFill();
/// \brief Triggers when a period of time passes
TriggerFcn Periodic(double seconds);
/// \brief Triggers when it detect a new object in QC repository with given name
TriggerFcn NewObject(std::string name);
/// \brief Triggers only first time it is executed
TriggerFcn Once();
/// \brief Triggers always
TriggerFcn Always();
/// \brief Triggers never
TriggerFcn Never();

} // namespace triggers

} // namespace o2::quality_control::postprocessing

#endif //QUALITYCONTROL_TRIGGERS_H
