///
/// \file   SkeletonCheck.h
/// \author Barthelemy von Haller
///

#ifndef QUALITYCONTROL_LIBS_CHECKER_SkeletonCheck_H_
#define QUALITYCONTROL_LIBS_CHECKER_SkeletonCheck_H_

#include "QualityControl/MonitorObject.h"
#include "QualityControl/Quality.h"
#include "QualityControl/CheckInterface.h"

namespace AliceO2 {
namespace QualityControlModules {
namespace Skeleton {

/// \brief  Check whether a plot is empty or not.
///
/// \author Barthelemy von Haller
class SkeletonCheck : public AliceO2::QualityControl::Checker::CheckInterface
{
  public:
    /// Default constructor
    SkeletonCheck();
    /// Destructor
    ~SkeletonCheck() override;

    // Override interface
    void configure(std::string name) override;
    Quality check(const MonitorObject *mo) override;
    void beautify(MonitorObject *mo, Quality checkResult = Quality::Null) override;
    std::string getAcceptedType() override;

  ClassDefOverride(SkeletonCheck, 1);
};

} // namespace Example
} // namespace QualityControl
} // namespace AliceO2

#endif // QUALITYCONTROL_LIBS_CHECKER_SkeletonCheck_H_
