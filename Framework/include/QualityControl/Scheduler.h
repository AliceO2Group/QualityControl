//
// Created by pkonopka on 20/08/2019.
//

#ifndef QUALITYCONTROL_SCHEDULER_H
#define QUALITYCONTROL_SCHEDULER_H

namespace o2::quality_control::postprocessing
{

namespace triggers {

using triggerFcn = std::function<bool()>;

triggerFcn StartOfRun() {
  // it should trigger only once per start of run

  // FIXME: it has to be initialized before the SOR, to actually catch it. Is it a problem?
  bool runStarted = runOngoing();
  int runNumber = getRunNumber();

  return [runStarted, runNumber]() -> bool mutable {

    bool trigger = false;

    // trigger if:
    // - previously the run was not started
    trigger |= !runStarted && runOngoing();
    // - previously there was a different run number (we missed a stop and start sequence)
    trigger |= runOngoing() && runNumber != getRunNumber();

    runStarted = runOngoing();
    runNumber = getRunNumber();

    return trigger;
  }
}

// todo: more triggers like the one above.
//  possible triggers:
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
//  Manual // click somewhere?
//  Once, // only triggers first time it is asked

// here we might use the same function with different configuration if needed
// for example, a periodic trigger with time as minutes or hours.
triggerFcn TriggerFactory(std::string trigger) {
  if (trigger == "SOR" || trigger == "StartOfRun") {
    return StartOfRun();
  } else {
    throw std::runtime_error("unknown trigger: " + trigger);
  }
}

} // namespace triggers


class Scheduler {

  Scheduler() = default;
  ~Scheduler() = default;

  registerInitTrigger(triggers::triggerFcn t);
  registerUpdateTrigger(triggers::triggerFcn t);

  bool isReady();

  std::function<bool()> mInitTrigger;
  std::function<bool()> mUpdateTrigger;
};

} // namespace o2::quality_control::postprocessing

#endif //QUALITYCONTROL_SCHEDULER_H
