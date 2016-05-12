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
  mBackend = new MockPublisher();
}

ObjectsManager::ObjectsManager(std::string publisherClassName)
{
  // We don't dynamically look for the class using TROOT and TSystem because we will
  // extremly rarely add a new publisher backend. It is not worth the trouble.
  if (publisherClassName == "MockPublisher") {
    mBackend = new MockPublisher();
  } else if (publisherClassName == "AlfaPublisher") {
    mBackend = new AlfaPublisher();
  } else {
    BOOST_THROW_EXCEPTION(FatalException() << errinfo_details("Unknown publisher class : " + publisherClassName));
  }
}

ObjectsManager::~ObjectsManager()
{
  delete mBackend;
  for (auto obj = mMonitorObjects.begin(); obj != mMonitorObjects.end(); obj++) {
    delete obj->second;
  }
  mMonitorObjects.clear();
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
    mBackend->publish(mo.second);
  }
}

} // namespace Core
} // namespace QualityControl
} // namespace AliceO2
