///
/// \file   TaskControl.cxx
/// \author Barthelemy von Haller
///

#include <unistd.h>
#include "QcInfoLogger.h"
#include "TaskControl.h"
#include "ExampleTask.h"

namespace AliceO2 {
namespace QualityControl {
namespace Core {

TaskControl::TaskControl(std::string taskName)
{
  AliceO2::InfoLogger::InfoLogger theLog;
  mPublisher = new Publisher();
  // TODO use a factory to load the proper plugin and class
  mTask = new AliceO2::QualityControl::ExampleModule::ExampleTask(taskName, mPublisher);
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
  // TODO get the activity from configuration (?)
  Activity activity(123, 2);
  mTask->startOfActivity(activity);
}

void TaskControl::execute()
{
  mTask->startOfCycle();
  // TODO get the data block from ??
  DataBlock block;
  mTask->monitorDataBlock(block);
  mTask->endOfCycle();
}

void TaskControl::stop()
{
  QcInfoLogger::GetInstance() << "stop" << AliceO2::InfoLogger::InfoLogger::endm;
  // TODO get the activity from configuration (?)
  Activity activity(123, 2);
  mTask->endOfActivity(activity);
}

}
}
}