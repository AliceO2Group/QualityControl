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
#include <Framework/ConfigParamRegistry.h>
#include <CommonUtils/ConfigurableParam.h>
#include <DetectorsBase/GRPGeomHelper.h>

#include "QualityControl/ObjectMetadataKeys.h"
#include "QualityControl/QcInfoLogger.h"
#include "QualityControl/TaskFactory.h"
#include "QualityControl/runnerUtils.h"
#include "QualityControl/InfrastructureSpecReader.h"
#include "QualityControl/TaskRunnerFactory.h"
#include "QualityControl/ConfigParamGlo.h"
#include "QualityControl/ObjectsManager.h"
#include "QualityControl/Bookkeeping.h"
#include "QualityControl/TimekeeperFactory.h"
#include "QualityControl/ActivityHelpers.h"
#include "QualityControl/WorkflowType.h"
#include "QualityControl/HashDataDescription.h"
#include "QualityControl/runnerUtils.h"

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
  : mTaskConfig(config)
{
  o2::ccdb::BasicCCDBManager::instance().setFatalWhenNull(false);
}

TaskRunner::~TaskRunner()
{
  ILOG(Debug, Trace) << "TaskRunner destructor (" << this << ")" << ENDM;
}

void TaskRunner::init(InitContext& iCtx)
{
  core::initInfologger(iCtx, mTaskConfig.infologgerDiscardParameters, "task/" + mTaskConfig.name, mTaskConfig.detectorName);
  ILOG(Info, Devel) << "Initializing TaskRunner" << ENDM;

  printTaskConfig();
  Bookkeeping::getInstance().init(mTaskConfig.bookkeepingUrl);

  // registering state machine callbacks
  try {
    iCtx.services().get<CallbackService>().set<CallbackService::Id::Start>([this, services = iCtx.services()]() mutable { start(services); });
    iCtx.services().get<CallbackService>().set<CallbackService::Id::Stop>([this, services = iCtx.services()]() { stop(services); });
    iCtx.services().get<CallbackService>().set<CallbackService::Id::Reset>([this]() { reset(); });
  } catch (o2::framework::RuntimeErrorRef& ref) {
    ILOG(Error) << "Error during initialization: " << o2::framework::error_from_ref(ref).what << ENDM;
  }

  // setup monitoring
  mCollector = MonitoringFactory::Get(mTaskConfig.monitoringUrl);
  mCollector->addGlobalTag(tags::Key::Subsystem, tags::Value::QC);
  mCollector->addGlobalTag("TaskName", mTaskConfig.name);
  mCollector->addGlobalTag("DetectorName", mTaskConfig.detectorName);

  // setup publisher
  mObjectsManager = std::make_shared<ObjectsManager>(mTaskConfig.name, mTaskConfig.className, mTaskConfig.detectorName, mTaskConfig.parallelTaskID);
  mObjectsManager->setMovingWindowsList(mTaskConfig.movingWindows);

  // setup timekeeping
  mDeploymentMode = DefaultsHelpers::deploymentMode();
  auto windowLengthMs = mTaskConfig.movingWindows.empty() ? 0 : (mTaskConfig.cycleDurations.back().first * 1000);
  mTimekeeper = TimekeeperFactory::create(mDeploymentMode, windowLengthMs);
  mTimekeeper->setCCDBOrbitsPerTFAccessor([]() {
    // getNHBFPerTF() returns 128 if it does not know, which can be very misleading.
    // instead we use 0, which will trigger another try when processing another timeslice.
    return o2::base::GRPGeomHelper::instance().getGRPECS() != nullptr ? o2::base::GRPGeomHelper::getNHBFPerTF() : 0;
  });

  // setup user's task
  mTask.reset(TaskFactory::create(mTaskConfig, mObjectsManager));
  mTask->setMonitoring(mCollector);
  mTask->setGlobalTrackingDataRequest(mTaskConfig.globalTrackingDataRequest);
  mTask->setDatabase(mTaskConfig.repository);

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

  if (mTimekeeper->shouldFinishCycle(pCtx.services().get<TimingInfo>())) {
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

  if (isDataReady(pCtx.inputs())) {
    mTimekeeper->updateByTimeFrameID(pCtx.services().get<TimingInfo>().tfCounter);
    mTask->monitorData(pCtx);
    updateMonitoringStats(pCtx);
  }
}

void TaskRunner::finaliseCCDB(ConcreteDataMatcher& matcher, void* obj)
{
  if (mTaskConfig.grpGeomRequest) {
    if (!GRPGeomHelper::instance().finaliseCCDB(matcher, obj)) {
      ILOG(Warning, Devel) << "Could not update CCDB objects requested by GRPGeomHelper" << ENDM;
    }
  }
  mTask->finaliseCCDB(matcher, obj);
}

CompletionPolicy::CompletionOp TaskRunner::completionPolicyCallback(o2::framework::InputSpan const& inputs, std::vector<framework::InputSpec> const& specs, ServiceRegistryRef&)
{
  struct InputCount {
    size_t seen = 0;
    size_t expected = 0;
  };

  InputCount dataInputs;
  InputCount timerInputs;
  InputCount conditionInputs;
  CompletionPolicy::CompletionOp action = CompletionPolicy::CompletionOp::Wait;

  assert(inputs.size() == specs.size());
  for (size_t i = 0; i < inputs.size(); ++i) {
    const auto header = inputs.header(i);
    const auto& spec = specs[i];
    const bool headerPresent = header != nullptr;

    if (spec.lifetime == Lifetime::Timer) {
      timerInputs.seen += headerPresent;
      timerInputs.expected += 1;
    } else if (spec.lifetime == Lifetime::Condition) {
      conditionInputs.seen += headerPresent;
      conditionInputs.expected += 1;
    } else {
      // we do not expect any concrete Lifetimes to be data to leave the room open for new ones
      dataInputs.seen += headerPresent;
      dataInputs.expected += 1;
    }
  }

  if ((dataInputs.expected == dataInputs.seen && conditionInputs.expected == conditionInputs.seen) || timerInputs.seen > 0) {
    action = CompletionPolicy::CompletionOp::Consume;
  }

  ILOG(Debug, Trace) << "Input summary (seen/expected): "
                     << "data " << dataInputs.seen << "/" << dataInputs.expected << ", "
                     << "timer " << timerInputs.seen << "/" << timerInputs.expected << ", "
                     << "condition " << conditionInputs.seen << "/" << conditionInputs.expected
                     << ". Action taken: " << action << ENDM;

  return action;
}

std::string TaskRunner::createTaskRunnerIdString()
{
  return { "qc-task" };
}

header::DataOrigin TaskRunner::createTaskDataOrigin(const std::string& detectorCode, bool movingWindows)
{
  // We need a unique Data Origin, so we can have QC Tasks with the same names for different detectors.
  // However, to avoid colliding with data marked as e.g. TPC/CLUSTERS, we add 'Q' to the data origin, so it is Q<det>.
  std::string originStr = movingWindows ? "W" : "Q";
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

  return quality_control::core::createDataDescription(taskName, TaskRunner::taskDescriptionHashLength);
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
    if (mTaskConfig.disableLastCycle) {
      ILOG(Info, Devel) << "Received an EndOfStream, but the last cycle is disabled" << ENDM;
    } else {
      ILOG(Info, Devel) << "Received an EndOfStream, finishing the current cycle" << ENDM;
      finishCycle(eosContext.outputs());
    }
  }
  mNoMoreCycles = true;
}

