///
/// \file   SkeletonCheck.h
/// \author Piotr Konopka
///

#ifndef QC_MODULE_SKELETON_SKELETONCHECK_H
#define QC_MODULE_SKELETON_SKELETONCHECK_H

#include "QualityControl/CheckInterface.h"
#include "QualityControl/MonitorObject.h"
#include "QualityControl/Quality.h"

namespace o2::quality_control_modules::skeleton
{

/// \brief  Check whether a plot is empty or not.
///
/// \author Barthelemy von Haller
class SkeletonCheck : public o2::quality_control::checker::CheckInterface
{
 public:
  /// Default constructor
  SkeletonCheck();
  /// Destructor
  ~SkeletonCheck() override;

  // Override interface
  void configure(std::string name) override;
  Quality check(const MonitorObject* mo) override;
  void beautify(MonitorObject* mo, Quality checkResult = Quality::Null) override;
  std::string getAcceptedType() override;

  ClassDefOverride(SkeletonCheck, 1);
};

} // namespace o2::quality_control_modules::skeleton

#endif // QC_MODULE_SKELETON_SKELETONCHECK_H
