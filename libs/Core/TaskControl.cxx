///
/// \file   TaskControl.cxx
/// \author Barthelemy von Haller
///

#include "QcInfoLogger.h"
#include "TaskControl.h"
#include "ExampleTask.h"
#include <DataSampling/MockSampler.h>
#include "TaskFactory.h"

namespace AliceO2 {
namespace QualityControl {
namespace Core {

TaskControl::TaskControl(std::string taskName, std::string configurationSource) : sampler(0)
{
  AliceO2::InfoLogger::InfoLogger theLog;
  mPublisher = new Publisher();
  mConfiguration.load(configurationSource);

  // TODO use a factory to load the proper plugin and class
  string moduleName = mConfiguration.getValue<string>("ExampleTask.moduleName");
  string className = mConfiguration.getValue<string>("ExampleTask.moduleName");
  TaskFactory f;
  mTask = f.create(taskName, moduleName, className, mPublisher);
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

  mPublisher->publish();
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
