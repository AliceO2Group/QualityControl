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

#include <boost/property_tree/ptree.hpp>

using namespace o2::quality_control::core;
using namespace o2::quality_control::repository;

namespace o2::quality_control::postprocessing
{

PostProcessingRunner::PostProcessingRunner(std::string name) //
  : mName(name)
{
  ILOG_INST.setFacility("PostProcessing");
}

void PostProcessingRunner::init(const boost::property_tree::ptree& config)
{
  ILOG(Info, Support) << "Initializing PostProcessingRunner" << ENDM;

  mConfig = PostProcessingConfig(mName, config);

  // configuration of the database
  mDatabase = DatabaseFactory::create(config.get<std::string>("qc.config.database.implementation"));
  std::unordered_map<std::string, std::string> dbConfig;
  for (const auto& [key, value] : config.get_child("qc.config.database")) {
    dbConfig[key] = value.get_value<std::string>();
  }
  mDatabase->connect(dbConfig);
  ILOG(Info, Support) << "Database that is going to be used : " << ENDM;
  ILOG(Info, Support) << ">> Implementation : " << config.get<std::string>("qc.config.database.implementation") << ENDM;
  ILOG(Info, Support) << ">> Host : " << config.get<std::string>("qc.config.database.host") << ENDM;
  mServices.registerService<DatabaseInterface>(mDatabase.get());

  // setup user's task
  ILOG(Info, Support) << "Creating a user task '" << mConfig.taskName << "'" << ENDM;
  PostProcessingFactory f;
  mTask.reset(f.create(mConfig));
  if (mTask) {
    ILOG(Info, Support) << "The user task '" << mConfig.taskName << "' has been successfully created" << ENDM;

    mTaskState = TaskState::Created;
    mTask->setName(mConfig.taskName);
    mTask->configure(mConfig.taskName, config);
  } else {
    throw std::runtime_error("Failed to create the task '" + mConfig.taskName + "'");
  }
}

bool PostProcessingRunner::run()
{
  ILOG(Debug, Devel) << "Checking triggers of the task '" << mTask->getName() << "'" << ENDM;

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
    ILOG(Debug, Devel) << "The user task finished." << ENDM;
    return false;
  }
  if (mTaskState == TaskState::INVALID) {
    // That in principle shouldn't happen if we reach run()
    throw std::runtime_error("The user task has INVALID state");
  }

  return true;
}

void PostProcessingRunner::runOverTimestamps(const std::vector<uint64_t>& timestamps)
{
  if (timestamps.size() < 2) {
    throw std::runtime_error(
      "At least two timestamps should be specified, " + std::to_string(timestamps.size()) +
      " given. One is for the initialization, zero or more for update, one for finalization");
  }

  ILOG(Info, Support) << "Running the task '" << mTask->getName() << "' over " << timestamps.size() << " timestamps." << ENDM;

  doInitialize({ TriggerType::UserOrControl, timestamps.front() });
  for (size_t i = 1; i < timestamps.size() - 1; i++) {
    doUpdate({ TriggerType::UserOrControl, timestamps[i] });
  }
  doFinalize({ TriggerType::UserOrControl, timestamps.back() });
}

void PostProcessingRunner::start()
{
  if (mTaskState == TaskState::Created || mTaskState == TaskState::Finished) {
    mInitTriggers = trigger_helpers::createTriggers(mConfig.initTriggers);
    if (trigger_helpers::hasUserOrControlTrigger(mConfig.initTriggers)) {
      doInitialize({ TriggerType::UserOrControl });
    }
  } else if (mTaskState == TaskState::Running) {
    ILOG(Debug, Devel) << "Requested start, but the user task is already running - doing nothing." << ENDM;
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
      doFinalize({ TriggerType::UserOrControl });
    }
  } else if (mTaskState == TaskState::Finished) {
    ILOG(Debug, Devel) << "Requested stop, but the user task is already finalized - doing nothing." << ENDM;
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
  ILOG(Info, Support) << "Initializing the user task due to trigger '" << trigger << "'" << ENDM;

  mTask->initialize(trigger, mServices);
  mTaskState = TaskState::Running;

  // We create the triggers just after task init (and not any sooner), so the timer triggers work as expected.
  mUpdateTriggers = trigger_helpers::createTriggers(mConfig.updateTriggers);
  mStopTriggers = trigger_helpers::createTriggers(mConfig.stopTriggers);
}

void PostProcessingRunner::doUpdate(Trigger trigger)
{
  ILOG(Info, Support) << "Updating the user task due to trigger '" << trigger << "'" << ENDM;
  mTask->update(trigger, mServices);
}

void PostProcessingRunner::doFinalize(Trigger trigger)
{
  ILOG(Info, Support) << "Finalizing the user task due to trigger '" << trigger << "'" << ENDM;
  mTask->finalize(trigger, mServices);
  mTaskState = TaskState::Finished;
}

} // namespace o2::quality_control::postprocessing
