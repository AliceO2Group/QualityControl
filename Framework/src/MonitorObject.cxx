///
/// \file   MonitorObject.cxx
/// \author Barthelemy von Haller
///

#include "QualityControl/MonitorObject.h"

ClassImp(AliceO2::QualityControl::Core::MonitorObject)

namespace AliceO2 {
namespace QualityControl {
namespace Core {

MonitorObject::MonitorObject()
  : TObject(), mName(""), mObject(nullptr), mQuality(Quality::Null), mTaskName(""), mIsOwner(true)
{
}

MonitorObject::~MonitorObject()
{
  if(mIsOwner && mObject) {
    delete mObject;
  }
}

MonitorObject::MonitorObject(const std::string &name, TObject *object, const std::string &taskName)
  : TObject(), mName(name), mObject(object), mQuality(Quality::Null), mTaskName(taskName), mIsOwner(true)
{

}

} // namespace Core
} // namespace QualityControl
} // namespace AliceO2
