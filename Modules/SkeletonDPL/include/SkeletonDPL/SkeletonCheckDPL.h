///
/// \file   SkeletonCheck.h
/// \author Barthelemy von Haller
///

#ifndef QUALITYCONTROL_LIBS_CHECKER_SkeletonCheckDPL_H_
#define QUALITYCONTROL_LIBS_CHECKER_SkeletonCheckDPL_H_

#include "QualityControl/MonitorObject.h"
#include "QualityControl/Quality.h"
#include "QualityControl/CheckInterface.h"

namespace o2 {
namespace quality_control_modules {
namespace skeleton_dpl {

/// \brief  Check whether a plot is empty or not.
///
/// \author Barthelemy von Haller
class SkeletonCheckDPL : public o2::quality_control::checker::CheckInterface
{
  public:
    /// Default constructor
    SkeletonCheckDPL();
    /// Destructor
    ~SkeletonCheckDPL() override;

    // Override interface
    void configure(std::string name) override;
    Quality check(const MonitorObject *mo) override;
    void beautify(MonitorObject *mo, Quality checkResult = Quality::Null) override;
    std::string getAcceptedType() override;

  ClassDefOverride(SkeletonCheckDPL, 1);
};

} // namespace Example
} // namespace QualityControl
} // namespace o2

#endif // QUALITYCONTROL_LIBS_CHECKER_SkeletonCheck_H_
