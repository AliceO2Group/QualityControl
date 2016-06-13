///
/// \file   ObjectsManager.cxx
/// \author Barthelemy von Haller
///

#include "Common/Exceptions.h"
#include "QualityControl/ObjectsManager.h"
#include "QualityControl/MonitorObject.h"
#include "QualityControl/MockPublisher.h"
#include "QualityControl/AlfaPublisher.h"

using namespace AliceO2::QualityControl::Core;
using namespace AliceO2::Common;

namespace AliceO2 {
namespace QualityControl {
namespace Core {

ObjectsManager::ObjectsManager()
{
  mPublisher = new MockPublisher();
}

ObjectsManager::ObjectsManager(std::string publisherClassName)
{
  // We don't dynamically look for the class using TROOT and TSystem because we will
  // extremly rarely add a new publisher backend. It is not worth the trouble.
  if (publisherClassName == "MockPublisher") {
    mPublisher = new MockPublisher();
  } else if (publisherClassName == "AlfaPublisher") {
    mPublisher = new AlfaPublisher();
  } else {
    BOOST_THROW_EXCEPTION(FatalException() << errinfo_details("Unknown publisher class : " + publisherClassName));
  }
}

ObjectsManager::~ObjectsManager()
{
  delete mPublisher;
  for (auto obj = mMonitorObjects.begin(); obj != mMonitorObjects.end(); obj++) {
    delete obj->second;
  }
  mMonitorObjects.clear();
}

void ObjectsManager::startPublishing(std::string taskName, std::string objectName, TObject *object) // TODO consider making taskname a property that we set on creation
{
  MonitorObject *newObject = new MonitorObject(objectName, object, taskName);
  newObject->setIsOwner(false);
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

void ObjectsManager::addCheck(const std::string objectName, const std::string checkName,
    const std::string checkClassName, const std::string checkLibraryName)
{
  MonitorObject *mo = getMonitorObject(objectName);
  mo->addCheck(checkName, checkClassName, checkLibraryName);
  std::cout << "Added check : " << objectName << " , " << checkName << " , " << checkClassName << " , " << checkLibraryName << std::endl;
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
    mPublisher->publish(mo.second);
  }
}

} // namespace Core
} // namespace QualityControl
} // namespace AliceO2
