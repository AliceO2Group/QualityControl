///
/// \file   CheckInterface.cxx
/// \author Barthelemy von Haller
///

#include "QualityControl/CheckInterface.h"
#include "TClass.h"
#include <iostream>

ClassImp(AliceO2::QualityControl::Checker::CheckInterface)

using namespace std;

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

bool CheckInterface::isObjectCheckable(const MonitorObject *mo)
{
  TObject *encapsulated = mo->getObject();

  if(encapsulated->IsA()->InheritsFrom(getAcceptedType().c_str())) {
    return true;
  }

  return false;
}

} /* namespace Checker */
} /* namespace QualityControl */
} /* namespace AliceO2 */

