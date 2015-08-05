///
/// \file   TaskControl.cxx
/// \author Barthelemy von Haller
///

#include <unistd.h>
#include "QcInfoLogger.h"
#include "TaskControl.h"

namespace AliceO2 {
namespace QualityControl {
namespace Core {

TaskControl::TaskControl()
{
  AliceO2::InfoLogger::InfoLogger theLog;
}

TaskControl::~TaskControl()
{
}

void TaskControl::initialize()
{
  QcInfoLogger::GetInstance() << "initialize" << AliceO2::InfoLogger::InfoLogger::endm;
}

void TaskControl::configure()
{
  QcInfoLogger::GetInstance() << "configure" << AliceO2::InfoLogger::InfoLogger::endm;
}

void TaskControl::start()
{
  QcInfoLogger::GetInstance() << "start" << AliceO2::InfoLogger::InfoLogger::endm;
}

void TaskControl::execute()
{
  QcInfoLogger::GetInstance() << "execute (5x1s)" << AliceO2::InfoLogger::InfoLogger::endm;
  sleep(1);
  QcInfoLogger::GetInstance() << "   ." << AliceO2::InfoLogger::InfoLogger::endm;
  sleep(1);
  QcInfoLogger::GetInstance() << "   ." << AliceO2::InfoLogger::InfoLogger::endm;
  sleep(1);
  QcInfoLogger::GetInstance() << "   ." << AliceO2::InfoLogger::InfoLogger::endm;
  sleep(1);
  QcInfoLogger::GetInstance() << "   ." << AliceO2::InfoLogger::InfoLogger::endm;
  sleep(1);
  QcInfoLogger::GetInstance() << "   ." << AliceO2::InfoLogger::InfoLogger::endm;
}

void TaskControl::stop()
{
  QcInfoLogger::GetInstance() << "stop" << AliceO2::InfoLogger::InfoLogger::endm;
}

}
}
}