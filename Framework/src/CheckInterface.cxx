///
/// \file   CheckInterface.cxx
/// \author Barthelemy von Haller
///

#include "QualityControl/CheckInterface.h"
#include "TClass.h"
#include <iostream>

ClassImp(o2::quality_control::checker::CheckInterface)

using namespace std;

namespace o2 {
namespace quality_control {
namespace checker {

//CheckInterface::CheckInterface()
////    : mName("")
//{
//}
//
//CheckInterface::~CheckInterface()
//{
//}

//void CheckInterface::configure(std::string name)
//{
//  mName = name;
//}

std::string CheckInterface::getAcceptedType()
{
  return "TObject";
}

bool CheckInterface::isObjectCheckable(const MonitorObject *mo)
{
  TObject *encapsulated = mo->getObject();

  if (encapsulated->IsA()->InheritsFrom(getAcceptedType().c_str())) {
    return true;
  }

  return false;
}

} /* namespace Checker */
} /* namespace quality_control */
} /* namespace o2 */

