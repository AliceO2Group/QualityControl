///
/// \file   MonitorObject.cxx
/// \author Barthelemy von Haller
///

#include "QualityControl/MonitorObject.h"

ClassImp(o2::quality_control::core::MonitorObject)

namespace o2 {
namespace quality_control {
namespace core {

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

} // namespace core
} // namespace quality_control
} // namespace o2
