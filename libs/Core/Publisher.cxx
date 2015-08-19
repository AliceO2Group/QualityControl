///
/// \file   Publisher.cxx
/// \author Barthelemy von Haller
///

#include "QcInfoLogger.h"
#include "Publisher.h"
#include "MonitorObject.h"
#include "Exceptions.h"
#include "MockPublisherBackend.h"

using namespace AliceO2::QualityControl::Core;

namespace AliceO2 {
namespace QualityControl {
namespace Core {

Publisher::Publisher()
{
  // TODO choose proper backend based on configuration or parameters
  mBackend = new MockPublisherBackend();
}

Publisher::~Publisher()
{
}

void Publisher::startPublishing(std::string objectName, TObject *object)
{
  MonitorObject *newObject = new MonitorObject(objectName, object);
  mMonitorObjects[objectName] = newObject;
}

void Publisher::setQuality(std::string objectName, Quality quality)
{
  MonitorObject *mo = getMonitorObject(objectName);
  mo->setQuality(quality);
}

Quality Publisher::getQuality(std::string objectName)
{
  MonitorObject *mo = getMonitorObject(objectName);
  return mo->getQuality();
}

void Publisher::addChecker(std::string objectName, std::string checkerName, std::string checkerClassName)
{
  MonitorObject *mo = getMonitorObject(objectName);
  mo->addCheck(checkerName, checkerClassName);
}

MonitorObject *Publisher::getMonitorObject(std::string objectName)
{
  if (mMonitorObjects.count(objectName) > 0) {
    return mMonitorObjects[objectName];
  } else {
    BOOST_THROW_EXCEPTION(ObjectNotFoundError() << errinfo_object_name(objectName));
  }
}

TObject *Publisher::getObject(std::string objectName)
{
  MonitorObject *mo = getMonitorObject(objectName);
  return mo->getObject();
}

void Publisher::publish()
{
  for (auto &mo : mMonitorObjects) {
    mBackend->publish(mo.second);
  }
}

} // namespace Core
} // namespace QualityControl
} // namespace AliceO2
