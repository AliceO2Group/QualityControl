//
// Created by pkonopka on 3/9/18.
//

#include <memory>

#include <fairmq/FairMQDevice.h>

#include "Configuration/ConfigurationFactory.h"
#include "Framework/RawDeviceService.h"
#include "Monitoring/MonitoringFactory.h"
#include "QualityControl/QcInfoLogger.h"
#include "QualityControl/TaskDataProcessor.h"
#include "QualityControl/TaskFactory.h"

namespace o2 {
namespace quality_control {
namespace core {

using namespace AliceO2::Configuration;
using namespace AliceO2::Monitoring;
using namespace std::chrono;

DataSetReference convertO2DataModelToDataSetReference(o2::framework::DataRef& input)
{
  const auto* header = o2::header::get<o2::header::DataHeader>(input.header);

  DataSetReference mDataSet = std::make_shared<DataSet>();

  // header
  static DataBlockId id = 1;

  auto block = std::make_shared<SelfReleasingBlockContainer>();
  block->getData()->header.blockType = DataBlockType::H_BASE; // = *static_cast<DataBlockHeaderBase *>(msg->GetData());
  block->getData()->header.headerSize = sizeof(DataBlockHeaderBase);
  block->getData()->header.dataSize = (uint32_t)header->payloadSize;
  block->getData()->header.id = id++;
  block->getData()->header.linkId = 0;

  block->getData()->data = new char[header->payloadSize];
  memcpy(block->getData()->data, const_cast<char*>(input.payload), header->payloadSize);

  mDataSet->push_back(block);

  return mDataSet;
}

TaskDataProcessor::TaskDataProcessor(std::string taskName, std::string configurationSource)
  : mTaskName(taskName),
    mTotalNumberObjectsPublished(0),
    mNumberBlocks(0),
    mLastNumberObjects(0),
    mCycleOn(false),
    mCycleNumber(0)
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
  mTask = f.create(mTaskConfig, mObjectsManager); // TODO could we use unique_ptr ?
}

TaskDataProcessor::~TaskDataProcessor() { endOfActivity(); }

void TaskDataProcessor::initCallback(InitContext& iCtx)
{
  mDevice = iCtx.services().get<o2::framework::RawDeviceService>().device();

  QcInfoLogger::GetInstance() << "initialize TaskDevicee" << AliceO2::InfoLogger::InfoLogger::endm;

  // init user's task
  mTask->initialize();

  // in the future the start of an activity/run will come from the control
  startOfActivity();

  mStatsTimer.reset(10000000); // 10 s.
  mLastNumberObjects = 0;

  QcInfoLogger::GetInstance() << "cycle " << mCycleNumber << AliceO2::InfoLogger::InfoLogger::endm;
  mNumberBlocks = 0;
  mCycleOn = true;

  // start a timer for finishing cycle and publishing the results
  mCycleTimer = std::make_shared<boost::asio::deadline_timer>(io, boost::posix_time::seconds(mTaskConfig.cycleDurationSeconds));
  mCycleTimer->async_wait(boost::bind(&TaskDataProcessor::finishCycle, this));
  ioThread = std::make_shared<std::thread>([&] { io.run(); });
}

void TaskDataProcessor::processCallback(ProcessingContext& pCtx)
{
  std::lock_guard<std::mutex> lock(mTaskMutex);

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

    DataRef input = *pCtx.inputs().begin();
    DataSetReference dataSetReference = convertO2DataModelToDataSetReference(input);
    if (dataSetReference) {
      mTask->monitorDataBlock(dataSetReference);
      mNumberBlocks++;
    }
  }