void TaskRunner::start(ServiceRegistryRef services)
{
  mActivity = o2::quality_control::core::computeActivity(services, mTaskConfig.fallbackActivity);
  QcInfoLogger::setRun(mActivity.mId);
  QcInfoLogger::setPartition(mActivity.mPartitionName);

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

void TaskRunner::stop(ServiceRegistryRef services)
{
  try {
    mActivity = o2::quality_control::core::computeActivity(services, mActivity);
    if (mCycleOn) {
      mTask->endOfCycle();
      mCycleNumber++;
      mCycleOn = false;
    }
    endOfActivity();
    mTask->reset();
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
    mActivity = Activity();
  } catch (...) {
    // we catch here because we don't know where it will go in DPL's CallbackService
    ILOG(Error, Support) << "Error caught in reset() : "
                         << current_diagnostic(true) << ENDM;
    throw;
  }
}

bool TaskRunner::isDataReady(const framework::InputRecord& inputs)
{
  size_t dataInputsPresent = 0;

  for (auto& input : inputs) {
    if (input.header != nullptr) {

      const auto* dataHeader = get<DataHeader*>(input.header);
      assert(dataHeader);

      if (strncmp(dataHeader->dataDescription.str, "TIMER", 5)) {
        dataInputsPresent++;
      }
    }
  }

  return dataInputsPresent == inputs.size() - 1;
}

void TaskRunner::printTaskConfig() const
{
  ILOG(Info, Devel) << "Configuration loaded > Task name : " << mTaskConfig.name //
                    << " / Module name : " << mTaskConfig.moduleName             //
                    << " / Detector name : " << mTaskConfig.detectorName         //
                    << " / Max number cycles : " << mTaskConfig.maxNumberCycles  //
                    << " / critical : " << mTaskConfig.critical                  //
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
  ILOG(Info, Support) << "Starting run " << mActivity.mId << ENDM;
  mObjectsManager->setActivity(mActivity);

  auto now = getCurrentTimestamp();
  mTimekeeper->setStartOfActivity(mActivity.mValidity.getMin(), mTaskConfig.fallbackActivity.mValidity.getMin(), now, activity_helpers::getCcdbSorTimeAccessor(mActivity.mId));
  mTimekeeper->updateByCurrentTimestamp(mTimekeeper->getActivityDuration().getMin());
  mTimekeeper->setEndOfActivity(mActivity.mValidity.getMax(), mTaskConfig.fallbackActivity.mValidity.getMax(), now, activity_helpers::getCcdbEorTimeAccessor(mActivity.mId));

  mCollector->setRunNumber(mActivity.mId);
  mTask->startOfActivity(mActivity);
}

void TaskRunner::endOfActivity()
{
  ILOG(Info, Support) << "Stopping run " << mActivity.mId << ENDM;

  auto now = getCurrentTimestamp();
  mTimekeeper->updateByCurrentTimestamp(now);
  mTimekeeper->setEndOfActivity(mActivity.mValidity.getMax(), mTaskConfig.fallbackActivity.mValidity.getMax(), now, activity_helpers::getCcdbEorTimeAccessor(mActivity.mId));

  mTask->endOfActivity(mObjectsManager->getActivity());
  mObjectsManager->stopPublishing(PublicationPolicy::ThroughStop);

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

void TaskRunner::registerToBookkeeping()
{
  if (!gSystem->Getenv("O2_QC_DONT_REGISTER_IN_BK")) { // Set this variable to disable the registration
    // register ourselves to the BK at the first cycle
    ILOG(Debug, Devel) << "Registering taskRunner to BookKeeping" << ENDM;
    Bookkeeping::getInstance().registerProcess(mActivity.mId, mTaskConfig.name, mTaskConfig.detectorName, bkp::DplProcessType::QC_TASK, "");
  }
}

void TaskRunner::finishCycle(DataAllocator& outputs)
{
  ILOG(Debug, Support) << "Finish cycle " << mCycleNumber << ENDM;
  // in the async context we print only info/ops logs, it's easier to temporarily elevate this log
  ((mDeploymentMode == DeploymentMode::Grid) ? ILOG(Info, Ops) : ILOG(Info, Devel)) //
    << "The objects validity is "
    << "(" << mTimekeeper->getValidity().getMin() << ", " << mTimekeeper->getValidity().getMax() << "), "
    << "(" << mTimekeeper->getSampleTimespan().getMin() << ", " << mTimekeeper->getSampleTimespan().getMax() << "), "
    << "(" << mTimekeeper->getTimerangeIdRange().getMin() << ", " << mTimekeeper->getTimerangeIdRange().getMax() << ")" << ENDM;
  mTask->endOfCycle();

  if (mCycleNumber == 0) { // register at the end of the first cycle
    registerToBookkeeping();
  }

  mObjectsManager->setValidity(mTimekeeper->getValidity());
  mNumberObjectsPublishedInCycle += publish(outputs);
  mTotalNumberObjectsPublished += mNumberObjectsPublishedInCycle;
  saveToFile();

  publishCycleStats();

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
  array->addOrUpdateMetadata(repository::metadata_keys::cycleNumber, std::to_string(mCycleNumber));
  int objectsPublished = array->GetEntries();

  outputs.snapshot(
    Output{ concreteOutput.origin,
            concreteOutput.description,
            concreteOutput.subSpec },
    *array);

  mLastPublicationDuration = publicationDurationTimer.getTime();
  mObjectsManager->stopPublishing(PublicationPolicy::Once);
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
