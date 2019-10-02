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
  // fixme: uncomment after a bugfix in Configuration is merged and released
  // mConfigFile->setPrefix(""); // protect from having the prefix changed by PostProcessingConfig
}

PostProcessingRunner::~PostProcessingRunner()
{
}

bool PostProcessingRunner::init()
{
  LOG(INFO) << "Initializing PostProcessingRunner";

  try {
    // configuration of the database
    mDatabase = DatabaseFactory::create(mConfigFile->get<std::string>("qc.config.database.implementation"));
    mDatabase->connect(mConfigFile->getRecursiveMap("qc.config.database"));
    LOG(INFO) << "Database that is going to be used : ";
    LOG(INFO) << ">> Implementation : " << mConfigFile->get<std::string>("qc.config.database.implementation");
    LOG(INFO) << ">> Host : " << mConfigFile->get<std::string>("qc.config.database.host");
    mServices.registerService<DatabaseInterface>(mDatabase.get());
  } catch (
    std::string const& e) { // we have to catch here to print the exception because the device will make it disappear todo: is it still the case?
    LOG(ERROR) << "exception : " << e;
    throw;
  } catch (...) {
    std::string diagnostic = boost::current_exception_diagnostic_information();
    LOG(ERROR) << "Unexpected exception, diagnostic information follows:\n"
               << diagnostic;
    throw;
  }

  // setup user's task
  LOG(INFO) << "Creating a user task '" << mConfig.taskName << "'";
  PostProcessingFactory f;
  mTask.reset(f.create(mConfig));
  if (mTask) {
    LOG(INFO) << "The user task '" << mConfig.taskName << "' successfully created";

    mState = TaskState::Created;
    mTask->setName(mConfig.taskName);
    mTask->configure(mConfig.taskName, *mConfigFile);

    mInitTriggers = trigger_helpers::createTriggers(mConfig.initTriggers);
    return true;
  } else {
    LOG(INFO) << "Failed to create the task '" << mConfig.taskName << "'";
    // todo :maybe exceptions instead of return values
    return false;
  }
}

bool PostProcessingRunner::run()
{
  LOG(INFO) << "Running PostProcessingRunner";

  if (mState == TaskState::Created) {
    if (Trigger trigger = trigger_helpers::tryTrigger(mInitTriggers)) {
      LOG(INFO) << "Initializing user task";

      mTask->initialize(trigger, mServices);

      mState = TaskState::Running; // maybe the task should monitor its state by itself?
      mUpdateTriggers = trigger_helpers::createTriggers(mConfig.updateTriggers);
      mStopTriggers = trigger_helpers::createTriggers(mConfig.stopTriggers);
    }
  }
  if (mState == TaskState::Running) {

    if (Trigger trigger = trigger_helpers::tryTrigger(mUpdateTriggers)) {
      LOG(INFO) << "Updating user task";
      mTask->update(trigger, mServices);
    }
    if (Trigger trigger = trigger_helpers::tryTrigger(mStopTriggers)) {
      LOG(INFO) << "Finalizing user task";
      mTask->finalize(trigger, mServices);
      mState = TaskState::Finished; // maybe the task should monitor its state by itself?
    }
  }
  if (mState == TaskState::Finished) {
    LOG(INFO) << "User task finished, returning...";
    return false;
  }
  if (mState == TaskState::INVALID) {
    // todo maybe exception?
    LOG(INFO) << "User task state INVALID, returning...";
    return false;
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