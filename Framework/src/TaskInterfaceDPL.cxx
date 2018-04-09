///
/// \file   TaskInterfaceDPL.cxx
/// \author Piotr Konopka
///

#include "QualityControl/TaskInterfaceDPL.h"

namespace o2 {
namespace quality_control {
namespace core {

TaskInterfaceDPL::TaskInterfaceDPL(ObjectsManager *objectsManager) :
  mObjectsManager(objectsManager)
{
}

TaskInterfaceDPL::TaskInterfaceDPL() :
  mObjectsManager(nullptr)
{
}

const std::string &TaskInterfaceDPL::getName() const
{
  return mName;
}

void TaskInterfaceDPL::setName(const std::string &name)
{
  mName = name;
}

void TaskInterfaceDPL::setObjectsManager(std::shared_ptr<ObjectsManager> objectsManager)
{
  mObjectsManager = objectsManager;
}

std::shared_ptr<ObjectsManager> TaskInterfaceDPL::getObjectsManager()
{
  return mObjectsManager;
}

} // namespace core
} // namespace quality_control
} // namespace o2
