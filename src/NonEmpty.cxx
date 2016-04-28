///
/// \file   NonEmpty.cxx
/// \author flpprotodev
///

#include "QualityControl/NonEmpty.h"

#include "TH1.h"
#include <assert.h>
#include <iostream>

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

Quality NonEmpty::check(const MonitorObject *mo)
{
  Quality result = Quality::Null;

  // The framework guarantees that the encapsulated object is of the accepted type.
  TH1 *histo = dynamic_cast<TH1*>(mo->getObject());
  assert(histo != nullptr);
  if (histo->GetEntries() > 0) {
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

void NonEmpty::beautify(MonitorObject *mo, Quality checkResult)
{
  // We set the background color of the mo to green if the quality returned by the check is good
  // red otherwise (this is a bad example as several calls would overwrite each others)

  if (!this->isObjectCheckable(mo)) {
    cerr << "object not checkable" << endl;
    return;
  }

  TH1* th1 = dynamic_cast<TH1*>(mo->getObject());

  if (checkResult == Quality::Good) {
    th1->SetFillColor(kGreen);
  } else if (checkResult == Quality::Medium) {
    th1->SetFillColor(kOrange);
  } else if (checkResult == Quality::Bad) {
    th1->SetFillColor(kRed);
  } else {
    th1->SetFillColor(kWhite);
  }
}

}  // namespace Checker
}  // namespace QualityControl
} // namespace AliceO2 

