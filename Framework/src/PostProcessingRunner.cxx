// Copyright CERN and copyright holders of ALICE O2. This software is
// distributed under the terms of the GNU General Public License v3 (GPL
// Version 3), copied verbatim in the file "COPYING".
//
// See http://alice-o2.web.cern.ch/license for full licensing information.
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.

#include "QualityControl/PostProcessingRunner.h"

#include "QualityControl/PostProcessingFactory.h"
#include "QualityControl/PostProcessingInterface.h"
#include "QualityControl/TriggerHelpers.h"

#include <Configuration/ConfigurationFactory.h>

using namespace o2::configuration;

namespace o2::quality_control::postprocessing
{

PostProcessingRunner::PostProcessingRunner(std::string name, std::string configPath) //
  : mConfigFile(ConfigurationFactory::getConfiguration(configPath)),
    mConfig(name, *mConfigFile)
{
}

PostProcessingRunner::~PostProcessingRunner()
{
}

void PostProcessingRunner::init()
{
  // setup user's task
  PostProcessingFactory f;
  mTask.reset(f.create(mConfig));
  if (mTask) {
    mState = TaskState::Created;

    mInitTriggers = trigger_helpers::createTriggers(mConfig.initTriggers);

  } else {
    // todo: print error message and do sth
  }
}

void PostProcessingRunner::run()
{
  switch (mState) {
    case TaskState::Created: {
      if (Trigger trigger = trigger_helpers::tryTrigger(mInitTriggers); trigger) {
        mTask->initialize(trigger);

        mState = TaskState::Running; // maybe the task should monitor its state by itself?
        mUpdateTriggers = trigger_helpers::createTriggers(mConfig.updateTriggers);
        mStopTriggers = trigger_helpers::createTriggers(mConfig.stopTriggers);
      }
    }
    case TaskState::Running: {
      if (Trigger trigger = trigger_helpers::tryTrigger(mUpdateTriggers); trigger) {
        mTask->update(trigger);
      }
      if (Trigger trigger = trigger_helpers::tryTrigger(mStopTriggers); trigger) {
        mTask->finalize(trigger);
        mState = TaskState::Finished; // maybe the task should monitor its state by itself?
      }
    }
    case TaskState::Finished: {

    }
  }
}

void PostProcessingRunner::stop()
{
}

void PostProcessingRunner::reset()
{
}

} // namespace o2::quality_control::postprocessing