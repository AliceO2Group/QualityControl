///
/// \file   TaskInterface.cxx
/// \author Barthelemy von Haller
///

#include <utility>

#include "QualityControl/TaskInterface.h"

namespace AliceO2 {
namespace QualityControl {
namespace Core {

TaskInterface::TaskInterface(ObjectsManager *objectsManager) :
  mObjectsManager(objectsManager)
{
}

TaskInterface::TaskInterface() :
  mObjectsManager(nullptr)
{
}

const std::string &TaskInterface::getName() const
{
  return mName;
}

void TaskInterface::setName(const std::string &name)
{
  mName = name;
}

void TaskInterface::setObjectsManager(std::shared_ptr<ObjectsManager> objectsManager)
{
  mObjectsManager = objectsManager;
}

std::shared_ptr<ObjectsManager> TaskInterface::getObjectsManager()
{
  return mObjectsManager;
}

} // namespace Core
} // namespace QualityControl
} // namespace AliceO2
