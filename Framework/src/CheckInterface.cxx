///
/// \file   CheckInterface.cxx
/// \author Barthelemy von Haller
///

#include "QualityControl/CheckInterface.h"

#include <TClass.h>

ClassImp(o2::quality_control::checker::CheckInterface)

  using namespace std;

namespace o2::quality_control::checker
{

std::string CheckInterface::getAcceptedType() { return "TObject"; }

bool CheckInterface::isObjectCheckable(const MonitorObject* mo)
{
  TObject* encapsulated = mo->getObject();

  if (encapsulated->IsA()->InheritsFrom(getAcceptedType().c_str())) {
    return true;
  }

  return false;
}

} // namespace o2::quality_control::checker
