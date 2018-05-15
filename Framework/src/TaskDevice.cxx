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
/// \file   TaskDevice.cxx
/// \author Barthelemy von Haller
///

#include "QualityControl/TaskDevice.h"
#include <TMessage.h>
#include <TClass.h>
#include <Common/Timer.h>
#include <Configuration/ConfigurationFactory.h>
#include <InfoLogger/InfoLogger.hxx>
#include <DataSampling/SamplerFactory.h>
#include "runFairMQDevice.h"
#include "QualityControl/QcInfoLogger.h"
#include "QualityControl/TaskFactory.h"

namespace bpo = boost::program_options;
using namespace std;
using namespace std::chrono;
using namespace o2::configuration;
using namespace o2::monitoring;

/*
 * Runner
 */

void addCustomOptions(bpo::options_description &options)
{
  options.add_options()
    ("name,n", bpo::value<string>()->required(), "Name of the task (required).")
    ("configuration,c", bpo::value<string>()->required(),
     "Configuration source, e.g. \"file:example.ini\" (required).");
}

FairMQDevicePtr getDevice(const FairMQProgOptions &config)
{
  std::string taskName = config.GetValue<std::string>("name");
  std::string configurationSource = config.GetValue<std::string>("configuration");

  return new o2::quality_control::core::TaskDevice(taskName, configurationSource);
}

/*
 * TaskDevice
 */

