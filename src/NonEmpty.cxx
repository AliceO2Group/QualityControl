///
/// \file   NonEmpty.cxx
/// \author Barthelemy von Haller
///

#include "QualityControl/NonEmpty.h"
#include "TClass.h"
#include <iostream>
#include <TH1.h>

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

void NonEmpty::configure(std::string name)
{
  // First call the parent !
  CheckInterface::configure(name);
}

std::string NonEmpty::getAcceptedType()
{
  return "TH1";
}

Quality NonEmpty::check(const MonitorObject *mo)
{
  TH1 *th1 = dynamic_cast<TH1*>(mo->getObject());
   if(!th1) {
     // TODO
     return Quality::Null;
   }
   cout << "nb entries : " << th1->GetEntries() << endl;
   if(th1->GetEntries() > 4) {
     return Quality::Good;
   }
   return Quality::Bad;
}

void NonEmpty::beautify(MonitorObject *mo, Quality checkResult)
{
  // NOOP
}
} /* namespace Checker */
} /* namespace QualityControl */
} /* namespace AliceO2 */

