///
/// \file   TaskFactory.cxx
/// \author Barthelemy von Haller
///

#include "QualityControl/TaskFactory.h"
#include "QualityControl/ExampleTask.h" // todo : remove
#include "QualityControl/TaskInterface.h"

namespace AliceO2 {
namespace QualityControl {
namespace Core {

TaskFactory::TaskFactory()
{
}

TaskFactory::~TaskFactory()
{
}

TaskInterface* TaskFactory::create(std::string taskName, std::string moduleName, std::string className,
    Publisher* publisher)
{
  TaskInterface *result = 0;

  // TODO some magic to load the proper library and class
  result = new AliceO2::QualityControl::ExampleModule::ExampleTask(taskName, publisher);

  return result;
}

} // namespace Core
} // namespace QualityControl
} // namespace AliceO2
