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

#include <Framework/CallbackService.h>
#include <Framework/CompletionPolicyHelpers.h>
#include <Framework/DataSpecUtils.h>
#include <Framework/InputRecordWalker.h>
#include <Framework/InputSpan.h>
#include <Framework/DataRefUtils.h>
#include <Framework/EndOfStreamContext.h>
#include <Framework/TimingInfo.h>
#include <Framework/DataTakingContext.h>
#include <Framework/DefaultsHelpers.h>
#include <CommonUtils/ConfigurableParam.h>
#include <DetectorsBase/GRPGeomHelper.h>

#include "QualityControl/QcInfoLogger.h"
#include "QualityControl/TaskFactory.h"
#include "QualityControl/runnerUtils.h"
#include "QualityControl/InfrastructureSpecReader.h"
#include "QualityControl/TaskRunnerFactory.h"
#include "QualityControl/ConfigParamGlo.h"
#include "QualityControl/ObjectsManager.h"
#include "QualityControl/Bookkeeping.h"
#include "QualityControl/TimekeeperSynchronous.h"
#include "QualityControl/TimekeeperAsynchronous.h"
#include "QualityControl/ActivityHelpers.h"

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
using namespace o2::base;
using namespace o2::configuration;
using namespace o2::monitoring;
using namespace std::chrono;
using namespace AliceO2::Common;

TaskRunner::TaskRunner(const TaskRunnerConfig& config)
  : mTaskConfig(config),
    mRunNumber(0)
{
}

TaskRunner::~TaskRunner()
{
  ILOG(Debug, Trace) << "TaskRunner destructor (" << this << ")" << ENDM;
}

void TaskRunner::refreshConfig(InitContext& iCtx)
{
  try {
    // get the tree
    auto updatedTree = iCtx.options().get<boost::property_tree::ptree>("qcConfiguration");

    if (updatedTree.empty()) {
      ILOG(Warning, Devel) << "Templated config tree is empty, we continue with the original one" << ENDM;
    } else {
      if (gSystem->Getenv("O2_QC_DEBUG_CONFIG_TREE")) { // until we are sure it works, keep a backdoor
        ILOG(Debug, Devel) << "We print the tree we got from the ECS via DPL : " << ENDM;
        printTree(updatedTree);
      }

      // prepare the information we need
      auto infrastructureSpec = InfrastructureSpecReader::readInfrastructureSpec(updatedTree);
      // find the correct taskSpec
      auto taskSpecIter = find_if(infrastructureSpec.tasks.begin(),
                                  infrastructureSpec.tasks.end(),
                                  [this](const TaskSpec& ts) { return ts.taskName == mTaskConfig.taskName; });
      if (taskSpecIter != infrastructureSpec.tasks.end()) {
        bool runningWithMergers = mTaskConfig.parallelTaskID != 0; // it is 0 when we are the one and only task instance.
        int resetAfterCycles = TaskRunnerFactory::computeResetAfterCycles(*taskSpecIter, runningWithMergers);
        mTaskConfig = TaskRunnerFactory::extractConfig(infrastructureSpec.common, *taskSpecIter, mTaskConfig.parallelTaskID, resetAfterCycles);
        ILOG(Debug, Devel) << "Configuration refreshed" << ENDM;
      } else {
        ILOG(Error, Support) << "Could not find the task " << mTaskConfig.taskName << " in the templated config provided by ECS, we continue with the original config" << ENDM;
      }
    }
  } catch (std::invalid_argument& error) {
    // ignore the error, we just skip the update of the config file. It can be legit, e.g. in command line mode
    ILOG(Warning, Devel) << "Could not get updated config tree in TaskRunner::init() - `qcConfiguration` could not be retrieved" << ENDM;
  } catch (...) {
    // we catch here because we don't know where it will get lost in dpl, and also we don't care if this part has failed.
    ILOG(Warning, Devel) << "Error caught in refreshConfig() : " << current_diagnostic(true) << ENDM;
  }
}

