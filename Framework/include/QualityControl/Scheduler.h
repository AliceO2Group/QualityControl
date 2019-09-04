//
// Created by pkonopka on 20/08/2019.
//

#ifndef QUALITYCONTROL_SCHEDULER_H
#define QUALITYCONTROL_SCHEDULER_H

#include "QualityControl/Triggers.h"

namespace o2::quality_control::postprocessing
{

class Scheduler {

  Scheduler() = default;
  ~Scheduler() = default;

  registerInitTrigger(triggers::triggerFcn t);
  registerUpdateTrigger(triggers::triggerFcn t);
  registerFinalizeTrigger(triggers::triggerFcn t);

  bool isReady();

  std::function<bool()> mInitTrigger;
  std::function<bool()> mUpdateTrigger;
  std::function<bool()> mFinalizeTrigger;
};

} // namespace o2::quality_control::postprocessing

#endif //QUALITYCONTROL_SCHEDULER_H
