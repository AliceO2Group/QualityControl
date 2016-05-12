///
/// \file   TaskInterface.cxx
/// \author Barthelemy von Haller
///

#include "QualityControl/TaskInterface.h"

namespace AliceO2 {
namespace QualityControl {
namespace Core {

TaskInterface::TaskInterface(/*std::string &name, */ObjectsManager *objectsManager) :
  /*mName(name), */mObjectsManager(objectsManager)
{
}

TaskInterface::TaskInterface() :
  /*mName(""), */mObjectsManager(nullptr)
{
}

//void TaskInterface::Init(/*std::string &name, */ObjectsManager *objectsManager)
//{
////  setName(name);
//  setObjectsManager(objectsManager);
//}

TaskInterface::~TaskInterface()
{
}

//const std::string &TaskInterface::getName() const
//{
//  return mName;
//}
//
//void TaskInterface::setName(const std::string &name)
//{
//  mName = name;
//}

void TaskInterface::setObjectsManager(ObjectsManager *objectsManager)
{
  mObjectsManager = objectsManager;
}


ObjectsManager *TaskInterface::getObjectsManager()
{
  return mObjectsManager;
}

} // namespace Core
} // namespace QualityControl
} // namespace AliceO2
