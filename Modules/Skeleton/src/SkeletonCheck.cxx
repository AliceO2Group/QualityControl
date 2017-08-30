///
/// \file   SkeletonCheck.cxx
/// \author Barthelemy von Haller
///

#include "Skeleton/SkeletonCheck.h"

// ROOT
#include <TH1.h>

using namespace std;

ClassImp(o2::quality_control_modules::skeleton::SkeletonCheck)

namespace o2 {
namespace quality_control_modules {
namespace skeleton {

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

} // namespace skeleton
} // namespace quality_control
} // namespace o2