void TaskRunner::initInfologger(InitContext& iCtx)
{
  // TODO : the method should be merged with the other, similar, methods in *Runners

  AliceO2::InfoLogger::InfoLoggerContext* ilContext = nullptr;
  AliceO2::InfoLogger::InfoLogger* il = nullptr;
  try {
    ilContext = &iCtx.services().get<AliceO2::InfoLogger::InfoLoggerContext>();
    il = &iCtx.services().get<AliceO2::InfoLogger::InfoLogger>();
  } catch (const RuntimeErrorRef& err) {
    ILOG(Error, Devel) << "Could not find the DPL InfoLogger" << ENDM;
  }

  mTaskConfig.infologgerDiscardParameters.discardFile = templateILDiscardFile(mTaskConfig.infologgerDiscardParameters.discardFile, iCtx);
  QcInfoLogger::init("task/" + mTaskConfig.taskName,
                     mTaskConfig.infologgerDiscardParameters,
                     il,
                     ilContext);
  QcInfoLogger::setDetector(mTaskConfig.detectorName);
}

void TaskRunner::init(InitContext& iCtx)
{
  initInfologger(iCtx);
  ILOG(Info, Devel) << "Initializing TaskRunner" << ENDM;

  refreshConfig(iCtx);
  printTaskConfig();
  Bookkeeping::getInstance().init(mTaskConfig.bookkeepingUrl);

  // registering state machine callbacks
  try {
    iCtx.services().get<CallbackService>().set<CallbackService::Id::Start>([this, services = iCtx.services()]() mutable { start(services); });
    iCtx.services().get<CallbackService>().set<CallbackService::Id::Reset>([this]() { reset(); });
    iCtx.services().get<CallbackService>().set<CallbackService::Id::Stop>([this]() { stop(); });
  } catch (o2::framework::RuntimeErrorRef& ref) {
    ILOG(Error) << "Error during initialization: " << o2::framework::error_from_ref(ref).what << ENDM;
  }

  // setup monitoring
  mCollector = MonitoringFactory::Get(mTaskConfig.monitoringUrl);
  mCollector->addGlobalTag(tags::Key::Subsystem, tags::Value::QC);
  mCollector->addGlobalTag("TaskName", mTaskConfig.taskName);
  mCollector->addGlobalTag("DetectorName", mTaskConfig.detectorName);

  // setup publisher
  mObjectsManager = std::make_shared<ObjectsManager>(mTaskConfig.taskName, mTaskConfig.className, mTaskConfig.detectorName, mTaskConfig.consulUrl, mTaskConfig.parallelTaskID);

  // setup timekeeping
  switch (DefaultsHelpers::deploymentMode()) {
    case DeploymentMode::Grid: {
      ILOG(Info, Devel) << "Detected async deployment, object validity will be based on incoming data and available SOR/EOR times" << ENDM;
      mTimekeeper = std::make_shared<TimekeeperAsynchronous>();
      break;
    }
    case DeploymentMode::Local:
    case DeploymentMode::OnlineECS:
    case DeploymentMode::OnlineDDS:
    case DeploymentMode::OnlineAUX:
    case DeploymentMode::FST:
    default: {
      ILOG(Info, Devel) << "Detected sync deployment, object validity will be based primarily on current time" << ENDM;
      mTimekeeper = std::make_shared<TimekeeperSynchronous>();
    }
  }

  // setup user's task
  mTask.reset(TaskFactory::create(mTaskConfig, mObjectsManager));
  mTask->setMonitoring(mCollector);
  mTask->setGlobalTrackingDataRequest(mTaskConfig.globalTrackingDataRequest);

  // load config params
  if (!ConfigParamGlo::keyValues.empty()) {
    conf::ConfigurableParam::updateFromString(ConfigParamGlo::keyValues);
  }
  // load reco helpers
  if (mTaskConfig.grpGeomRequest) {
    GRPGeomHelper::instance().setRequest(mTaskConfig.grpGeomRequest);
  }

  // init user's task
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

  if (mTaskConfig.grpGeomRequest) {
    GRPGeomHelper::instance().checkUpdates(pCtx);
  }

  auto [dataReady, timerReady] = validateInputs(pCtx.inputs());

  if (dataReady) {
    mTimekeeper->updateByTimeFrameID(pCtx.services().get<TimingInfo>().tfCounter, pCtx.services().get<DataTakingContext>().nOrbitsPerTF);
    mTask->monitorData(pCtx);
    updateMonitoringStats(pCtx);
  }

  if (timerReady) {
    mTimekeeper->updateByCurrentTimestamp(pCtx.services().get<TimingInfo>().timeslice / 1000);
    finishCycle(pCtx.outputs());
    if (mTaskConfig.resetAfterCycles > 0 && (mCycleNumber % mTaskConfig.resetAfterCycles == 0)) {
      mTask->reset();
      mTimekeeper->reset();
    }
    if (mTaskConfig.maxNumberCycles < 0 || mCycleNumber < mTaskConfig.maxNumberCycles) {
      startCycle();
    } else {
      mNoMoreCycles = true;
    }
  }
}

