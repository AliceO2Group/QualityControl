///
/// \file   CheckInterface.cxx
/// \author Barthelemy von Haller
///

#include "QualityControl/CheckInterface.h"
#include "TClass.h"

namespace AliceO2 {
namespace QualityControl {
namespace Checker {

CheckInterface::CheckInterface()
{
}

CheckInterface::~CheckInterface()
{
}

std::string CheckInterface::getAcceptedType()
{
  return "TObject";
}

bool CheckInterface::isObjectCheckable(MonitorObject *mo)
{
  TObject *encapsulated = mo->getObject();

  if(encapsulated->Class()->InheritsFrom(getAcceptedType().c_str())) {
    return true;
  }

  return false;
}

} /* namespace Checker */
} /* namespace QualityControl */
} /* namespace AliceO2 */

