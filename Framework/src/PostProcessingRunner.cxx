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
#include "QualityControl/TriggerHelpers.h"
#include "QualityControl/DatabaseFactory.h"
#include "QualityControl/QcInfoLogger.h"

#include <Configuration/ConfigurationFactory.h>

using namespace o2::configuration;
using namespace o2::quality_control::core;
using namespace o2::quality_control::repository;

namespace o2::quality_control::postprocessing
{

PostProcessingRunner::PostProcessingRunner(std::string name, std::string configPath) //
  : mName(name), mConfigPath(configPath)
{
}

void PostProcessingRunner::init()
{
  ILOG(Info) << "Initializing PostProcessingRunner" << ENDM;

  mConfigFile = ConfigurationFactory::getConfiguration(mConfigPath);
  mConfig = PostProcessingConfig(mName, *mConfigFile);
  mConfigFile->setPrefix(""); // protect from having the prefix changed by PostProcessingConfig

  // configuration of the database
  mDatabase = DatabaseFactory::create(mConfigFile->get<std::string>("qc.config.database.implementation"));
  mDatabase->connect(mConfigFile->getRecursiveMap("qc.config.database"));
  ILOG(Info) << "Database that is going to be used : " << ENDM;
  ILOG(Info) << ">> Implementation : " << mConfigFile->get<std::string>("qc.config.database.implementation") << ENDM;
  ILOG(Info) << ">> Host : " << mConfigFile->get<std::string>("qc.config.database.host") << ENDM;
  mServices.registerService<DatabaseInterface>(mDatabase.get());

  // setup user's task
  ILOG(Info) << "Creating a user task '" << mConfig.taskName << "'" << ENDM;
  PostProcessingFactory f;
  mTask.reset(f.create(mConfig));
  if (mTask) {
    ILOG(Info) << "The user task '" << mConfig.taskName << "' successfully created" << ENDM;

    mTaskState = TaskState::Created;
    mTask->setName(mConfig.taskName);
    mTask->configure(mConfig.taskName, *mConfigFile);
  } else {
    throw std::runtime_error("Failed to create the task '" + mConfig.taskName + "'");
  }
}

bool PostProcessingRunner::run()
{
  ILOG(Info) << "Checking triggers of the task '" << mTask->getName() << "'" << ENDM;

  if (mTaskState == TaskState::Created) {
    if (Trigger trigger = trigger_helpers::tryTrigger(mInitTriggers)) {
      doInitialize(trigger);
    }
  }
  if (mTaskState == TaskState::Running) {
    if (Trigger trigger = trigger_helpers::tryTrigger(mUpdateTriggers)) {
      doUpdate(trigger);
    }
    if (Trigger trigger = trigger_helpers::tryTrigger(mStopTriggers)) {
      doFinalize(trigger);
    }
  }
  if (mTaskState == TaskState::Finished) {
    ILOG(Info) << "The user task finished." << ENDM;
    return false;
  }
  if (mTaskState == TaskState::INVALID) {
    // That in principle shouldn't happen if we reach run()
    throw std::runtime_error("The user task has INVALID state");
  }

  return true;
}

void PostProcessingRunner::start()
{
  if (mTaskState == TaskState::Created || mTaskState == TaskState::Finished) {
    mInitTriggers = trigger_helpers::createTriggers(mConfig.initTriggers);
    if (trigger_helpers::hasUserOrControlTrigger(mConfig.initTriggers)) {
      doInitialize(Trigger::UserOrControl);
    }
  } else if (mTaskState == TaskState::Running) {
    ILOG(Info) << "Requested start, but the user task is already running - doing nothing." << ENDM;
  } else if (mTaskState == TaskState::INVALID) {
    throw std::runtime_error("The user task has INVALID state");
  } else {
    throw std::runtime_error("Unknown task state");
  }
}

void PostProcessingRunner::stop()
{
  if (mTaskState == TaskState::Created || mTaskState == TaskState::Running) {
    if (trigger_helpers::hasUserOrControlTrigger(mConfig.stopTriggers)) {
      doFinalize(Trigger::UserOrControl);
    }
  } else if (mTaskState == TaskState::Finished) {
    ILOG(Info) << "Requested stop, but the user task is already finalized - doing nothing." << ENDM;
  } else if (mTaskState == TaskState::INVALID) {
    throw std::runtime_error("The user task has INVALID state");
  } else {
    throw std::runtime_error("Unknown task state");
  }
}

void PostProcessingRunner::reset()
{
  mTaskState = TaskState::INVALID;

  mTask.reset();
  mDatabase.reset();
  mServices = framework::ServiceRegistry();

  mInitTriggers.clear();
  mUpdateTriggers.clear();
  mStopTriggers.clear();
}

void PostProcessingRunner::doInitialize(Trigger trigger)
{
  ILOG(Info) << "Initializing the user task due to trigger '" << trigger << "'" << ENDM;

  mTask->initialize(trigger, mServices);
  mTaskState = TaskState::Running;

  // We create the triggers just after task init (and not any sooner), so the timer triggers work as expected.
  mUpdateTriggers = trigger_helpers::createTriggers(mConfig.updateTriggers);
  mStopTriggers = trigger_helpers::createTriggers(mConfig.stopTriggers);
}

void PostProcessingRunner::doUpdate(Trigger trigger)
{
  ILOG(Info) << "Updating the user task due to trigger '" << trigger << "'" << ENDM;
  mTask->update(trigger, mServices);
}

void PostProcessingRunner::doFinalize(Trigger trigger)
{
  ILOG(Info) << "Finalizing the user task due to trigger '" << trigger << "'" << ENDM;
  mTask->finalize(UserOrControl, mServices);
  mTaskState = TaskState::Finished;
}

} // namespace o2::quality_control::postprocessing