void TaskRunner::finaliseCCDB(ConcreteDataMatcher& matcher, void* obj)
{
  if (mTaskConfig.grpGeomRequest) {
    if (!GRPGeomHelper::instance().finaliseCCDB(matcher, obj)) {
      ILOG(Warning, Devel) << "Could not update CCDB objects requested by GRPGeomHelper" << ENDM;
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
  return { "qc-task" };
}

header::DataOrigin TaskRunner::createTaskDataOrigin(const std::string& detectorCode)
{
  // We need a unique Data Origin, so we can have QC Tasks with the same names for different detectors.
  // However, to avoid colliding with data marked as e.g. TPC/CLUSTERS, we add 'Q' to the data origin, so it is Q<det>.
  std::string originStr = "Q";
  if (detectorCode.empty()) {
    ILOG(Warning, Support) << "empty detector code for a task data origin, trying to survive with: DET" << ENDM;
    originStr += "DET";
  } else if (detectorCode.size() > 3) {
    ILOG(Warning, Support) << "too long detector code for a task data origin: " + detectorCode + ", trying to survive with: " + detectorCode.substr(0, 3) << ENDM;
    originStr += detectorCode.substr(0, 3);
  } else {
    originStr += detectorCode;
  }
  o2::header::DataOrigin origin;
  origin.runtimeInit(originStr.c_str());
  return origin;
}

header::DataDescription TaskRunner::createTaskDataDescription(const std::string& taskName)
{
  if (taskName.empty()) {
    BOOST_THROW_EXCEPTION(FatalException() << errinfo_details("Empty taskName for task's data description"));
  }
  o2::header::DataDescription description;
  if (taskName.length() > header::DataDescription::size) {
    ILOG(Warning, Devel) << "Task name \"" << taskName << "\" is longer than " << (int)header::DataDescription::size << ", it might cause name clashes in the DPL workflow" << ENDM;
  }
  description.runtimeInit(std::string(taskName.substr(0, header::DataDescription::size)).c_str());
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
  if (!mCycleOn && mCycleNumber == 0) {
    ILOG(Error, Support) << "An EndOfStream was received before TaskRunner could start the first cycle, probably the device was not started. Something is wrong, doing nothing." << ENDM;
  } else {
    ILOG(Info, Trace) << "Updating timekeeper with a current timestamp upon receiving an EoS message" << ENDM;
    mTimekeeper->updateByCurrentTimestamp(getCurrentTimestamp());
    ILOG(Info, Support) << "Received an EndOfStream, finishing the current cycle" << ENDM;
    finishCycle(eosContext.outputs());
  }
  mNoMoreCycles = true;
}

void TaskRunner::start(ServiceRegistryRef services)
{
  mRunNumber = o2::quality_control::core::computeRunNumber(services, mTaskConfig.fallbackActivity.mId);
  QcInfoLogger::setRun(mRunNumber);
  string partitionName = computePartitionName(services);
  QcInfoLogger::setPartition(partitionName);

  mNoMoreCycles = false;
  mCycleNumber = 0;

  try {
    startOfActivity();
    startCycle();
  } catch (...) {
    // we catch here because we don't know where it will go in DPL's CallbackService
    ILOG(Error, Support) << "Error caught in start() :"
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
    ILOG(Error, Support) << "Error caught in stop() : "
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
    mTimekeeper.reset();
    mRunNumber = 0;
  } catch (...) {
    // we catch here because we don't know where it will go in DPL's CallbackService
    ILOG(Error, Support) << "Error caught in reset() : "
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

void TaskRunner::printTaskConfig() const
{
  ILOG(Info, Devel) << "Configuration loaded > Task name : " << mTaskConfig.taskName //
                    << " / Module name : " << mTaskConfig.moduleName                 //
                    << " / Detector name : " << mTaskConfig.detectorName             //
                    << " / Max number cycles : " << mTaskConfig.maxNumberCycles      //
                    << " / Save to file : " << mTaskConfig.saveToFile
                    << " / Cycle duration seconds : ";
  for (auto& [cycleDuration, period] : mTaskConfig.cycleDurations) {
    ILOG(Info, Devel) << cycleDuration << "s during " << period << "s, ";
  }
  ILOG(Info, Devel) << ENDM;
}

void TaskRunner::startOfActivity()
{
  // stats
  mTimerTotalDurationActivity.reset();
  mTotalNumberObjectsPublished = 0;

  // Start activity in module's task and update objectsManager
  ILOG(Info, Support) << "Starting run " << mRunNumber << ENDM;
  Activity activity = mTaskConfig.fallbackActivity;
  activity.mId = mRunNumber;
  Bookkeeping::getInstance().populateActivity(activity, mRunNumber);
  mObjectsManager->setActivity(activity);

  auto now = getCurrentTimestamp();
  mTimekeeper->setStartOfActivity(activity.mValidity.getMin(), mTaskConfig.fallbackActivity.mValidity.getMin(), now, activity_helpers::getCcdbSorTimeAccessor(mRunNumber));
  mTimekeeper->updateByCurrentTimestamp(mTimekeeper->getActivityDuration().getMin());
  mTimekeeper->setEndOfActivity(activity.mValidity.getMax(), mTaskConfig.fallbackActivity.mValidity.getMax(), now, activity_helpers::getCcdbEorTimeAccessor(mRunNumber));

  mCollector->setRunNumber(mRunNumber);
  mTask->startOfActivity(activity);
  mObjectsManager->updateServiceDiscovery();
}

void TaskRunner::endOfActivity()
{
  ILOG(Info, Support) << "Stopping run " << mRunNumber << ENDM;

  auto now = getCurrentTimestamp();
  mTimekeeper->updateByCurrentTimestamp(now);
  mTimekeeper->setEndOfActivity(0, mTaskConfig.fallbackActivity.mValidity.getMax(), now, activity_helpers::getCcdbEorTimeAccessor(mRunNumber)); // TODO: get end of run from ECS/BK if possible

  mTask->endOfActivity(mObjectsManager->getActivity());
  mObjectsManager->removeAllFromServiceDiscovery();

  double rate = mTotalNumberObjectsPublished / mTimerTotalDurationActivity.getTime();
  mCollector->send(Metric{ "qc_objects_published" }.addValue(rate, "per_second_whole_run"));
}

void TaskRunner::startCycle()
{
  ILOG(Debug, Support) << "Start cycle " << mCycleNumber << ENDM;
  mTask->startOfCycle();
  mNumberMessagesReceivedInCycle = 0;
  mNumberObjectsPublishedInCycle = 0;
  mDataReceivedInCycle = 0;
  mTimerDurationCycle.reset();
  mCycleOn = true;
}

void TaskRunner::finishCycle(DataAllocator& outputs)
{
  ILOG(Debug, Support) << "Finish cycle " << mCycleNumber << ENDM;
  ILOG(Info, Devel) << "According to new validity rules, the objects validity is "
                    << "(" << mTimekeeper->getValidity().getMin() << ", " << mTimekeeper->getValidity().getMax() << "), "
                    << "(" << mTimekeeper->getSampleTimespan().getMin() << ", " << mTimekeeper->getSampleTimespan().getMax() << "), "
                    << "(" << mTimekeeper->getTimerangeIdRange().getMin() << ", " << mTimekeeper->getTimerangeIdRange().getMax() << ")" << ENDM;
  mTask->endOfCycle();

  // this stays until we move to using mTimekeeper.
  auto nowMs = getCurrentTimestamp();
  mObjectsManager->setValidity(ValidityInterval{ nowMs, nowMs + 1000l * 60 * 60 * 24 * 365 * 10 });
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
    const auto* inputHeader = DataRefUtils::getHeader<header::DataHeader*>(input);
    auto payloadSize = DataRefUtils::getPayloadSize(input);
    if (inputHeader == nullptr) {
      ILOG(Warning, Devel) << "No DataHeader found in message, ignoring this one for the statistics." << ENDM;
      continue;
    }
    mDataReceivedInCycle += inputHeader->headerSize + payloadSize;
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
  ILOG(Debug, Support) << "Publishing " << mObjectsManager->getNumberPublishedObjects() << " MonitorObjects" << ENDM;
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
