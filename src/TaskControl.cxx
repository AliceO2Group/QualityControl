///
/// \file   TaskControl.cxx
/// \author Barthelemy von Haller
///

#include "QualityControl/QcInfoLogger.h"
#include "QualityControl/TaskControl.h"
#include <DataSampling/MockSampler.h>
#include "QualityControl/TaskFactory.h"

using namespace std;
using namespace std::chrono;

namespace AliceO2 {
namespace QualityControl {
namespace Core {

TaskControl::TaskControl(std::string taskName, std::string configurationSource)
  : mSampler(nullptr), mCollector(nullptr), mCycleDurationSeconds(5)
{
  mConfigFile.load(configurationSource);
  populateConfig(taskName);

  // setup publisher
  mObjectsManager = new ObjectsManager(mTaskConfig);

  // setup task
  TaskFactory f;
  mTask = f.create(mTaskConfig, mObjectsManager);  // TODO could we use unique_ptr ?

  // setup monitoring
  mCollector = new Monitoring::Core::Collector(mConfigFile);

  // TODO create DataSampling with correct parameters
  mSampler = new AliceO2::DataSampling::MockSampler();
}

TaskControl::~TaskControl()
{
  if (mSampler) {
    delete mSampler;
  }
  if (mCollector) {
    delete mCollector;
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
  // todo measure duration of monitor cycle and publication
  mTask->startOfCycle();
  auto start = system_clock::now();
  auto end = start + seconds(mCycleDurationSeconds);
  int numberBlocks = 0;
  while (system_clock::now() < end) {
//    cout << "now : " << system_clock::to_time_t(system_clock::now()) << endl;
//    cout << "end : " << system_clock::to_time_t(end) << endl;
    DataBlock *block = mSampler->getData(0);
    mTask->monitorDataBlock(*block);
    mSampler->releaseData(); // invalids the block !!!
    numberBlocks++;
  }
  mTask->endOfCycle();

  mObjectsManager->publish();

  mCollector->send(numberBlocks, "QC_numberofblocks_in_cycle");
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
