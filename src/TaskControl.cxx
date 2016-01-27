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

TaskControl::TaskControl(std::string taskName, std::string configurationSource) : sampler(0)
{
  AliceO2::InfoLogger::InfoLogger theLog;
  mObjectsManager = new ObjectsManager();
  mConfiguration.load(configurationSource);

  // TODO use a factory to load the proper plugin and class
  string moduleName = mConfiguration.getValue<string>("ExampleTask.moduleName");
  string className = mConfiguration.getValue<string>("ExampleTask.moduleName");
  TaskFactory f;
  mTask = f.create(taskName, moduleName, className, mObjectsManager);
//  mTask = new AliceO2::QualityControl::ExampleModule::ExampleTask(taskName, mPublisher);
}

TaskControl::~TaskControl()
{
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
  Activity activity(mConfiguration.getValue<int>("Activity.number"), mConfiguration.getValue<int>("Activity.type"));
  mTask->startOfActivity(activity);

  // TODO create DataSampling with correct parameters, keep it
  sampler = new AliceO2::DataSampling::MockSampler();
}

void TaskControl::execute()
{
  mTask->startOfCycle();
  DataBlock *block = sampler->getData(0);
  mTask->monitorDataBlock(*block);
  sampler->releaseData();
  mTask->endOfCycle();

  mObjectsManager->publish();

  // TODO get config file from config manager
  // TODO create a member for this collector in order to use it from other methods
  Monitoring::Core::DataCollectorApMon collector(mConfiguration.getValue<string>("Monitoring.pathToConfig"));
//  Monitoring::Core::DataCollector collector;
  collector.sendValue("QC", "QC-barth", "asdf", 10);
}

void TaskControl::stop()
{
  QcInfoLogger::GetInstance() << "stop" << AliceO2::InfoLogger::InfoLogger::endm;
  Activity activity(mConfiguration.getValue<int>("Activity.number"), mConfiguration.getValue<int>("Activity.type"));
  mTask->endOfActivity(activity);
}

}
}
}
