///
/// \file   Publisher.cxx
/// \author Barthelemy von Haller
///

#include <TCanvas.h>
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

TObject *Publisher::getObject(std::string objectName)
{
  MonitorObject *mo = getMonitorObject(objectName);
  return mo->getObject();
}

void Publisher::publish()
{
  // TODO use a backend to publish to database, file, alfa, etc....
  // just to see it
  if (mMonitorObjects.size() > 0) {
    TCanvas canvas;
    mMonitorObjects[0]->getObject()->Draw();
    canvas.SaveAs("test.jpg");
  }
}

} /* namespace Publisher */
} /* namespace QualityControl */
} /* namespace AliceO2 */
