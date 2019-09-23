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

namespace o2::quality_control::postprocessing
{

namespace triggers
{

TriggerFcn StartOfRun()
{
  // it should trigger only once per start of run

  // FIXME: it has to be initialized before the SOR, to actually catch it. Is it a problem?
  bool runStarted = false; // runOngoing();
  int runNumber = false;   // getRunNumber();

  return [runStarted, runNumber]() mutable -> Trigger {
    bool trigger = false;

    // trigger if:
    // - previously the run was not started
    //    trigger |= !runStarted && runOngoing();
    // - previously there was a different run number (we missed a stop and start sequence)
    //    trigger |= runOngoing() && runNumber != getRunNumber();

    //    runStarted = runOngoing();
    //    runNumber = getRunNumber();

    return trigger ? Trigger::StartOfRun : Trigger::No;
  };
}

TriggerFcn Once()
{
  return [hasTriggered = false]() mutable -> Trigger {
    if (hasTriggered) {
      return Trigger::No;
    } else {
      hasTriggered = true;
      return Trigger::Once;
    }
  };
}

TriggerFcn Always()
{
  return []() mutable -> Trigger {
    return Trigger::Always;
  };
}

} // namespace triggers

} // namespace o2::quality_control::postprocessing
