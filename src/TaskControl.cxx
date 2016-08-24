///
/// \file   TaskControl.cxx
/// \author Barthelemy von Haller
///

#include "QualityControl/QcInfoLogger.h"
#include "QualityControl/TaskControl.h"
#include "DataSampling/MockSampler.h"
#include "Common/Timer.h"
#include "QualityControl/TaskFactory.h"

using namespace std;
using namespace std::chrono;

namespace AliceO2 {
namespace QualityControl {
namespace Core {

TaskControl::TaskControl(std::string taskName, std::string configurationSource)
  : mSampler(nullptr),
    mCycleDurationSeconds(5), mTotalNumberObjectsPublished(0)
{
  // configuration
  mConfigFile.load(configurationSource);
  populateConfig(taskName);

  // monitoring
  mCollector = std::shared_ptr<Monitoring::Core::Collector>(new Monitoring::Core::Collector(mConfigFile));
  mMonitor = std::unique_ptr<Monitoring::Core::ProcessMonitor>(
    new Monitoring::Core::ProcessMonitor(mCollector, mConfigFile));
  //mMonitor->startMonitor();

  // setup publisher
  mObjectsManager = new ObjectsManager(mTaskConfig);

  // setup task
  TaskFactory f;
  mTask = f.create(mTaskConfig, mObjectsManager);  // TODO could we use unique_ptr ?

  // TODO create DataSampling with correct parameters
  mSampler = new AliceO2::DataSampling::MockSampler();
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
  QcInfoLogger::GetInstance() << "initialize" << AliceO2::InfoLogger::InfoLogger::endm;

  mTask->initialize();
}

void TaskControl::configure()
{
  QcInfoLogger::GetInstance() << "configure" << AliceO2::InfoLogger::InfoLogger::endm;
}

void TaskControl::start()
{
  QcInfoLogger::GetInstance() << "start" << AliceO2::InfoLogger::InfoLogger::endm;
  Activity activity(mConfigFile.getValue<int>("Activity.number"), mConfigFile.getValue<int>("Activity.type"));
  mTask->startOfActivity(activity);
}

void TaskControl::execute()
{
  // monitor cycle
  AliceO2::Common::Timer timer;
  mTask->startOfCycle();
  auto start = system_clock::now();
  auto end = start + seconds(mCycleDurationSeconds);
  int numberBlocks = 0;
  while (system_clock::now() < end) {
    DataBlock *block = mSampler->getData(0);
    mTask->monitorDataBlock(*block);
    mSampler->releaseData(); // invalids the block !!!
    numberBlocks++;
  }
  mTask->endOfCycle();
  double durationCycle = timer.getTime();
  timer.reset();

  // publication
  int numberObjectsPublished = mObjectsManager->publish();

  // monitoring metrics
  double durationPublication = timer.getTime();
  mCollector->send(numberBlocks, "QC_numberofblocks_in_cycle");
  mCollector->send(durationCycle, "Module's cycle duration");
  mCollector->send(durationPublication, "Publication duration");
  mCollector->send(numberObjectsPublished, "Number of objects published");
  double rate = numberObjectsPublished / (durationCycle + durationPublication);
  mCollector->send(rate, "Number of objects published");
  mTotalNumberObjectsPublished+=numberObjectsPublished;
}

void TaskControl::stop()
{
  QcInfoLogger::GetInstance() << "stop" << AliceO2::InfoLogger::InfoLogger::endm;
  Activity activity(mConfigFile.getValue<int>("Activity.number"), mConfigFile.getValue<int>("Activity.type"));
  mTask->endOfActivity(activity);
}

}
}
}
