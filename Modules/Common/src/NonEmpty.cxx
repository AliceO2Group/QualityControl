///
/// \file   NonEmpty.cxx
/// \author Barthelemy von Haller
///

#include "Common/NonEmpty.h"

// ROOT
#include <TH1.h>

using namespace std;

ClassImp(o2::quality_control_modules::common::NonEmpty)

  namespace o2::quality_control_modules::common
{

  NonEmpty::NonEmpty() {}

  NonEmpty::~NonEmpty() {}

  void NonEmpty::configure(std::string name) {}

  Quality NonEmpty::check(const MonitorObject* mo)
  {
    auto result = Quality::Null;

    // The framework guarantees that the encapsulated object is of the accepted type.
    auto* histo = dynamic_cast<TH1*>(mo->getObject());

    // assert(histo != nullptr);
    if (histo != nullptr) {
      if (histo->GetEntries() > 0) {
        result = Quality::Good;
      } else {
        result = Quality::Bad;
      }
    }

    return result;
  }

  std::string NonEmpty::getAcceptedType() { return "TH1"; }

  void NonEmpty::beautify(MonitorObject* mo, Quality checkResult)
  {
    // NOOP
  }

} // namespace daq::quality_control_modules::common
