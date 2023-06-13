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

#include "QualityControl/PostProcessingInterface.h"
#include "QualityControl/PostProcessingFactory.h"
#include "QualityControl/PostProcessingConfig.h"
#include "QualityControl/PostProcessingTaskSpec.h"
#include "QualityControl/TriggerHelpers.h"
#include "QualityControl/DatabaseFactory.h"
#include "QualityControl/QcInfoLogger.h"
#include "QualityControl/CommonSpec.h"
#include "QualityControl/InfrastructureSpecReader.h"
#include "QualityControl/Activity.h"
#include "QualityControl/RootClassFactory.h"
#include "QualityControl/runnerUtils.h"
#include "QualityControl/ConfigParamGlo.h"
#include "QualityControl/MonitorObjectCollection.h"

#include <utility>
#include <Framework/DataAllocator.h>
#include <Framework/DataTakingContext.h>
#include <CommonUtils/ConfigurableParam.h>

using namespace o2::quality_control::core;
using namespace o2::quality_control::repository;

namespace o2::quality_control::postprocessing
{

constexpr long objectValidity = 1000l * 60 * 60 * 24 * 365 * 10;

PostProcessingRunner::PostProcessingRunner(std::string id) //
  : mID(std::move(id))
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
                                 [id = mID](const auto& spec) {
                                   return spec.id == id;
                                 });
  if (ppTaskSpec == specs.postProcessingTasks.end()) {
    throw std::runtime_error("Could not find the configuration of the post-processing task '" + mID + "'");
  }

  init(PostProcessingRunner::extractConfig(specs.common, *ppTaskSpec), PostProcessingConfig{ mID, config });
}

void PostProcessingRunner::init(const PostProcessingRunnerConfig& runnerConfig, const PostProcessingConfig& taskConfig)
{
  mRunnerConfig = runnerConfig;
  mTaskConfig = taskConfig;

  QcInfoLogger::init("post/" + mID, mRunnerConfig.infologgerDiscardParameters);
  ILOG(Info, Support) << "Initializing PostProcessingRunner" << ENDM;

  root_class_factory::loadLibrary(mTaskConfig.moduleName);
  if (!ConfigParamGlo::keyValues.empty()) {
    conf::ConfigurableParam::updateFromString(ConfigParamGlo::keyValues);
  }

  // configuration of the database
  mDatabase = DatabaseFactory::create(mRunnerConfig.database.at("implementation"));
  mDatabase->connect(mRunnerConfig.database);
  ILOG(Info, Support) << "Database that is going to be used > Implementation : " << mRunnerConfig.database.at("implementation") << " / "
                      << " Host : " << mRunnerConfig.database.at("host") << ENDM;

  mObjectManager = std::make_shared<ObjectsManager>(mTaskConfig.taskName, mTaskConfig.className, mTaskConfig.detectorName, mRunnerConfig.consulUrl);
  mObjectManager->setActivity(mTaskConfig.activity);
  mServices.registerService<DatabaseInterface>(mDatabase.get());
  if (mPublicationCallback == nullptr) {
    mPublicationCallback = publishToRepository(*mDatabase);
  }

  // setup user's task
  ILOG(Debug, Devel) << "Creating a user task '" << mTaskConfig.taskName << "'" << ENDM;
  PostProcessingFactory f;
  mTask.reset(f.create(mTaskConfig));
  mTask->setObjectsManager(mObjectManager);
  if (mTask) {
    ILOG(Debug, Devel) << "The user task '" << mTaskConfig.taskName << "' has been successfully created" << ENDM;

    mTaskState = TaskState::Created;
    mTask->setID(mTaskConfig.id);
    mTask->setName(mTaskConfig.taskName);
    mTask->configure(mRunnerConfig.configTree);
  } else {
    throw std::runtime_error("Failed to create the task '" + mTaskConfig.taskName + "' (det " + mTaskConfig.detectorName + ")");
  }
}

