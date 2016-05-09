///
/// \file   NonEmpty.cxx
/// \author Barthelemy von Haller
///

#include "QualityControl/NonEmpty.h"
#include "TClass.h"
#include <iostream>

ClassImp(AliceO2::QualityControl::Checker::NonEmpty)

using namespace std;

namespace AliceO2 {
namespace QualityControl {
namespace Checker {

NonEmpty::NonEmpty()
{
}

NonEmpty::~NonEmpty()
{
}

std::string NonEmpty::getAcceptedType()
{
  return "TH1";
}

Quality NonEmpty::check(const MonitorObject *mo)
{
  return Quality::Good;
}

void NonEmpty::beautify(MonitorObject *mo, Quality checkResult)
{
  // NOOP
}
} /* namespace Checker */
} /* namespace QualityControl */
} /* namespace AliceO2 */

