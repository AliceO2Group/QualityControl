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

#include <functional>

namespace o2::quality_control::postprocessing
{

// todo: possible triggers:
//  INVALID,
//  StartOfRun,
//  EndOfRun,
//  StartOfProcessing,
//  EndOfProcessing,
//  StartOfFill,
//  EndOfFill, // BeamDump?
//  StartOfPeriod, // same as below?
//  EveryXMin,
//  EveryXHour,
//  HourOfDay, // only if really needed
//  DayOfMonth, // only if really needed
//  NewObject,
//  Manual, // click somewhere?
//  Once, // only triggers first time it is asked

enum Trigger {
  No = 0, // should cast to boolean false
  Once, // only triggers first time it is asked
  Always, // always triggers
  StartOfRun,
  EndOfRun,
  StartOfFill,
  EndOfFill,
  Periodic,
  NewObject,
  INVALID
};

using TriggerFcn = std::function<Trigger()>;


namespace triggers {

TriggerFcn StartOfRun();
TriggerFcn Once();

} // namespace triggers

} // namespace o2::quality_control::postprocessing


#endif //QUALITYCONTROL_TRIGGERS_H
