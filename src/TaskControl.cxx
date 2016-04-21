///
/// \file   TaskControl.cxx
/// \author Barthelemy von Haller
///

#include "QualityControl/QcInfoLogger.h"
#include "QualityControl/TaskControl.h"
#include "QualityControl/ExampleTask.h"
#include <DataSampling/MockSampler.h>
#include "QualityControl/TaskFactory.h"
#include <Monitoring/DataCollectorApMon.h>

namespace Monitoring = AliceO2::Monitoring;

namespace AliceO2 {
namespace QualityControl {
namespace Core {

TaskControl::TaskControl(std::string taskName, std::string configurationSource) : mSampler(0)
{
  AliceO2::InfoLogger::InfoLogger theLog;
  mObjectsManager = new ObjectsManager();
  mConfigFile.load(configurationSource);

  // TODO could we use unique_ptr ?
  string moduleName = mConfigFile.getValue<string>("ExampleTask.moduleName");
  string className = mConfigFile.getValue<string>("ExampleTask.moduleName");
  TaskFactory f;
  mTask = f.create(taskName, moduleName, className, mObjectsManager);
  //mCollector = new Monitoring::Core::DataCollectorApMon(mConfigFile.getValue<string>("Monitoring.pathToConfig"));
  mCollector = Monitoring::Core::Collector();
  // TODO create DataSampling with correct parameters
  mSampler = new AliceO2::DataSampling::MockSampler();
}

TaskControl::~TaskControl()
{
  delete mSampler;
  delete mCollector;
  delete mTask;
  delete mObjectsManager;
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
  mTask->startOfCycle();
  DataBlock *block = mSampler->getData(0);
  mTask->monitorDataBlock(*block);
  mTask->endOfCycle();

  mObjectsManager->publish();

  // TODO the cast to int is WRONG ! change when new monitoring interface is available
  mCollector->send("QC", "QC-barth", "dataBlockSize", (int)block->header.dataSize);

  mSampler->releaseData(); // invalids the block !!!

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
