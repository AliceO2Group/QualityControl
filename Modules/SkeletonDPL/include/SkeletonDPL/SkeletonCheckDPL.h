///
/// \file   SkeletonCheckDPL.h
/// \author Piotr Konopka
///

#ifndef QC_MODULE_SKELETONDPL_SKELETONCHECKDPL_H
#define QC_MODULE_SKELETONDPL_SKELETONCHECKDPL_H

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

} // namespace skeleton_dpl
} // namespace quality_control_modules
} // namespace o2

#endif // QC_MODULE_SKELETONDPL_SKELETONCHECKDPL_H
