///
/// \file   TaskControl.cxx
/// \author Barthelemy von Haller
///

//#include <DataSampling/FairSampler.h>
#include "QualityControl/QcInfoLogger.h"
#include "QualityControl/TaskControl.h"
#include "DataSampling/SamplerFactory.h"
#include "DataSampling/SamplerInterface.h"
#include "QualityControl/TaskFactory.h"

using namespace std;
using namespace std::chrono;

namespace AliceO2 {
namespace QualityControl {
namespace Core {

TaskControl::TaskControl(std::string taskName, std::string configurationSource)
  : mSampler(nullptr),
    mTotalNumberObjectsPublished(0)
{
  // configuration
  mConfigFile.load(configurationSource);
  populateConfig(taskName);

  // monitoring
  mCollector = std::shared_ptr<Monitoring::Collector>(new Monitoring::Collector(configurationSource));
//  mMonitor = std::unique_ptr<Monitoring::ProcessMonitor>(new Monitoring::ProcessMonitor(mCollector, mConfigFile));

  // setup publisher
  mObjectsManager = new ObjectsManager(mTaskConfig);

  // setup task
  TaskFactory f;
  mTask = f.create(mTaskConfig, mObjectsManager);  // TODO could we use unique_ptr ?

  // TODO create DataSampling with correct parameters
  string dataSamplingImplementation = mConfigFile.getValue<string>("DataSampling.implementation");
  QcInfoLogger::GetInstance() << "DataSampling implementation is '" << dataSamplingImplementation << "'" << AliceO2::InfoLogger::InfoLogger::endm;
  mSampler = AliceO2::DataSampling::SamplerFactory::create(dataSamplingImplementation);
}

TaskControl::~TaskControl()
{
  if (mSampler) {
    delete mSampler;
  }
  delete mTask;
  delete mObjectsManager;
}

void TaskControl::populateConfig(std::string taskName)
{
  string taskDefinitionName = mConfigFile.getValue<string>(taskName + ".taskDefinition");

  mTaskConfig.taskName = taskName;
  mTaskConfig.moduleName = mConfigFile.getValue<string>(taskDefinitionName + ".moduleName");
  mTaskConfig.address = mConfigFile.getValue<string>(taskName + ".address");
  mTaskConfig.numberHistos = mConfigFile.getValue<int>(taskDefinitionName + ".numberHistos");
  mTaskConfig.numberChecks = mConfigFile.getValue<int>(taskDefinitionName + ".numberChecks");
  mTaskConfig.typeOfChecks = mConfigFile.getValue<string>(taskDefinitionName + ".typeOfChecks");
  mTaskConfig.className = mConfigFile.getValue<string>(taskDefinitionName + ".className");
  mTaskConfig.cycleDurationSeconds = mConfigFile.getValue<int>(taskDefinitionName + ".cycleDurationSeconds");
  mTaskConfig.publisherClassName = mConfigFile.getValue<string>("Publisher.className");
}

void TaskControl::initialize()
{
    QcInfoLogger::GetInstance() << "initialize TaskControl" << AliceO2::InfoLogger::InfoLogger::endm;
    mTask->initialize();
}

void TaskControl::configure()
{
  QcInfoLogger::GetInstance() << "configure" << AliceO2::InfoLogger::InfoLogger::endm;
}

void TaskControl::start()
{
  QcInfoLogger::GetInstance() << "start" << AliceO2::InfoLogger::InfoLogger::endm;
  timerTotalDurationActivity.reset();
  Activity activity(mConfigFile.getValue<int>("Activity.number"), mConfigFile.getValue<int>("Activity.type"));
  mTask->startOfActivity(activity);
}

void TaskControl::execute()
{
  // monitor cycle
  AliceO2::Common::Timer timer;
  mTask->startOfCycle();
  auto start = system_clock::now();
  auto end = start + seconds(mTaskConfig.cycleDurationSeconds);
  int numberBlocks = 0;
  while (system_clock::now() < end) {
    std::vector<std::shared_ptr<DataBlockContainer>> *block = mSampler->getData(100);
    if(block) {
    mTask->monitorDataBlock(*block);
    mSampler->releaseData(); // invalids the block !!!
    numberBlocks++;
  }
  }
  mTask->endOfCycle();
  double durationCycle = timer.getTime();
  timer.reset();

  // publication
  unsigned long numberObjectsPublished = mObjectsManager->publish();

  // monitoring metrics
  double durationPublication = timer.getTime();
  mCollector->send(numberBlocks, "QC_task_Numberofblocks_in_cycle");
  mCollector->send(durationCycle, "QC_task_Module_cycle_duration");
  mCollector->send(durationPublication, "QC_task_Publication_duration");
  mCollector->send((int)numberObjectsPublished, "QC_task_Number_objects_published_in_cycle"); // cast due to Monitoring accepting only int
  double rate = numberObjectsPublished / (durationCycle + durationPublication);
  mCollector->send(rate, "QC_task_Rate_objects_published_per_second");
  mTotalNumberObjectsPublished += numberObjectsPublished;
  //std::vector<std::string> pidStatus = mMonitor->getPIDStatus(::getpid());
  //pcpus(std::stod(pidStatus[3]));
  //pmems(std::stod(pidStatus[4]));
  double whole_run_rate = mTotalNumberObjectsPublished / timerTotalDurationActivity.getTime();
  mCollector->send(mTotalNumberObjectsPublished, "QC_task_Total_objects_published_whole_run");
  mCollector->send(timerTotalDurationActivity.getTime(), "QC_task_Total_duration_activity_whole_run");
  mCollector->send(whole_run_rate, "QC_task_Rate_objects_published_per_second_whole_run");
//  mCollector->send(std::stod(pidStatus[3]), "QC_task_Mean_pcpu_whole_run");
  mCollector->send(ba::mean(pmems), "QC_task_Mean_pmem_whole_run");
}

void TaskControl::stop()
{
  QcInfoLogger::GetInstance() << "stop" << AliceO2::InfoLogger::InfoLogger::endm;
  Activity activity(mConfigFile.getValue<int>("Activity.number"), mConfigFile.getValue<int>("Activity.type"));
  mTask->endOfActivity(activity);

  double rate = mTotalNumberObjectsPublished / timerTotalDurationActivity.getTime();
  mCollector->send(rate, "QC_task_Rate_objects_published_per_second_whole_run");
  mCollector->send(ba::mean(pcpus), "QC_task_Mean_pcpu_whole_run");
  mCollector->send(ba::mean(pmems), "QC_task_Mean_pmem_whole_run");
}

}
}
}
