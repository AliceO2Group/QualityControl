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
  : TObject(), mName(""), mObject(nullptr), mQuality(Quality::Null)
{
}

MonitorObject::~MonitorObject()
{
}

MonitorObject::MonitorObject(const std::string &name, TObject *object)
  : TObject(), mName(name), mObject(object), mQuality(Quality::Null)
{

}

} // namespace Core
} // namespace QualityControl
} // namespace AliceO2
