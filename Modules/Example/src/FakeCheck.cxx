///
/// \file   FakeCheck.cxx
/// \author Barthelemy von Haller
///

#include "Example/FakeCheck.h"

// ROOT
#include <TH1.h>

using namespace std;

ClassImp(AliceO2::QualityControlModules::Example::FakeCheck)

namespace AliceO2 {
namespace QualityControlModules {
namespace Example {

FakeCheck::FakeCheck()
{
}

FakeCheck::~FakeCheck()
{
}

void FakeCheck::configure(std::string name)
{
}

Quality FakeCheck::check(const MonitorObject *mo)
{
  Quality result = Quality::Null;

  return result;
}

std::string FakeCheck::getAcceptedType()
{
  return "TH1";
}

void FakeCheck::beautify(MonitorObject *mo, Quality checkResult)
{
// NOOP
}

}  // namespace Example
}  // namespace QualityControl
} // namespace AliceO2

