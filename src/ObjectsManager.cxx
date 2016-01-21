///
/// \file   ObjectsManager.cxx
/// \author Barthelemy von Haller
///

#include "QualityControl/QcInfoLogger.h"
#include "QualityControl/ObjectsManager.h"
#include "QualityControl/MonitorObject.h"
#include "QualityControl/Exceptions.h"
#include "QualityControl/MockPublisher.h"
#include "QualityControl/AlfaPublisher.h"

using namespace AliceO2::QualityControl::Core;

namespace AliceO2 {
namespace QualityControl {
namespace Core {

ObjectsManager::ObjectsManager()
{
  // TODO choose proper backend based on configuration or parameters
//  mBackend = new MockObjectsManagerBackend();
  mBackend = new AlfaPublisher();
}

ObjectsManager::~ObjectsManager()
{
}

void ObjectsManager::startPublishing(std::string objectName, TObject *object)
{
  MonitorObject *newObject = new MonitorObject(objectName, object);
  mMonitorObjects[objectName] = newObject;
}

void ObjectsManager::setQuality(std::string objectName, Quality quality)
{
  MonitorObject *mo = getMonitorObject(objectName);
  mo->setQuality(quality);
}

Quality ObjectsManager::getQuality(std::string objectName)
{
  MonitorObject *mo = getMonitorObject(objectName);
  return mo->getQuality();
}

void ObjectsManager::addChecker(std::string objectName, std::string checkerName, std::string checkerClassName)
{
  MonitorObject *mo = getMonitorObject(objectName);
  mo->addCheck(checkerName, checkerClassName);
}

MonitorObject *ObjectsManager::getMonitorObject(std::string objectName)
{
  if (mMonitorObjects.count(objectName) > 0) {
    return mMonitorObjects[objectName];
  } else {
    BOOST_THROW_EXCEPTION(ObjectNotFoundError() << errinfo_object_name(objectName));
  }
  return nullptr; // we should never reach this point
}

TObject *ObjectsManager::getObject(std::string objectName)
{
  MonitorObject *mo = getMonitorObject(objectName);
  return mo->getObject();
}

void ObjectsManager::publish()
{
for (auto &mo : mMonitorObjects) {
    mBackend->publish(mo.second);
  }
}

} // namespace Core
} // namespace QualityControl
} // namespace AliceO2
