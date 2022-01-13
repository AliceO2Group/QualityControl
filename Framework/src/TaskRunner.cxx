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

///
/// \file   TaskRunner.cxx
/// \author Barthelemy von Haller
/// \author Piotr Konopka
///

#include "QualityControl/TaskRunner.h"

#include <memory>

// O2
#include <Common/Exceptions.h>
#include <Monitoring/MonitoringFactory.h>
#include <DataSampling/DataSampling.h>

#include <Framework/CallbackService.h>
#include <Framework/CompletionPolicyHelpers.h>
#include <Framework/TimesliceIndex.h>
#include <Framework/DataSpecUtils.h>
#include <Framework/InputRecordWalker.h>
#include <Framework/InputSpan.h>

#include "QualityControl/QcInfoLogger.h"
#include "QualityControl/TaskFactory.h"
#include "QualityControl/runnerUtils.h"
#include "QualityControl/InfrastructureSpecReader.h"
#include "QualityControl/TaskRunnerFactory.h"

#include <string>
#include <TFile.h>
#include <boost/property_tree/ptree.hpp>
#include <TSystem.h>

using namespace std;

const auto current_diagnostic = boost::current_exception_diagnostic_information;

namespace o2::quality_control::core
{

using namespace o2::framework;
using namespace o2::header;
using namespace o2::configuration;
using namespace o2::monitoring;
using namespace o2::utilities;
using namespace std::chrono;
using namespace AliceO2::Common;

TaskRunner::TaskRunner(const TaskRunnerConfig& config)
  : mTaskConfig(config),
    mRunNumber(0)
{
}

void TaskRunner::refreshConfig(InitContext& iCtx)
{
  try {
    // get the tree
    auto updatedTree = iCtx.options().get<boost::property_tree::ptree>("qcConfiguration");

    if(updatedTree.empty()) {
      ILOG(Warning, Devel) << "Templated config tree is empty, we continue with the original one" << ENDM;
    } else {
      if(gSystem->Getenv("O2_QC_DEBUG_CONFIG_TREE")) { // until we are sure it works, keep a backdoor
        ILOG(Debug,Devel) << "We print the tree we got from the ECS via DPL : " << ENDM;
        printTree(updatedTree);
      }

      // prepare the information we need
      auto infrastructureSpec = InfrastructureSpecReader::readInfrastructureSpec(updatedTree);
      // find the correct taskSpec
      auto it = find_if(infrastructureSpec.tasks.begin(),
                        infrastructureSpec.tasks.end(),
                        [this](const TaskSpec& ts) {return ts.taskName == mTaskConfig.taskName;});
      if (it != infrastructureSpec.tasks.end()) {
        mTaskConfig = TaskRunnerFactory::extractConfig(infrastructureSpec.common,  *it, mTaskConfig.parallelTaskID,  it->resetAfterCycles);
        ILOG(Debug, Devel) << "Configuration refreshed" << ENDM;
      } else {
        ILOG(Error, Support) << "Could not find the task " << mTaskConfig.taskName <<
          " in the templated config provided by ECS, we continue with the original config" << ENDM;
      }
    }
  } catch (std::invalid_argument & error) {
    // ignore the error, we just skip the update of the config file. It can be legit, e.g. in command line mode
    ILOG(Warning, Devel) << "Could not get updated config tree in TaskRunner::init() - `qcConfiguration` could not be retrieved" << ENDM;
  }
}

void TaskRunner::init(InitContext& iCtx)
{
  AliceO2::InfoLogger::InfoLoggerContext* ilContext = nullptr;
  AliceO2::InfoLogger::InfoLogger* il = nullptr;
  try {
    ilContext = &iCtx.services().get<AliceO2::InfoLogger::InfoLoggerContext>();
    il = &iCtx.services().get<AliceO2::InfoLogger::InfoLogger>();
  } catch (const RuntimeErrorRef& err) {
    ILOG(Error, Devel) << "Could not find the DPL InfoLogger" << ENDM;
  }
  QcInfoLogger::init("task/" + mTaskConfig.taskName,
                     mTaskConfig.infologgerFilterDiscardDebug,
                     mTaskConfig.infologgerDiscardLevel,
                     il,
                     ilContext);

  ILOG(Info, Support) << "Initializing TaskRunner" << ENDM;

  refreshConfig(iCtx);

  try {
    loadTaskConfig();
  } catch (...) {
    // catch the configuration exception and print it to avoid losing it
    ILOG(Fatal, Ops) << "Unexpected exception during configuration:\n"
                     << current_diagnostic(true) << ENDM;
    throw;
  }

  // registering state machine callbacks
  iCtx.services().get<CallbackService>().set(CallbackService::Id::Start, [this, &services = iCtx.services()]() { start(services); });
  iCtx.services().get<CallbackService>().set(CallbackService::Id::Reset, [this]() { reset(); });
  iCtx.services().get<CallbackService>().set(CallbackService::Id::Stop, [this]() { stop(); });

  // setup monitoring
  mCollector = MonitoringFactory::Get(mTaskConfig.monitoringUrl);
  mCollector->addGlobalTag(tags::Key::Subsystem, tags::Value::QC);
  mCollector->addGlobalTag("TaskName", mTaskConfig.taskName);

  // setup publisher
  mObjectsManager = std::make_shared<ObjectsManager>(mTaskConfig.taskName, mTaskConfig.className, mTaskConfig.detectorName, mTaskConfig.consulUrl, mTaskConfig.parallelTaskID);

  // setup user's task
  TaskFactory f;
  mTask.reset(f.create(mTaskConfig, mObjectsManager));
  mTask->setMonitoring(mCollector);

  // init user's task
  mTask->loadCcdb(mTaskConfig.conditionUrl);
  mTask->initialize(iCtx);

  mNoMoreCycles = false;
  mCycleNumber = 0;
}

void TaskRunner::run(ProcessingContext& pCtx)
{
  if (mNoMoreCycles) {
    ILOG(Info, Support) << "The maximum number of cycles (" << mTaskConfig.maxNumberCycles << ") has been reached"
                        << " or the device has received an EndOfStream signal. Won't start a new cycle." << ENDM;
    return;
  }

  if (!mCycleOn) {
    startCycle();
  }

  auto [dataReady, timerReady] = validateInputs(pCtx.inputs());

  if (dataReady) {
    mTask->monitorData(pCtx);
    updateMonitoringStats(pCtx);
  }

  if (timerReady) {
    finishCycle(pCtx.outputs());
    if (mTaskConfig.resetAfterCycles > 0 && (mCycleNumber % mTaskConfig.resetAfterCycles == 0)) {
      mTask->reset();
    }
    if (mTaskConfig.maxNumberCycles < 0 || mCycleNumber < mTaskConfig.maxNumberCycles) {
      startCycle();
    } else {
      mNoMoreCycles = true;
    }
  }
}

CompletionPolicy::CompletionOp TaskRunner::completionPolicyCallback(o2::framework::InputSpan const& inputs)
{
  // fixme: we assume that there is one timer input and the rest are data inputs. If some other implicit inputs are
  //  added, this will break.
  size_t dataInputsExpected = inputs.size() - 1;
  size_t dataInputsPresent = 0;

  CompletionPolicy::CompletionOp action = CompletionPolicy::CompletionOp::Wait;

  for (auto& input : inputs) {
    if (input.header == nullptr) {
      continue;
    }

    const auto* dataHeader = CompletionPolicyHelpers::getHeader<DataHeader>(input);
    assert(dataHeader);

    if (!strncmp(dataHeader->dataDescription.str, "TIMER", 5)) {
      action = CompletionPolicy::CompletionOp::Consume;
    } else {
      dataInputsPresent++;
    }
  }

  ILOG(Debug, Trace) << "Completion policy callback. "
                     << "Total inputs possible: " << inputs.size()
                     << ", data inputs: " << dataInputsPresent
                     << ", timer inputs: " << (action == CompletionPolicy::CompletionOp::Consume) << ENDM;

  if (dataInputsPresent == dataInputsExpected) {
    action = CompletionPolicy::CompletionOp::Consume;
  }

  ILOG(Debug, Trace) << "Action: " << action << ENDM;

  return action;
}

std::string TaskRunner::createTaskRunnerIdString()
{
  return std::string("QC-TASK-RUNNER");
}

header::DataOrigin TaskRunner::createTaskDataOrigin()
{
  return header::DataOrigin{ "QC" };
}

header::DataDescription TaskRunner::createTaskDataDescription(const std::string& taskName)
{
  if (taskName.empty()) {
    BOOST_THROW_EXCEPTION(FatalException() << errinfo_details("Empty taskName for task's data description"));
  }
  o2::header::DataDescription description;
  if (taskName.length() > header::DataDescription::size - 3) {
    ILOG(Warning, Devel) << "Task name is longer than " << header::DataDescription::size - 3 << ", it might cause name clashes in the DPL workflow" << ENDM;
  }
  description.runtimeInit(std::string(taskName.substr(0, header::DataDescription::size - 3) + "-mo").c_str());
  return description;
}

header::DataDescription TaskRunner::createTimerDataDescription(const std::string& taskName)
{
  if (taskName.empty()) {
    BOOST_THROW_EXCEPTION(FatalException() << errinfo_details("Empty taskName for timers's data description"));
  }
  // hash the taskName to avoid clashing if the name is long and the beginning is identical
  auto hashedName = std::hash<std::string>{}(taskName);
  hashedName = hashedName % 10000000000LU; // 10 characters max
  std::ostringstream ss;
  ss << std::setw(10) << std::setfill('0') << hashedName; // 10 characters min
  o2::header::DataDescription description;
  description.runtimeInit(std::string("TIMER-" + ss.str()).substr(0, header::DataDescription::size).c_str());
  return description;
}

void TaskRunner::endOfStream(framework::EndOfStreamContext& eosContext)
{
  ILOG(Info, Support) << "Received an EndOfStream, finishing the current cycle" << ENDM;
  finishCycle(eosContext.outputs());
  mNoMoreCycles = true;
}

void TaskRunner::start(const ServiceRegistry& services)
{
  mRunNumber = o2::quality_control::core::computeRunNumber(services, mTaskConfig.fallbackRunNumber);
  QcInfoLogger::setRun(mRunNumber);
  string partitionName = computePartitionName(services);
  QcInfoLogger::setPartition(partitionName);

  try {
    startOfActivity();

    if (mNoMoreCycles) {
      ILOG(Info, Support) << "The maximum number of cycles (" << mTaskConfig.maxNumberCycles << ") has been reached"
                          << " or the device has received an EndOfStream signal. Won't start a new cycle." << ENDM;
      return;
    }

    startCycle();
  } catch (...) {
    // we catch here because we don't know where it will go in DPL's CallbackService
    ILOG(Error, Support) << "Error caught in start() :\n"
                         << current_diagnostic(true) << ENDM;
    throw;
  }
}

void TaskRunner::stop()
{
  try {
    if (mCycleOn) {
      mTask->endOfCycle();
      mCycleNumber++;
      mCycleOn = false;
    }
    endOfActivity();
    mTask->reset();
    mRunNumber = 0;
  } catch (...) {
    // we catch here because we don't know where it will go in DPL's CallbackService
    ILOG(Error, Support) << "Error caught in stop() :\n"
                         << current_diagnostic(true) << ENDM;
    throw;
  }
}

void TaskRunner::reset()
{
  try {
    mTask.reset();
    mCollector.reset();
    mObjectsManager.reset();
    mRunNumber = 0;
  } catch (...) {
    // we catch here because we don't know where it will go in DPL's CallbackService
    ILOG(Error, Support) << "Error caught in reset() :\n"
                         << current_diagnostic(true) << ENDM;
    throw;
  }
}

std::tuple<bool /*data ready*/, bool /*timer ready*/> TaskRunner::validateInputs(const framework::InputRecord& inputs)
{
  size_t dataInputsPresent = 0;
  bool timerReady = false;

  for (auto& input : inputs) {
    if (input.header != nullptr) {

      const auto* dataHeader = get<DataHeader*>(input.header);
      assert(dataHeader);

      if (!strncmp(dataHeader->dataDescription.str, "TIMER", 5)) {
        timerReady = true;
      } else {
        dataInputsPresent++;
      }
    }
  }
  bool dataReady = dataInputsPresent == inputs.size() - 1;

  return { dataReady, timerReady };
}

void TaskRunner::loadTaskConfig() // todo consider renaming
{
  ILOG(Info, Support) << "Loading configuration" << ENDM;

  QcInfoLogger::setDetector(mTaskConfig.detectorName);

  ILOG(Info, Support) << "Configuration loaded : " << ENDM;
  ILOG(Info, Support) << ">> Task name : " << mTaskConfig.taskName << ENDM;
  ILOG(Info, Support) << ">> Module name : " << mTaskConfig.moduleName << ENDM;
  ILOG(Info, Support) << ">> Detector name : " << mTaskConfig.detectorName << ENDM;
  ILOG(Info, Support) << ">> Cycle duration seconds : " << mTaskConfig.cycleDurationSeconds << ENDM;
  ILOG(Info, Support) << ">> Max number cycles : " << mTaskConfig.maxNumberCycles << ENDM;
  ILOG(Info, Support) << ">> Save to file : " << mTaskConfig.saveToFile << ENDM;
}

void TaskRunner::startOfActivity()
{
  // stats
  mTimerTotalDurationActivity.reset();
  mTotalNumberObjectsPublished = 0;

  // Start activity in module's stask and update objectsManager
  Activity activity(mRunNumber, mTaskConfig.activityType, mTaskConfig.activityPeriodName, mTaskConfig.activityPassName, mTaskConfig.activityProvenance);
  ILOG(Info, Ops) << "Starting run " << mRunNumber << ENDM;
  mObjectsManager->setActivity(activity);
  mCollector->setRunNumber(mRunNumber);
  mTask->startOfActivity(activity);
  mObjectsManager->updateServiceDiscovery();
}

void TaskRunner::endOfActivity()
{
  Activity activity(mRunNumber, mTaskConfig.activityType, mTaskConfig.activityPeriodName, mTaskConfig.activityPassName, mTaskConfig.activityProvenance);
  ILOG(Info, Ops) << "Stopping run " << mRunNumber << ENDM;
  mTask->endOfActivity(activity);
  mObjectsManager->removeAllFromServiceDiscovery();

  double rate = mTotalNumberObjectsPublished / mTimerTotalDurationActivity.getTime();
  mCollector->send(Metric{ "qc_objects_published" }.addValue(rate, "per_second_whole_run"));
}

void TaskRunner::startCycle()
{
  ILOG(Debug, Ops) << "Start cycle " << mCycleNumber << ENDM;
  mTask->startOfCycle();
  mNumberMessagesReceivedInCycle = 0;
  mNumberObjectsPublishedInCycle = 0;
  mDataReceivedInCycle = 0;
  mTimerDurationCycle.reset();
  mCycleOn = true;
}

void TaskRunner::finishCycle(DataAllocator& outputs)
{
  ILOG(Debug, Ops) << "Finish cycle " << mCycleNumber << ENDM;
  mTask->endOfCycle();

  mNumberObjectsPublishedInCycle += publish(outputs);
  mTotalNumberObjectsPublished += mNumberObjectsPublishedInCycle;
  saveToFile();

  publishCycleStats();
  mObjectsManager->updateServiceDiscovery();

  mCycleNumber++;
  mCycleOn = false;

  if (mTaskConfig.maxNumberCycles == mCycleNumber) {
    ILOG(Info, Support) << "The maximum number of cycles (" << mTaskConfig.maxNumberCycles << ") has been reached."
                        << " The task will not do anything from now on." << ENDM;
  }
}

void TaskRunner::updateMonitoringStats(ProcessingContext& pCtx)
{
  mNumberMessagesReceivedInCycle++;
  for (const auto& input : InputRecordWalker(pCtx.inputs())) {
    const auto* inputHeader = header::get<header::DataHeader*>(input.header);
    if (inputHeader == nullptr) {
      ILOG(Warning, Devel) << "No DataHeader found in message, ignoring this one for the statistics." << ENDM;
      continue;
    }
    mDataReceivedInCycle += inputHeader->headerSize + inputHeader->payloadSize;
  }
}

void TaskRunner::publishCycleStats()
{
  double cycleDuration = mTimerDurationCycle.getTime();
  double rate = mNumberObjectsPublishedInCycle / (cycleDuration + mLastPublicationDuration);
  double rateMessagesReceived = mNumberMessagesReceivedInCycle / (cycleDuration + mLastPublicationDuration);
  double rateDataReceived = mDataReceivedInCycle / (cycleDuration + mLastPublicationDuration);
  double wholeRunRate = mTotalNumberObjectsPublished / mTimerTotalDurationActivity.getTime();
  double totalDurationActivity = mTimerTotalDurationActivity.getTime();

  mCollector->send(Metric{ "qc_data_received" }
                     .addValue(mNumberMessagesReceivedInCycle, "messages_in_cycle")
                     .addValue(rateMessagesReceived, "messages_per_second")
                     .addValue(mDataReceivedInCycle, "data_in_cycle")
                     .addValue(rateDataReceived, "data_per_second"));

  mCollector->send(Metric{ "qc_duration" }
                     .addValue(cycleDuration, "module_cycle")
                     .addValue(mLastPublicationDuration, "publication")
                     .addValue(totalDurationActivity, "activity_whole_run"));

  mCollector->send(Metric{ "qc_objects_published" }
                     .addValue(mNumberObjectsPublishedInCycle, "in_cycle")
                     .addValue(rate, "per_second")
                     .addValue(mTotalNumberObjectsPublished, "whole_run")
                     .addValue(wholeRunRate, "per_second_whole_run"));
}

int TaskRunner::publish(DataAllocator& outputs)
{
  ILOG(Info, Support) << "Publishing " << mObjectsManager->getNumberPublishedObjects() << " MonitorObjects" << ENDM;
  AliceO2::Common::Timer publicationDurationTimer;

  auto concreteOutput = framework::DataSpecUtils::asConcreteDataMatcher(mTaskConfig.moSpec);
  // getNonOwningArray creates a TObjArray containing the monitoring objects, but not
  // owning them. The array is created by new and must be cleaned up by the caller
  std::unique_ptr<MonitorObjectCollection> array(mObjectsManager->getNonOwningArray());
  int objectsPublished = array->GetEntries();

  outputs.snapshot(
    Output{ concreteOutput.origin,
            concreteOutput.description,
            concreteOutput.subSpec,
            mTaskConfig.moSpec.lifetime },
    *array);

  mLastPublicationDuration = publicationDurationTimer.getTime();
  return objectsPublished;
}

void TaskRunner::saveToFile()
{
  if (!mTaskConfig.saveToFile.empty()) {
    ILOG(Debug, Support) << "Save data to file " << mTaskConfig.saveToFile << ENDM;
    TFile f(mTaskConfig.saveToFile.c_str(), "RECREATE");
    for (size_t i = 0; i < mObjectsManager->getNumberPublishedObjects(); i++) {
      mObjectsManager->getMonitorObject(i)->getObject()->Write();
    }
    f.Close();
  }
}

} // namespace o2::quality_control::core
