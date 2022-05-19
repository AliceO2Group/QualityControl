// Copyright 2019-2020 CERN and copyright holders of ALICE O2.
// See https://alice-o2.web.cern.ch/copyright for details of the copyright holders.
// All rights not expressly granted are reserved.
//
// This software is distributed under the terms of the GNU General Public
// License v3 (GPL Version 3), copied verbatim in the file "COPYING".
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.

#include "QualityControl/PostProcessingRunner.h"

#include "QualityControl/PostProcessingFactory.h"
#include "QualityControl/PostProcessingConfig.h"
#include "QualityControl/TriggerHelpers.h"
#include "QualityControl/DatabaseFactory.h"
#include "QualityControl/QcInfoLogger.h"
#include "QualityControl/CommonSpec.h"
#include "QualityControl/InfrastructureSpecReader.h"
#include "QualityControl/Activity.h"
#include "QualityControl/RootClassFactory.h"
#include "QualityControl/runnerUtils.h"

#include <boost/property_tree/ptree.hpp>
#include <utility>
#include <Framework/DataAllocator.h>
#include <CommonUtils/ConfigurableParam.h>

using namespace o2::quality_control::core;
using namespace o2::quality_control::repository;

namespace o2::quality_control::postprocessing
{

constexpr long objectValidity = 1000l * 60 * 60 * 24 * 365 * 10;

PostProcessingRunner::PostProcessingRunner(std::string name) //
  : mName(std::move(name))
{
}

void PostProcessingRunner::setPublicationCallback(MOCPublicationCallback callback)
{
  mPublicationCallback = std::move(callback);
}

void PostProcessingRunner::init(const boost::property_tree::ptree& config)
{
  auto specs = InfrastructureSpecReader::readInfrastructureSpec(config);
  auto ppTaskSpec = std::find_if(specs.postProcessingTasks.begin(),
                                 specs.postProcessingTasks.end(),
                                 [name = mName](const auto& spec) {
                                   return spec.taskName == name;
                                 });
  if (ppTaskSpec == specs.postProcessingTasks.end()) {
    throw std::runtime_error("Could not find the configuration of the post-processing task '" + mName + "'");
  }

  init(PostProcessingRunner::extractConfig(specs.common, *ppTaskSpec), PostProcessingConfig{ mName, config });
}

void PostProcessingRunner::init(const PostProcessingRunnerConfig& runnerConfig, const PostProcessingConfig& taskConfig)
{
  mRunnerConfig = runnerConfig;
  mTaskConfig = taskConfig;

  QcInfoLogger::init("post/" + mName, runnerConfig.infologgerFilterDiscardDebug, runnerConfig.infologgerDiscardLevel);
  ILOG(Info, Support) << "Initializing PostProcessingRunner" << ENDM;

  root_class_factory::loadLibrary(mTaskConfig.moduleName);
  if (!mRunnerConfig.configKeyValues.empty()) {
    conf::ConfigurableParam::updateFromString(mRunnerConfig.configKeyValues);
  }

  // configuration of the database
  mDatabase = DatabaseFactory::create(mRunnerConfig.database.at("implementation"));
  mDatabase->connect(mRunnerConfig.database);
  ILOG(Info, Support) << "Database that is going to be used : " << ENDM;
  ILOG(Info, Support) << ">> Implementation : " << mRunnerConfig.database.at("implementation") << ENDM;
  ILOG(Info, Support) << ">> Host : " << mRunnerConfig.database.at("host") << ENDM;

  mObjectManager = std::make_shared<ObjectsManager>(mTaskConfig.taskName, mTaskConfig.className, mTaskConfig.detectorName, mRunnerConfig.consulUrl);
  mServices.registerService<DatabaseInterface>(mDatabase.get());
  if (mPublicationCallback == nullptr) {
    mPublicationCallback = publishToRepository(*mDatabase);
  }

  // setup user's task
  ILOG(Info, Support) << "Creating a user task '" << mTaskConfig.taskName << "'" << ENDM;
  PostProcessingFactory f;
  mTask.reset(f.create(mTaskConfig));
  mTask->setObjectsManager(mObjectManager);
  if (mTask) {
    ILOG(Info, Support) << "The user task '" << mTaskConfig.taskName << "' has been successfully created" << ENDM;

    mTaskState = TaskState::Created;
    mTask->setName(mTaskConfig.taskName);
    mTask->configure(mTaskConfig.taskName, mRunnerConfig.configTree);
  } else {
    throw std::runtime_error("Failed to create the task '" + mTaskConfig.taskName + "'");
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
    if (mUpdateTriggers.empty()) {
      doFinalize({ TriggerType::UserOrControl, true, mTaskConfig.activity });
    } else if (Trigger trigger = trigger_helpers::tryTrigger(mStopTriggers)) {
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

  doInitialize({ TriggerType::UserOrControl, false, mTaskConfig.activity, timestamps.front() });
  for (size_t i = 1; i < timestamps.size() - 1; i++) {
    doUpdate({ TriggerType::UserOrControl, i == timestamps.size() - 2, mTaskConfig.activity, timestamps[i] });
  }
  doFinalize({ TriggerType::UserOrControl, false, mTaskConfig.activity, timestamps.back() });
}

void PostProcessingRunner::start(const framework::ServiceRegistry* dplServices)
{
  if (dplServices != nullptr) {
    mTaskConfig.activity.mId = computeRunNumber(*dplServices, mTaskConfig.activity.mId);
    mTaskConfig.activity.mType = computeRunType(*dplServices, mTaskConfig.activity.mType);
    mTaskConfig.activity.mPeriodName = computePeriodName(*dplServices, mTaskConfig.activity.mPeriodName);
    mTaskConfig.activity.mPassName = computePassName(mTaskConfig.activity.mPassName);
    mTaskConfig.activity.mProvenance = computeProvenance(mTaskConfig.activity.mProvenance);
  }

  if (mTaskState == TaskState::Created || mTaskState == TaskState::Finished) {
    mInitTriggers = trigger_helpers::createTriggers(mTaskConfig.initTriggers, mTaskConfig);
    if (trigger_helpers::hasUserOrControlTrigger(mTaskConfig.initTriggers)) {
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
    if (trigger_helpers::hasUserOrControlTrigger(mTaskConfig.stopTriggers)) {
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
  mObjectManager.reset();

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
  mUpdateTriggers = trigger_helpers::createTriggers(mTaskConfig.updateTriggers, mTaskConfig);
  mStopTriggers = trigger_helpers::createTriggers(mTaskConfig.stopTriggers, mTaskConfig);
}

void PostProcessingRunner::doUpdate(Trigger trigger)
{
  ILOG(Info, Support) << "Updating the user task due to trigger '" << trigger << "'" << ENDM;
  mTask->update(trigger, mServices);
  mPublicationCallback(mObjectManager->getNonOwningArray(), trigger.timestamp, trigger.timestamp + objectValidity);
}

void PostProcessingRunner::doFinalize(Trigger trigger)
{
  ILOG(Info, Support) << "Finalizing the user task due to trigger '" << trigger << "'" << ENDM;
  mTask->finalize(trigger, mServices);
  mPublicationCallback(mObjectManager->getNonOwningArray(), trigger.timestamp, trigger.timestamp + objectValidity);
  mTaskState = TaskState::Finished;
}
const std::string& PostProcessingRunner::getName()
{
  return mName;
}

PostProcessingRunnerConfig PostProcessingRunner::extractConfig(const CommonSpec& commonSpec, const PostProcessingTaskSpec& ppTaskSpec)
{
  return {
    ppTaskSpec.taskName,
    commonSpec.database,
    commonSpec.consulUrl,
    commonSpec.infologgerFilterDiscardDebug,
    commonSpec.infologgerDiscardLevel,
    commonSpec.postprocessingPeriod,
    "",
    ppTaskSpec.tree
  };
}

MOCPublicationCallback publishToDPL(framework::DataAllocator& allocator, std::string outputBinding)
{
  return [&allocator = allocator, outputBinding = std::move(outputBinding)](const MonitorObjectCollection* moc, long, long) {
    // TODO pass timestamps to objects, so they are later stored correctly.
    allocator.snapshot(framework::OutputRef{ outputBinding }, *moc);
  };
}

MOCPublicationCallback publishToRepository(o2::quality_control::repository::DatabaseInterface& repository)
{
  return [&](const MonitorObjectCollection* collection, long from, long to) {
    for (const TObject* mo : *collection) {
      // We have to copy the object so we can pass a shared_ptr.
      // This is not ideal, but MySQL interface requires shared ptrs to queue the objects.
      repository.storeMO(std::shared_ptr<MonitorObject>(dynamic_cast<MonitorObject*>(mo->Clone())), from, to);
    }
  };
}

} // namespace o2::quality_control::postprocessing
