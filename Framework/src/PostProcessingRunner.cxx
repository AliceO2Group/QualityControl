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
  : mConfigFile(ConfigurationFactory::getConfiguration(configPath)),
    mConfig(name, *mConfigFile)
{
  mConfigFile->setPrefix(""); // protect from having the prefix changed by PostProcessingConfig
}

PostProcessingRunner::~PostProcessingRunner()
{
}

void PostProcessingRunner::init()
{
  ILOG(Info) << "Initializing PostProcessingRunner" << ENDM;

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

    mState = TaskState::Created;
    mTask->setName(mConfig.taskName);
    mTask->configure(mConfig.taskName, *mConfigFile);

    mInitTriggers = trigger_helpers::createTriggers(mConfig.initTriggers);
  } else {
    throw std::runtime_error("Failed to create the task '" + mConfig.taskName + "'");
  }
}

bool PostProcessingRunner::run()
{
  ILOG(Info) << "Checking triggers of the task '" << mTask->getName() << "'" << ENDM;

  if (mState == TaskState::Created) {
    if (Trigger trigger = trigger_helpers::tryTrigger(mInitTriggers)) {
      ILOG(Info) << "Initializing user task" << ENDM;

      mTask->initialize(trigger, mServices);

      mState = TaskState::Running; // maybe the task should monitor its state by itself?
      mUpdateTriggers = trigger_helpers::createTriggers(mConfig.updateTriggers);
      mStopTriggers = trigger_helpers::createTriggers(mConfig.stopTriggers);
    }
  }
  if (mState == TaskState::Running) {

    if (Trigger trigger = trigger_helpers::tryTrigger(mUpdateTriggers)) {
      ILOG(Info) << "Updating user task" << ENDM;
      mTask->update(trigger, mServices);
    }
    if (Trigger trigger = trigger_helpers::tryTrigger(mStopTriggers)) {
      ILOG(Info) << "Finalizing user task" << ENDM;
      mTask->finalize(trigger, mServices);
      mState = TaskState::Finished; // maybe the task should monitor its state by itself?
    }
  }
  if (mState == TaskState::Finished) {
    ILOG(Info) << "User task finished, returning..." << ENDM;
    return false;
  }
  if (mState == TaskState::INVALID) {
    // That in principle shouldn't happen, as we don't have a code path to reach INVALID.
    ILOG(Error) << "User task state INVALID, returning..." << ENDM;
    throw std::runtime_error("User task state INVALID");
  }

  return true;
}

void PostProcessingRunner::stop()
{
  if (mState == TaskState::Created || mState == TaskState::Running) {
    if (Trigger trigger = trigger_helpers::tryTrigger(mStopTriggers)) {
      ILOG(Info) << "Finalizing user task";
      mTask->finalize(trigger, mServices);
      mState = TaskState::Finished; // maybe the task should monitor its state by itself?
    }
  }
}

void PostProcessingRunner::reset()
{
  // todo: see where is "reset" in the state machine
  // stop();
  // init();
}

} // namespace o2::quality_control::postprocessing