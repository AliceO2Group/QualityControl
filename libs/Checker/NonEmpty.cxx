///
/// \file   NonEmpty.cxx
/// \author flpprotodev
///

#include "NonEmpty.h"

#include "TH1.h"
#include <assert.h>

namespace AliceO2 {
namespace QualityControl {
namespace Checker {

NonEmpty::NonEmpty()
{
  // TODO Auto-generated constructor stub

}

NonEmpty::~NonEmpty()
{
  // TODO Auto-generated destructor stub
}

Quality NonEmpty::check(MonitorObject *mo)
{
  Quality result = Quality::Null;

  // The framework guarantees that the encapsulated object is of the accepted type.
  TH1 *histo = dynamic_cast<TH1*>(mo->getObject());
  assert(histo != nullptr);
  if(histo->GetEntries() > 0) {
    result = Quality::Good;
  } else {
    result = Quality::Bad;
  }

  return result;
}

std::string NonEmpty::getAcceptedType()
{
  return "TH1";
}

} // namespace Checker 
} // namespace QualityControl 
} // namespace AliceO2 

