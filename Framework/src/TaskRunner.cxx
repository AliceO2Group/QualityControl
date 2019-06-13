// Copyright CERN and copyright holders of ALICE O2. This software is
// distributed under the terms of the GNU General Public License v3 (GPL
// Version 3), copied verbatim in the file "COPYING".
//
// See http://alice-o2.web.cern.ch/license for full licensing information.
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.

///
/// \file   TaskDataProcessor.cxx
/// \author Barthelemy von Haller
/// \author Piotr Konopka
///

#include <memory>
#include <iostream>

#include <fairmq/FairMQDevice.h>

// O2
#include "Common/Exceptions.h"
#include "Configuration/ConfigurationFactory.h"
#include "Framework/RawDeviceService.h"
#include "Framework/DataSampling.h"
#include "Framework/CallbackService.h"
#include "Monitoring/MonitoringFactory.h"
#include "QualityControl/QcInfoLogger.h"
#include "QualityControl/TaskFactory.h"
#include "QualityControl/TaskRunner.h"
#include "QualityControl/FileFinish.h"



namespace o2
{
namespace quality_control
{
namespace core
{

using namespace o2::configuration;
using namespace o2::monitoring;
using namespace std::chrono;

TaskRunner::TaskRunner(std::string taskName, std::string configurationSource, size_t id)
  : mTaskName(taskName),
    mNumberBlocks(0),
    mTotalNumberObjectsPublished(0),
    mLastNumberObjects(0),
    mCycleOn(false),
    mCycleNumber(0),
    mMonitorObjectsSpec({"mo"}, createTaskDataOrigin(), createTaskDataDescription(taskName), id),
    mResetAfterPublish(false),
    mTask(nullptr)
{
  // setup configuration
  mConfigFile = ConfigurationFactory::getConfiguration(configurationSource);
  populateConfig(mTaskName);
}

TaskRunner::~TaskRunner()
{
}

void TaskRunner::initCallback(InitContext& iCtx)
{
  QcInfoLogger::GetInstance() << "initializing TaskRunner" << AliceO2::InfoLogger::InfoLogger::endm;

  // registering state machine callbacks
  iCtx.services().get<framework::CallbackService>().set(framework::CallbackService::Id::Start, [this]() { start(); });
  iCtx.services().get<framework::CallbackService>().set(framework::CallbackService::Id::Stop, [this]() { stop(); });
  iCtx.services().get<framework::CallbackService>().set(framework::CallbackService::Id::Reset, [this]() { reset(); });

  // setup monitoring
  mCollector = MonitoringFactory::Get("infologger:///debug?qc");
  //  mCollector = MonitoringFactory::Get("influxdb-udp://aido2mon-gpn.cern.ch:8087");
  mCollector->enableProcessMonitoring();

  // setup publisher
  mObjectsManager = std::make_shared<ObjectsManager>(mTaskConfig);

  // setup user's task
  TaskFactory f;
  mTask.reset(f.create<TaskInterface>(mTaskConfig, mObjectsManager));

  // init user's task
  mTask->initialize(iCtx);
}

void TaskRunner::processCallback(ProcessingContext& pCtx)
{
  if (mTaskConfig.maxNumberCycles >= 0 && mCycleNumber >= mTaskConfig.maxNumberCycles) {
    LOG(INFO) << "The maximum number of cycles (" << mTaskConfig.maxNumberCycles << ") has been reached.";
    return;
  }

  if (!mCycleOn) {
    QcInfoLogger::GetInstance() << "cycle " << mCycleNumber << AliceO2::InfoLogger::InfoLogger::endm;

    mTask->startOfCycle();

    mNumberBlocks = 0;
    mCycleOn = true;
  }

  mTask->monitorData(pCtx);
  mNumberBlocks++;

  QcInfoLogger::GetInstance() << "FileFinish IN Task " << FileFinish << AliceO2::InfoLogger::InfoLogger::endm;


  // if 10 s we publish stats
  if (mStatsTimer.isTimeout() || FileFinish == 1) {
    double current = mStatsTimer.getTime();
    int objectsPublished = (mTotalNumberObjectsPublished - mLastNumberObjects);
    mLastNumberObjects = mTotalNumberObjectsPublished;
    mCollector->send({ objectsPublished / current, "QC_task_Rate_objects_published_per_10_seconds" });
    mStatsTimer.increment();

    // temporarily here, until timer callback is implemented in dpl
    timerCallback(pCtx);
    if (mResetAfterPublish) {
      mTask->reset();
    }
  }
}

void TaskRunner::timerCallback(ProcessingContext& pCtx) { finishCycle(pCtx.outputs()); }

void TaskRunner::setResetAfterPublish(bool resetAfterPublish) { mResetAfterPublish = resetAfterPublish; }

header::DataOrigin TaskRunner::createTaskDataOrigin()
{
  return header::DataOrigin{ "QC" };
}

header::DataDescription TaskRunner::createTaskDataDescription(const std::string& taskName)
{
  o2::header::DataDescription description;
  description.runtimeInit(std::string(taskName.substr(0, header::DataDescription::size - 3) + "-mo").c_str());
  return description;
}

void TaskRunner::start()
{
  startOfActivity();

  mStatsTimer.reset(60000000); // 60 s.
  mLastNumberObjects = 0;

  QcInfoLogger::GetInstance() << "cycle " << mCycleNumber << AliceO2::InfoLogger::InfoLogger::endm;
  mNumberBlocks = 0;
  mCycleOn = true;
}

void TaskRunner::stop()
{
  if (mCycleOn) {
    mTask->endOfCycle();
    mCycleNumber++;
    mCycleOn = false;
  }
  endOfActivity();
  mTask->reset();
}

void TaskRunner::reset()
{
  mTask.reset();
  mCollector.reset();
  mObjectsManager.reset();
}

void TaskRunner::populateConfig(std::string taskName)
{
  try {
    auto tasksConfigList = mConfigFile->getRecursive("qc.tasks");
    auto taskConfigTree = tasksConfigList.find(taskName);
    if (taskConfigTree == tasksConfigList.not_found()) {
      throw;
    }

    mTaskConfig.taskName = taskName;
    mTaskConfig.moduleName = taskConfigTree->second.get<std::string>("moduleName");
    mTaskConfig.className = taskConfigTree->second.get<std::string>("className");
    mTaskConfig.cycleDurationSeconds = taskConfigTree->second.get<int>("cycleDurationSeconds", 10);
    mTaskConfig.maxNumberCycles = taskConfigTree->second.get<int>("maxNumberCycles", -1);

    std::string policiesFilePath = mConfigFile->get<std::string>("dataSamplingPolicyFile", "");
    if (policiesFilePath.empty()) {
      mInputSpecs = framework::DataSampling::InputSpecsForPolicy(mConfigFile.get(), taskConfigTree->second.get<std::string>("dataSamplingPolicy"));
    } else {
      mInputSpecs = framework::DataSampling::InputSpecsForPolicy(policiesFilePath, taskConfigTree->second.get<std::string>("dataSamplingPolicy"));
    }

  } catch (...) { // catch already here the configuration exception and print it
    // because if we are in a constructor, the exception could be lost
    std::string diagnostic = boost::current_exception_diagnostic_information();
    LOG(ERROR) << "Unexpected exception, diagnostic information follows:\n" << diagnostic;
    throw;
  }
  LOG(INFO) << "Configuration loaded : ";
  LOG(INFO) << ">> Task name : " << mTaskConfig.taskName;
  LOG(INFO) << ">> Module name : " << mTaskConfig.moduleName;
  LOG(INFO) << ">> Cycle duration seconds : " << mTaskConfig.cycleDurationSeconds;
  LOG(INFO) << ">> Max number cycles : " << mTaskConfig.maxNumberCycles;
}

void TaskRunner::startOfActivity()
{
  mTimerTotalDurationActivity.reset();
  Activity activity(mConfigFile->get<int>("qc.config.Activity.number"),
                    mConfigFile->get<int>("qc.config.Activity.type"));
  mTask->startOfActivity(activity);
}

void TaskRunner::endOfActivity()
{
  Activity activity(mConfigFile->get<int>("qc.config.Activity.number"),
                    mConfigFile->get<int>("qc.config.Activity.type"));
  mTask->endOfActivity(activity);

  double rate = mTotalNumberObjectsPublished / mTimerTotalDurationActivity.getTime();
  mCollector->send({ rate, "QC_task_Rate_objects_published_per_second_whole_run" });
  mCollector->send({ ba::mean(mPCpus), "QC_task_Mean_pcpu_whole_run" });
  mCollector->send({ ba::mean(mPMems), "QC_task_Mean_pmem_whole_run" });
}

void TaskRunner::finishCycle(DataAllocator& outputs)
{
  mTask->endOfCycle();

  double durationCycle = 0; // (boost::posix_time::seconds(mTaskConfig.cycleDurationSeconds) -
                            // mCycleTimer->expires_from_now()).total_nanoseconds() / double(1e9);
  // mCycleTimer->expires_at(mCycleTimer->expires_at() +
  // boost::posix_time::seconds(mTaskConfig.cycleDurationSeconds));

  // publication
  unsigned long numberObjectsPublished = publish(outputs);

  // monitoring metrics
  double durationPublication = 0; // (boost::posix_time::seconds(mTaskConfig.cycleDurationSeconds) -
                                  // mCycleTimer->expires_from_now()).total_nanoseconds() / double(1e9);
  mCollector->send({ mNumberBlocks, "QC_task_Numberofblocks_in_cycle" });
  mCollector->send({ durationCycle, "QC_task_Module_cycle_duration" });
  mCollector->send({ durationPublication, "QC_task_Publication_duration" });
  mCollector->send({ (int)numberObjectsPublished,
                     "QC_task_Number_objects_published_in_cycle" }); // cast due to Monitoring accepting only int
  double rate = numberObjectsPublished / (durationCycle + durationPublication);
  mCollector->send({ rate, "QC_task_Rate_objects_published_per_second" });
  mTotalNumberObjectsPublished += numberObjectsPublished;
  // std::vector<std::string> pidStatus = mMonitor->getPIDStatus(::getpid());
  // mPCpus(std::stod(pidStatus[3]));
  // mPMems(std::stod(pidStatus[4]));
  double whole_run_rate = mTotalNumberObjectsPublished / mTimerTotalDurationActivity.getTime();
  mCollector->send({ mTotalNumberObjectsPublished, "QC_task_Total_objects_published_whole_run" });
  mCollector->send({ mTimerTotalDurationActivity.getTime(), "QC_task_Total_duration_activity_whole_run" });
  mCollector->send({ whole_run_rate, "QC_task_Rate_objects_published_per_second_whole_run" });
  //    mCollector->send({std::stod(pidStatus[3]), "QC_task_Mean_pcpu_whole_run"});
  mCollector->send({ ba::mean(mPMems), "QC_task_Mean_pmem_whole_run" });

  mCycleNumber++;
  mCycleOn = false;

  if (mTaskConfig.maxNumberCycles == mCycleNumber) {
    LOG(INFO) << "The maximum number of cycles (" << mTaskConfig.maxNumberCycles << ") has been reached."
              << " The task will not do anything from now on.";
  }
}

unsigned long TaskRunner::publish(DataAllocator& outputs)
{
  outputs.adopt(
    Output{ mMonitorObjectsSpec.origin,
            mMonitorObjectsSpec.description,
            mMonitorObjectsSpec.subSpec,
            mMonitorObjectsSpec.lifetime },
    dynamic_cast<TObject*>(mObjectsManager->getNonOwningArray())
  );

  return 1;
}

} // namespace core
} // namespace quality_control
} // namespace o2
