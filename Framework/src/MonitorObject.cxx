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
  : TObject(), mName(""), mQuality(Quality::Null), mObject(nullptr), mTaskName(""), mIsOwner(true)
{
}

MonitorObject::~MonitorObject()
{
  if(mIsOwner && mObject) {
    delete mObject;
  }
}

MonitorObject::MonitorObject(const std::string &name, TObject *object, const std::string &taskName)
  : TObject(), mName(name), mQuality(Quality::Null), mObject(object), mTaskName(taskName), mIsOwner(true)
{

}

} // namespace core
} // namespace quality_control
} // namespace o2
