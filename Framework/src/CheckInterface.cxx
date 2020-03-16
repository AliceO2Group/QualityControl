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

bool CheckInterface::isObjectCheckable(const std::shared_ptr<MonitorObject> mo)
{
  return isObjectCheckable(mo.get());
}

bool CheckInterface::isObjectCheckable(const MonitorObject* mo)
{
  TObject* encapsulated = mo->getObject();

  return encapsulated->IsA()->InheritsFrom(getAcceptedType().c_str());
}

void CheckInterface::setCustomParameters(const std::unordered_map<std::string, std::string>& parameters)
{
  mCustomParameters = parameters;
}

} // namespace o2::quality_control::checker