namespace o2 {
namespace quality_control {
namespace core {

TaskDevice::TaskDevice(std::string taskName, std::string configurationSource) : mTaskName(taskName),
                                                                                mTotalNumberObjectsPublished(0)
{
  // setup configuration
  mConfigFile = ConfigurationFactory::getConfiguration(configurationSource);
  populateConfig(mTaskName);

  // setup monitoring
  mCollector = MonitoringFactory::Get("infologger://");

  // setup publisher
  mObjectsManager = make_shared<ObjectsManager>(mTaskConfig);

  // setup task
  TaskFactory f;
  mTask = f.create<TaskInterface>(mTaskConfig, mObjectsManager);  // TODO could we use unique_ptr ?

  // setup datasampling
  string dataSamplingImplementation = mConfigFile->get<std::string>("DataSampling/implementation").value();
  QcInfoLogger::GetInstance() << "DataSampling implementation is '" << dataSamplingImplementation << "'"
                              << AliceO2::InfoLogger::InfoLogger::endm;
  mSampler = AliceO2::DataSampling::SamplerFactory::create(dataSamplingImplementation);

  // setup device --> It does not work thus we use a json file
//  FairMQChannel histoChannel;
//  histoChannel.UpdateType("pub");
//  histoChannel.UpdateMethod("bind");
//  histoChannel.UpdateAddress(mTaskConfig.address);
//  histoChannel.UpdateSndBufSize(10000);
//  histoChannel.UpdateRcvBufSize(10000);
//  histoChannel.UpdateRateLogging(0);
//  fChannels["data-out"].push_back(histoChannel);
//  cout << "data-out size : " << fChannels["data-out"].size() << endl;
//  cout << "data-out : " << fChannels["data-out"][0].GetAddress() << endl;
}

void TaskDevice::populateConfig(std::string taskName)
{
  std::string prefix = std::string("qc/tasks_config")+taskName;
  string taskDefinitionName = mConfigFile->get<string>(prefix + "/taskDefinition").value();

  mTaskConfig.taskName = taskName;
  mTaskConfig.moduleName = mConfigFile->get<string>(taskDefinitionName + "/moduleName").value();
  mTaskConfig.className = mConfigFile->get<string>(taskDefinitionName + "/className").value();
  mTaskConfig.cycleDurationSeconds = mConfigFile->get<int>(taskDefinitionName + "/cycleDurationSeconds").value_or(10);
  mTaskConfig.maxNumberCycles = mConfigFile->get<int>(taskDefinitionName + "/maxNumberCycles").value_or(-1);
}

void TaskDevice::InitTask()
{
  QcInfoLogger::GetInstance() << "initialize TaskDevice" << AliceO2::InfoLogger::InfoLogger::endm;

  // init user's task
  mTask->initialize();
}

TaskDevice::~TaskDevice()
{
  delete mTask;
}

void TaskDevice::Run()
{
  AliceO2::Common::Timer timer;
  timer.reset(10000000); // 10 s.
  int lastNumberObjects = 0;

  // in the future the start of an activity/run will come from the control
  startOfActivity();

  int cycle = 0;
  while (CheckCurrentState(RUNNING) && (mTaskConfig.maxNumberCycles < 0 || cycle < mTaskConfig.maxNumberCycles)) {
    QcInfoLogger::GetInstance() << "cycle " << cycle << AliceO2::InfoLogger::InfoLogger::endm;
    monitorCycle();
    cycle++;

    // if 10 s we publish stats
    if (timer.isTimeout()) {
      double current = timer.getTime();
      int objectsPublished = (mTotalNumberObjectsPublished-lastNumberObjects);
      lastNumberObjects = mTotalNumberObjectsPublished;
      mCollector->send({objectsPublished/current, "QC_task_Rate_objects_published_per_10_seconds"});
      timer.increment();
    }
  }

  // in the future the end of an activity/run will come from the control
  endOfActivity();
}

void TaskDevice::monitorCycle()
{
  // monitor cycle
  AliceO2::Common::Timer timer;
  mTask->startOfCycle();
  auto start = system_clock::now();
  auto end = start + seconds(mTaskConfig.cycleDurationSeconds);
  int numberBlocks = 0;
  while (system_clock::now() < end) {
    DataSetReference dataSetReference = mSampler->getData(100);
    if (dataSetReference) {
      mTask->monitorDataBlock(dataSetReference);
      mSampler->releaseData(); // invalids the block !!!
      numberBlocks++;
    }
  }
  mTask->endOfCycle();

  // publication
  unsigned long numberObjectsPublished = publish();

  // monitoring metrics
  double durationCycle = timer.getTime();
  timer.reset();
  double durationPublication = timer.getTime();
  mCollector->send({numberBlocks, "QC_task_Numberofblocks_in_cycle"});
  mCollector->send({durationCycle, "QC_task_Module_cycle_duration"});
  mCollector->send({durationPublication, "QC_task_Publication_duration"});
  mCollector->send({(int) numberObjectsPublished,
                   "QC_task_Number_objects_published_in_cycle"}); // cast due to Monitoring accepting only int
  double rate = numberObjectsPublished / (durationCycle + durationPublication);
  mCollector->send({rate, "QC_task_Rate_objects_published_per_second"});
  mTotalNumberObjectsPublished += numberObjectsPublished;
  //std::vector<std::string> pidStatus = mMonitor->getPIDStatus(::getpid());
  //pcpus(std::stod(pidStatus[3]));
  //pmems(std::stod(pidStatus[4]));
  double whole_run_rate = mTotalNumberObjectsPublished / timerTotalDurationActivity.getTime();
  mCollector->send({mTotalNumberObjectsPublished, "QC_task_Total_objects_published_whole_run"});
  mCollector->send({timerTotalDurationActivity.getTime(), "QC_task_Total_duration_activity_whole_run"});
  mCollector->send({whole_run_rate, "QC_task_Rate_objects_published_per_second_whole_run"});
//  mCollector->send({std::stod(pidStatus[3]), "QC_task_Mean_pcpu_whole_run"});
  mCollector->send({ba::mean(pmems), "QC_task_Mean_pmem_whole_run"});
}

unsigned long TaskDevice::publish()
{
  unsigned int sentMessages = 0;

  for (auto &pair : *mObjectsManager) {
    auto *mo = pair.second;
    auto *message = new TMessage(kMESS_OBJECT); // will be deleted by fairmq using our custom method
    message->WriteObjectAny(mo, mo->IsA());
    FairMQMessagePtr msg(NewMessage(message->Buffer(),
                                    message->BufferSize(),
                                    CustomCleanupTMessage,
                                    message));
    QcInfoLogger::GetInstance() << "Sending \"" << mo->getName() << "\"" << AliceO2::InfoLogger::InfoLogger::endm;
    Send(msg, "data-out");
    sentMessages++;
  }

  sendToInformationService(mObjectsManager->getObjectsListString());

  return sentMessages;
}

void TaskDevice::CustomCleanupTMessage(void *data, void *object)
{
  delete (TMessage *) object;
}

void TaskDevice::Reset()
{
  FairMQDevice::Reset();
  mTask->reset();
}

void TaskDevice::startOfActivity()
{
  timerTotalDurationActivity.reset();
  Activity activity(mConfigFile->get<int>("Activity/number").value(), mConfigFile->get<int>("Activity/type").value());
  mTask->startOfActivity(activity);
}

void TaskDevice::endOfActivity()
{
  Activity activity(mConfigFile->get<int>("Activity/number").value(), mConfigFile->get<int>("Activity/type").value());
  mTask->endOfActivity(activity);

  double rate = mTotalNumberObjectsPublished / timerTotalDurationActivity.getTime();
  mCollector->send({rate, "QC_task_Rate_objects_published_per_second_whole_run"});
  mCollector->send({ba::mean(pcpus), "QC_task_Mean_pcpu_whole_run"});
  mCollector->send({ba::mean(pmems), "QC_task_Mean_pmem_whole_run"});
}

void TaskDevice::sendToInformationService(string objectsListString)
{
  string* text = new std::string(mTaskName);
  *text += ":" + objectsListString;
  // todo escape names with a comma or a colon

  // create message object with a pointer to the data buffer,
  // its size,
  // custom deletion function (called when transfer is done),
  // and pointer to the object managing the data buffer
  FairMQMessagePtr msg(NewMessage(const_cast<char*>(text->c_str()),
                                   text->length(),
                                   [](void* /*data*/, void* object) { delete static_cast<string*>(object); },
                                   text));

  LOG(info) << "Sending \"" << *text << "\"";
  LOG(info) << " llength : " << text->length();

  // in case of error or transfer interruption, return false to go to IDLE state
  // successfull transfer will return number of bytes transfered (can be 0 if sending an empty message).
  int ret = Send(msg, "information-service-out");
  if(ret < 0)
  {
      LOG(error) << "Error sending" << endl;
  }

}

}
}
}

