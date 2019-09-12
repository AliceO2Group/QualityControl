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
#include "QualityControl/QcInfoLogger.h"

#include <Configuration/ConfigurationFactory.h>
using namespace o2::configuration;
using namespace o2::quality_control::core;

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

bool PostProcessingRunner::init()
{
  QcInfoLogger::GetInstance() << "Initializing PostProcessingRunner" << AliceO2::InfoLogger::InfoLogger::endm;

  // setup user's task
  QcInfoLogger::GetInstance() << "Creating a user task: " << mConfig.taskName << AliceO2::InfoLogger::InfoLogger::endm;
  PostProcessingFactory f;
  mTask.reset(f.create(mConfig));
  if (mTask) {
    QcInfoLogger::GetInstance() << " > Successfully created" << AliceO2::InfoLogger::InfoLogger::endm;

    mState = TaskState::Created;

    mInitTriggers = trigger_helpers::createTriggers(mConfig.initTriggers);
    return true;
  } else {
    QcInfoLogger::GetInstance() << "Failed to create the task" << AliceO2::InfoLogger::InfoLogger::endm;
    // todo :maybe exceptions instead of return values
    return false;
  }
}

bool PostProcessingRunner::run()
{
  QcInfoLogger::GetInstance() << "Running PostProcessingRunner" << AliceO2::InfoLogger::InfoLogger::endm;

  switch (mState) {
    case TaskState::Created: {
      if (Trigger trigger = trigger_helpers::tryTrigger(mInitTriggers); trigger) {
        QcInfoLogger::GetInstance() << "Initializing user task" << AliceO2::InfoLogger::InfoLogger::endm;

        mTask->initialize(trigger);

        mState = TaskState::Running; // maybe the task should monitor its state by itself?
        mUpdateTriggers = trigger_helpers::createTriggers(mConfig.updateTriggers);
        mStopTriggers = trigger_helpers::createTriggers(mConfig.stopTriggers);
      }
    }
    case TaskState::Running: {

      if (Trigger trigger = trigger_helpers::tryTrigger(mUpdateTriggers); trigger) {
        QcInfoLogger::GetInstance() << "Updating user task" << AliceO2::InfoLogger::InfoLogger::endm;
        mTask->update(trigger);
      }
      if (Trigger trigger = trigger_helpers::tryTrigger(mStopTriggers); trigger) {
        QcInfoLogger::GetInstance() << "Finalizing user task" << AliceO2::InfoLogger::InfoLogger::endm;
        mTask->finalize(trigger);
        mState = TaskState::Finished; // maybe the task should monitor its state by itself?
      }
    }
    case TaskState::Finished:{
      QcInfoLogger::GetInstance() << "User task finished, returning..." << AliceO2::InfoLogger::InfoLogger::endm;
      return false;
    }

    case TaskState::INVALID:{
      QcInfoLogger::GetInstance() << "User task state INVALID, returning..." << AliceO2::InfoLogger::InfoLogger::endm;
      return false;
    }
  }

  return true;
}

void PostProcessingRunner::stop()
{
}

void PostProcessingRunner::reset()
{
}

} // namespace o2::quality_control::postprocessing