///
/// \file   FakeCheck.cxx
/// \author Barthelemy von Haller
///

#include "Example/FakeCheck.h"

// ROOT
#include <TH1.h>

using namespace std;

ClassImp(o2::quality_control_modules::example::FakeCheck)

  namespace o2
{
  namespace quality_control_modules
  {
  namespace example
  {

  FakeCheck::FakeCheck() {}

  FakeCheck::~FakeCheck() {}

  void FakeCheck::configure(std::string name) {}

  Quality FakeCheck::check(const MonitorObject* mo)
  {
    Quality result = Quality::Null;

    return result;
  }

  std::string FakeCheck::getAcceptedType() { return "TH1"; }

  void FakeCheck::beautify(MonitorObject* mo, Quality checkResult)
  {
    // NOOP
  }

  } // namespace example
  } // namespace quality_control_modules
} // namespace o2
