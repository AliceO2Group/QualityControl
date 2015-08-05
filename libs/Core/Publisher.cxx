///
/// \file   Publisher.cxx
/// \author Barthelemy von Haller
///

#include "QcInfoLogger.h"
#include "Publisher.h"
#include "MonitorObject.h"
#include "Exceptions.h"

using namespace AliceO2::QualityControl::Core;

namespace AliceO2 {
namespace QualityControl {
namespace Core {

Publisher::Publisher()
{
}

Publisher::~Publisher()
{
}

void Publisher::startPublishing(std::string objectName, void *object)
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
  mo->addChecker(checkerName, checkerClassName);
}

MonitorObject *Publisher::getMonitorObject(std::string objectName)
{
  if (mMonitorObjects.count(objectName) > 0) {
    return mMonitorObjects[objectName];
  } else {
    BOOST_THROW_EXCEPTION(ObjectNotFoundError() << errinfo_object_name(objectName));
  }
}

void *Publisher::getObject(std::string objectName)
{
  MonitorObject *mo = getMonitorObject(objectName);
  return mo->getObject();
}

} /* namespace Publisher */
} /* namespace QualityControl */
} /* namespace AliceO2 */