  // if 10 s we publish stats
  if (mStatsTimer.isTimeout()) {
    double current = mStatsTimer.getTime();
    int objectsPublished = (mTotalNumberObjectsPublished - mLastNumberObjects);
    mLastNumberObjects = mTotalNumberObjectsPublished;
    mCollector->send(objectsPublished / current, "QC_task_Rate_objects_published_per_10_seconds");
    mStatsTimer.increment();
  }
}

void TaskDataProcessor::populateConfig(std::string taskName)
{
  std::string taskDefinitionName = mConfigFile->get<std::string>(taskName + "/taskDefinition").value();

  mTaskConfig.taskName = taskName;
  mTaskConfig.moduleName = mConfigFile->get<std::string>(taskDefinitionName + "/moduleName").value();
  mTaskConfig.className = mConfigFile->get<std::string>(taskDefinitionName + "/className").value();
  mTaskConfig.cycleDurationSeconds = mConfigFile->get<int>(taskDefinitionName + "/cycleDurationSeconds").value_or(10);
  mTaskConfig.maxNumberCycles = mConfigFile->get<int>(taskDefinitionName + "/maxNumberCycles").value_or(-1);
}

void TaskDataProcessor::startOfActivity()
{
  mTimerTotalDurationActivity.reset();
  Activity activity(mConfigFile->get<int>("Activity/number").value(), mConfigFile->get<int>("Activity/type").value());
  mTask->startOfActivity(activity);
}

void TaskDataProcessor::endOfActivity()
{
  Activity activity(mConfigFile->get<int>("Activity/number").value(), mConfigFile->get<int>("Activity/type").value());
  mTask->endOfActivity(activity);

  double rate = mTotalNumberObjectsPublished / mTimerTotalDurationActivity.getTime();
  mCollector->send(rate, "QC_task_Rate_objects_published_per_second_whole_run");
//  mCollector->send(ba::mean(pcpus), "QC_task_Mean_pcpu_whole_run");
//  mCollector->send(ba::mean(pmems), "QC_task_Mean_pmem_whole_run");
}

void TaskDataProcessor::finishCycle()
{
  {
    std::lock_guard<std::mutex> lock(mTaskMutex);

    mTask->endOfCycle();

    double durationCycle = (boost::posix_time::seconds(mTaskConfig.cycleDurationSeconds) - mCycleTimer->expires_from_now()).total_nanoseconds() / double(1e9);
    mCycleTimer->expires_at(mCycleTimer->expires_at() + boost::posix_time::seconds(mTaskConfig.cycleDurationSeconds));

    // publication
    unsigned long numberObjectsPublished = publish();

    // monitoring metrics
    double durationPublication = (boost::posix_time::seconds(mTaskConfig.cycleDurationSeconds) - mCycleTimer->expires_from_now()).total_nanoseconds() / double(1e9);
    mCollector->send(mNumberBlocks, "QC_task_Numberofblocks_in_cycle");
    mCollector->send(durationCycle, "QC_task_Module_cycle_duration");
    mCollector->send(durationPublication, "QC_task_Publication_duration");
    mCollector->send((int) numberObjectsPublished,
                     "QC_task_Number_objects_published_in_cycle"); // cast due to Monitoring accepting only int
    double rate = numberObjectsPublished / (durationCycle + durationPublication);
    mCollector->send(rate, "QC_task_Rate_objects_published_per_second");
    mTotalNumberObjectsPublished += numberObjectsPublished;
    //std::vector<std::string> pidStatus = mMonitor->getPIDStatus(::getpid());
    //pcpus(std::stod(pidStatus[3]));
    //pmems(std::stod(pidStatus[4]));
    double whole_run_rate = mTotalNumberObjectsPublished / mTimerTotalDurationActivity.getTime();
    mCollector->send(mTotalNumberObjectsPublished, "QC_task_Total_objects_published_whole_run");
    mCollector->send(mTimerTotalDurationActivity.getTime(), "QC_task_Total_duration_activity_whole_run");
    mCollector->send(whole_run_rate, "QC_task_Rate_objects_published_per_second_whole_run");
//  mCollector->send(std::stod(pidStatus[3]), "QC_task_Mean_pcpu_whole_run");
//  mCollector->send(ba::mean(pmems), "QC_task_Mean_pmem_whole_run");

    mCycleNumber++;
    mCycleOn = false;
  }
  // restart timer
  mCycleTimer->async_wait(boost::bind(&TaskDataProcessor::finishCycle, this));
}

unsigned long TaskDataProcessor::publish()
{
  unsigned int sentMessages = 0;

  for (auto& pair : *mObjectsManager) {
    auto* mo = pair.second;
    auto* message = new TMessage(kMESS_OBJECT); // will be deleted by fairmq using our custom method
    message->WriteObjectAny(mo, mo->IsA());
    FairMQMessagePtr msg(mDevice->NewMessage(message->Buffer(), message->BufferSize(), CustomCleanupTMessage, message));
    QcInfoLogger::GetInstance() << "Sending \"" << mo->getName() << "\"" << AliceO2::InfoLogger::InfoLogger::endm;
    mDevice->Send(msg, "data-out");
    sentMessages++;
  }

  return sentMessages;
}

void TaskDataProcessor::CustomCleanupTMessage(void* data, void* object) { delete (TMessage*)object; }

}
}
}
