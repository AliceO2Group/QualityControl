///
/// \file   TaskDataProcessor.cxx
/// \author Barthelemy von Haller
/// \author Piotr Konopka
///

#include <memory>

#include <fairmq/FairMQDevice.h>

// O2
#include "Common/Exceptions.h"
#include "Configuration/ConfigurationFactory.h"
#include "Framework/RawDeviceService.h"
#include "Monitoring/MonitoringFactory.h"
#include "QualityControl/QcInfoLogger.h"
#include "QualityControl/TaskDataProcessor.h"
#include "QualityControl/TaskFactory.h"

namespace o2 {
namespace quality_control {
namespace core {

using namespace o2::configuration;
using namespace o2::monitoring;
using namespace std::chrono;

TaskDataProcessor::TaskDataProcessor(std::string taskName, std::string configurationSource)
  : mTaskName(taskName),
    mNumberBlocks(0),
    mTotalNumberObjectsPublished(0),
    mLastNumberObjects(0),
    mCycleOn(false),
    mCycleNumber(0),
    mMonitorObjectsSpec("", "", 0)
{
  // setup configuration
  mConfigFile = ConfigurationFactory::getConfiguration(configurationSource);
  populateConfig(mTaskName);

  // setup monitoring
  mCollector = MonitoringFactory::Get("infologger://");

  // setup publisher
  mObjectsManager = std::make_shared<ObjectsManager>(mTaskConfig);

  // setup task
  TaskFactory f;
  mTask = f.create<TaskInterfaceDPL>(mTaskConfig, mObjectsManager); // TODO could we use unique_ptr ?
}

TaskDataProcessor::~TaskDataProcessor()
{
  endOfActivity();
}

void TaskDataProcessor::initCallback(InitContext& iCtx)
{
  QcInfoLogger::GetInstance() << "initialize TaskDevicee" << AliceO2::InfoLogger::InfoLogger::endm;

  // init user's task
  mTask->initialize(iCtx);

  // in the future the start of an activity/run will come from the control
  startOfActivity();

  mStatsTimer.reset(10000000); // 10 s.
  mLastNumberObjects = 0;

  QcInfoLogger::GetInstance() << "cycle " << mCycleNumber << AliceO2::InfoLogger::InfoLogger::endm;
  mNumberBlocks = 0;
  mCycleOn = true;

  // start a timer for finishing cycle and publishing the results
//  mCycleTimer = std::make_shared<boost::asio::deadline_timer>(io, boost::posix_time::seconds(mTaskConfig.cycleDurationSeconds));
//  mCycleTimer->async_wait(boost::bind(&TaskDataProcessor::finishCycle, this));
//  ioThread = std::make_shared<std::thread>([&] { io.run(); });
}

void TaskDataProcessor::processCallback(ProcessingContext& pCtx)
{
  std::lock_guard<std::recursive_mutex> lock(mTaskMutex);

  if (!mCycleOn) {
    if (/*CheckCurrentState(RUNNING) &&*/ (mTaskConfig.maxNumberCycles < 0 ||
                                           mCycleNumber < mTaskConfig.maxNumberCycles)) {

      QcInfoLogger::GetInstance() << "cycle " << mCycleNumber << AliceO2::InfoLogger::InfoLogger::endm;

      mTask->startOfCycle();

      mNumberBlocks = 0;
      mCycleOn = true;
    }
  }
  if (mCycleOn) {

    mTask->monitorData(pCtx);
    mNumberBlocks++;
  }

  // if 10 s we publish stats
  if (mStatsTimer.isTimeout()) {
    double current = mStatsTimer.getTime();
    int objectsPublished = (mTotalNumberObjectsPublished - mLastNumberObjects);
    mLastNumberObjects = mTotalNumberObjectsPublished;
    mCollector->send({objectsPublished / current, "QC_task_Rate_objects_published_per_10_seconds"});
    mStatsTimer.increment();

    // temporarily here, until timer callback is implemented in dpl
    timerCallback(pCtx);
  }
}

void TaskDataProcessor::timerCallback(ProcessingContext& pCtx)
{
  finishCycle(pCtx.outputs());
}

o2::header::DataDescription TaskDataProcessor::taskDataDescription(const std::string taskName)
{
  o2::header::DataDescription description;
  description.runtimeInit(std::string(taskName.substr(0,o2::header::DataDescription::size-3) + "-mo").c_str());
  return description;
}

void TaskDataProcessor::populateConfig(std::string taskName)
{
  try {
    std::string prefix = std::string("qc/tasks_config/");
    std::string taskDefinitionName = mConfigFile->get<std::string>(prefix + taskName + "/taskDefinition").value();

    mTaskConfig.taskName = taskName;
    mTaskConfig.moduleName = mConfigFile->get<std::string>(prefix + taskDefinitionName + "/moduleName").value();
    mTaskConfig.className = mConfigFile->get<std::string>(prefix + taskDefinitionName + "/className").value();
    mTaskConfig.cycleDurationSeconds = mConfigFile->get<int>(prefix + taskDefinitionName + "/cycleDurationSeconds").value_or(10);
    mTaskConfig.maxNumberCycles = mConfigFile->get<int>(prefix + taskDefinitionName + "/maxNumberCycles").value_or(-1);

    // maybe it should be moved somewhere else?
    std::string taskInputsNames = mConfigFile->getString(prefix + taskDefinitionName + "/inputs").value();
    std::vector<std::string> taskInputsSplit;
    boost::split(taskInputsSplit, taskInputsNames, boost::is_any_of(","));

    for (auto&& input : taskInputsSplit) {
      InputSpec inputSpec;
      inputSpec.binding = mConfigFile->getString(prefix + input + "/inputName").value();
      inputSpec.origin.runtimeInit(mConfigFile->getString(prefix + input + "/dataOrigin").value().c_str());
      inputSpec.description.runtimeInit(mConfigFile->getString(prefix + input + "/dataDescription").value().c_str());
      size_t len = strlen(inputSpec.description.str);
      if (len < inputSpec.description.size - 2) {
        inputSpec.description.str[len] = '_';
        inputSpec.description.str[len + 1] = 'S';
      }
      else {
        BOOST_THROW_EXCEPTION(AliceO2::Common::FatalException() << AliceO2::Common::errinfo_details(
          std::string("Too long description name: ") + inputSpec.description.str));
      }
      inputSpec.subSpec = 0;
      mInputSpecs.push_back(inputSpec);
    }

    mMonitorObjectsSpec.origin.runtimeInit("QC");
    mMonitorObjectsSpec.description = taskDataDescription(taskName);
    mMonitorObjectsSpec.subSpec = 0;
    mMonitorObjectsSpec.lifetime = o2::framework::Lifetime::QA;

  } catch (...) { // catch already here the configuration exception and print it
    // because if we are in a constructor, the exception could be lost
    std::string diagnostic = boost::current_exception_diagnostic_information();
    std::cerr << "Unexpected exception, diagnostic information follows:\n" << diagnostic << std::endl;
    throw;
  }
}

void TaskDataProcessor::startOfActivity()
{
  mTimerTotalDurationActivity.reset();
  Activity activity(mConfigFile->get<int>("qc/config/Activity/number").value(), mConfigFile->get<int>("qc/config/Activity/type").value());
  mTask->startOfActivity(activity);
}

void TaskDataProcessor::endOfActivity()
{
  Activity activity(mConfigFile->get<int>("qc/config/Activity/number").value(), mConfigFile->get<int>("qc/config/Activity/type").value());
  mTask->endOfActivity(activity);

  double rate = mTotalNumberObjectsPublished / mTimerTotalDurationActivity.getTime();
  mCollector->send({rate, "QC_task_Rate_objects_published_per_second_whole_run"});
  mCollector->send({ba::mean(mPCpus), "QC_task_Mean_pcpu_whole_run"});
  mCollector->send({ba::mean(mPMems), "QC_task_Mean_pmem_whole_run"});
}

void TaskDataProcessor::finishCycle(DataAllocator& outputs)
{
  {
    std::lock_guard<std::recursive_mutex> lock(mTaskMutex);

    mTask->endOfCycle();

    double durationCycle = 0; // (boost::posix_time::seconds(mTaskConfig.cycleDurationSeconds) - mCycleTimer->expires_from_now()).total_nanoseconds() / double(1e9);
    // mCycleTimer->expires_at(mCycleTimer->expires_at() + boost::posix_time::seconds(mTaskConfig.cycleDurationSeconds));

    // publication
    unsigned long numberObjectsPublished = publish(outputs);

    // monitoring metrics
    double durationPublication = 0; // (boost::posix_time::seconds(mTaskConfig.cycleDurationSeconds) - mCycleTimer->expires_from_now()).total_nanoseconds() / double(1e9);
    mCollector->send({mNumberBlocks, "QC_task_Numberofblocks_in_cycle"});
    mCollector->send({durationCycle, "QC_task_Module_cycle_duration"});
    mCollector->send({durationPublication, "QC_task_Publication_duration"});
    mCollector->send({(int) numberObjectsPublished,
                     "QC_task_Number_objects_published_in_cycle"}); // cast due to Monitoring accepting only int
    double rate = numberObjectsPublished / (durationCycle + durationPublication);
    mCollector->send({rate, "QC_task_Rate_objects_published_per_second"});
    mTotalNumberObjectsPublished += numberObjectsPublished;
    //std::vector<std::string> pidStatus = mMonitor->getPIDStatus(::getpid());
    //mPCpus(std::stod(pidStatus[3]));
    //mPMems(std::stod(pidStatus[4]));
    double whole_run_rate = mTotalNumberObjectsPublished / mTimerTotalDurationActivity.getTime();
    mCollector->send({mTotalNumberObjectsPublished, "QC_task_Total_objects_published_whole_run"});
    mCollector->send({mTimerTotalDurationActivity.getTime(), "QC_task_Total_duration_activity_whole_run"});
    mCollector->send({whole_run_rate, "QC_task_Rate_objects_published_per_second_whole_run"});
//    mCollector->send({std::stod(pidStatus[3]), "QC_task_Mean_pcpu_whole_run"});
    mCollector->send({ba::mean(mPMems), "QC_task_Mean_pmem_whole_run"});

    mCycleNumber++;
    mCycleOn = false;
  }
  // restart timer
//  mCycleTimer->async_wait(boost::bind(&TaskDataProcessor::finishCycle, this));
}


unsigned long TaskDataProcessor::publish(DataAllocator& outputs)
{
  unsigned int sentMessages = 0;

  for (auto& pair : *mObjectsManager) {

    auto* mo = pair.second;
    outputs.snapshot<MonitorObject>(
      Output{
        mMonitorObjectsSpec.origin,
        mMonitorObjectsSpec.description,
        mMonitorObjectsSpec.subSpec,
        mMonitorObjectsSpec.lifetime
        },
      *mo);

    QcInfoLogger::GetInstance() << "Sending \"" << mo->getName() << "\"" << AliceO2::InfoLogger::InfoLogger::endm;
    sentMessages++;
  }

  return sentMessages;
}

void TaskDataProcessor::CustomCleanupTMessage(void* data, void* object) { delete (TMessage*)object; }

}
}
}