bool PostProcessingRunner::run()
{
  ILOG(Debug, Devel) << "Checking triggers of the task '" << mTask->getName() << "' (det " << mTaskConfig.detectorName << ")" << ENDM;

  if (mTaskState == TaskState::Created) {
    if (Trigger trigger = trigger_helpers::tryTrigger(mInitTriggers)) {
      doInitialize(trigger);
      return true;
    }
  }
  if (mTaskState == TaskState::Running) {
    if (Trigger trigger = trigger_helpers::tryTrigger(mUpdateTriggers)) {
      doUpdate(trigger);
      return true;
    }
    if (mUpdateTriggers.empty()) {
      doFinalize({ TriggerType::UserOrControl, true, mTaskConfig.activity });
      return false;
    } else if (Trigger trigger = trigger_helpers::tryTrigger(mStopTriggers)) {
      doFinalize(trigger);
      return false;
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

  ILOG(Info, Support) << "Running the task '" << mTask->getName() << "' (det " << mRunnerConfig.detectorName << ") over " << timestamps.size() << " timestamps." << ENDM;

  doInitialize({ TriggerType::UserOrControl, false, mTaskConfig.activity, timestamps.front() });
  for (size_t i = 1; i < timestamps.size() - 1; i++) {
    doUpdate({ TriggerType::UserOrControl, i == timestamps.size() - 2, mTaskConfig.activity, timestamps[i] });
  }
  doFinalize({ TriggerType::UserOrControl, false, mTaskConfig.activity, timestamps.back() });
}

void PostProcessingRunner::start(framework::ServiceRegistryRef dplServices)
{
  if (dplServices.active<framework::RawDeviceService>()) {
    mTaskConfig.activity.mId = computeRunNumber(dplServices, mTaskConfig.activity.mId);
    mTaskConfig.activity.mType = computeRunType(dplServices, mTaskConfig.activity.mType);
    mTaskConfig.activity.mPeriodName = computePeriodName(dplServices, mTaskConfig.activity.mPeriodName);
    mTaskConfig.activity.mPassName = computePassName(mTaskConfig.activity.mPassName);
    mTaskConfig.activity.mProvenance = computeProvenance(mTaskConfig.activity.mProvenance);
    auto partitionName = computePartitionName(dplServices);
    QcInfoLogger::setPartition(partitionName);
  }
  QcInfoLogger::setRun(mTaskConfig.activity.mId);

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

void PostProcessingRunner::doInitialize(const Trigger& trigger)
{
  ILOG(Info, Support) << "Initializing the user task due to trigger '" << trigger << "'" << ENDM;

  mTask->initialize(trigger, mServices);
  mTaskState = TaskState::Running;

  // We create the triggers just after task init (and not any sooner), so the timer triggers work as expected.
  mUpdateTriggers = trigger_helpers::createTriggers(mTaskConfig.updateTriggers, mTaskConfig);
  mStopTriggers = trigger_helpers::createTriggers(mTaskConfig.stopTriggers, mTaskConfig);
}

void PostProcessingRunner::doUpdate(const Trigger& trigger)
{
  ILOG(Info, Support) << "Updating the user task due to trigger '" << trigger << "'" << ENDM;
  mTask->update(trigger, mServices);
  mObjectManager->setValidity(ValidityInterval{ trigger.timestamp, trigger.timestamp + objectValidity });
  mPublicationCallback(mObjectManager->getNonOwningArray());
}

void PostProcessingRunner::doFinalize(const Trigger& trigger)
{
  if (mTaskState != TaskState::Running) {
    ILOG(Warning, Support) << "Attempt at finalizing the user task although it was not initialized. Skipping the finalization." << ENDM;
    return;
  }
  ILOG(Info, Support) << "Finalizing the user task due to trigger '" << trigger << "'" << ENDM;
  mTask->finalize(trigger, mServices);
  mObjectManager->setValidity(ValidityInterval{ trigger.timestamp, trigger.timestamp + objectValidity });
  mPublicationCallback(mObjectManager->getNonOwningArray());
  mTaskState = TaskState::Finished;
}

const std::string& PostProcessingRunner::getID() const
{
  return mID;
}

PostProcessingRunnerConfig PostProcessingRunner::extractConfig(const CommonSpec& commonSpec, const PostProcessingTaskSpec& ppTaskSpec)
{
  return {
    ppTaskSpec.id,
    ppTaskSpec.taskName,
    ppTaskSpec.detectorName,
    commonSpec.database,
    commonSpec.consulUrl,
    commonSpec.infologgerDiscardParameters,
    commonSpec.postprocessingPeriod,
    "",
    ppTaskSpec.tree
  };
}

MOCPublicationCallback publishToDPL(framework::DataAllocator& allocator, std::string outputBinding)
{
  return [&allocator = allocator, outputBinding = std::move(outputBinding)](const MonitorObjectCollection* moc) {
    // TODO pass timestamps to objects, so they are later stored correctly.
    ILOG(Debug, Support) << "Publishing " << moc->GetEntries() << " MonitorObjects" << ENDM;
    allocator.snapshot(framework::OutputRef{ outputBinding }, *moc);
  };
}

MOCPublicationCallback publishToRepository(o2::quality_control::repository::DatabaseInterface& repository)
{
  return [&](const MonitorObjectCollection* collection) {
    ILOG(Debug, Support) << "Publishing " << collection->GetEntries() << " MonitorObjects" << ENDM;
    for (const TObject* mo : *collection) {
      // We have to copy the object so we can pass a shared_ptr.
      // This is not ideal, but MySQL interface requires shared ptrs to queue the objects.
      repository.storeMO(std::shared_ptr<MonitorObject>(dynamic_cast<MonitorObject*>(mo->Clone())));
    }
  };
}

} // namespace o2::quality_control::postprocessing
