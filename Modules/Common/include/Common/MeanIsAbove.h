///
/// \file   CheckInterface.h
/// \author Barthelemy von Haller
///

#ifndef QC_MODULE_COMMON_MEANISABOVE_H
#define QC_MODULE_COMMON_MEANISABOVE_H

#include "QualityControl/CheckInterface.h"
#include "QualityControl/MonitorObject.h"
#include "QualityControl/Quality.h"

using namespace o2::quality_control::core;

namespace o2::quality_control_modules::common
{

/// \brief  Check whether the mean of the plot is above a certain limit.
///
/// \author Barthelemy von Haller
class MeanIsAbove : public o2::quality_control::checker::CheckInterface
{
 public:
  /// Default constructor
  MeanIsAbove();
  /// Destructor
  ~MeanIsAbove() override = default;

  void configure(std::string name) override;
  Quality check(const MonitorObject* mo) override;
  void beautify(MonitorObject* mo, Quality checkResult = Quality::Null) override;
  std::string getAcceptedType() override;

 private:
  float mThreshold;

  ClassDefOverride(MeanIsAbove, 1)
};

} // namespace o2::quality_control_modules::common

#endif /* QC_MODULE_COMMON_MEANISABOVE_H */
