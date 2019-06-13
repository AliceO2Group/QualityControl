///
/// \file   SkeletonCheck.h
/// \author Piotr Konopka
///

#ifndef QC_MODULE_ITSQCTASK_ITSQCCHECK_H
#define QC_MODULE_ITSQCTASK_ITSQCCHECK_H

#include "QualityControl/CheckInterface.h"
#include "QualityControl/MonitorObject.h"
#include "QualityControl/Quality.h"

namespace o2
{
namespace quality_control_modules
{
namespace itsqctask
{

/// \brief  Check whether a plot is empty or not.
///
/// \author Barthelemy von Haller
class ITSQCCheck : public o2::quality_control::checker::CheckInterface
{
 public:
  /// Default constructor
  ITSQCCheck();
  /// Destructor
  ~ITSQCCheck() override;

  // Override interface
  void configure(std::string name) override;
  Quality check(const MonitorObject* mo) override;
  void beautify(MonitorObject* mo, Quality checkResult = Quality::Null) override;
  std::string getAcceptedType() override;

  ClassDefOverride(ITSQCCheck, 1);
};

} // namespace skeleton
} // namespace quality_control_modules
} // namespace o2

#endif // QC_MODULE_SKELETON_SKELETONCHECK_H
