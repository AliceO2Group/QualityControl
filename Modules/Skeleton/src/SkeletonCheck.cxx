///
/// \file   SkeletonCheck.cxx
/// \author Barthelemy von Haller
///

#include "Skeleton/SkeletonCheck.h"

// ROOT
#include <TH1.h>

using namespace std;

ClassImp(AliceO2::QualityControlModules::Skeleton::SkeletonCheck)

namespace AliceO2 {
namespace QualityControlModules {
namespace Skeleton {

SkeletonCheck::SkeletonCheck()
{
}

SkeletonCheck::~SkeletonCheck()
{
}

void SkeletonCheck::configure(std::string name)
{
}

Quality SkeletonCheck::check(const MonitorObject *mo)
{
  Quality result = Quality::Null;

  return result;
}

std::string SkeletonCheck::getAcceptedType()
{
  return "TH1";
}

void SkeletonCheck::beautify(MonitorObject *mo, Quality checkResult)
{
// NOOP
}

} // namespace Skeleton
} // namespace QualityControl
} // namespace AliceO2

