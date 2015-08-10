///
/// \file   MonitorObject.cxx
/// \author Barthelemy von Haller
///

#include "MonitorObject.h"

namespace AliceO2 {
namespace QualityControl {
namespace Core {

MonitorObject::MonitorObject()
  : mName(""), mObject(nullptr), mQuality(Quality::Null)
{
}

MonitorObject::~MonitorObject()
{
}

MonitorObject::MonitorObject(const std::string &name, TObject *object)
  : mName(name), mObject(object), mQuality(Quality::Null)
{

}

} /* namespace Core */
} /* namespace QualityControl */
} /* namespace AliceO2 */
