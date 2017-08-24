///
/// \file   CheckInterface.h
/// \author Barthelemy von Haller
///

#ifndef QUALITYCONTROL_CHECKER_MEANISABOVE_H_
#define QUALITYCONTROL_CHECKER_MEANISABOVE_H_

#include "QualityControl/MonitorObject.h"
#include "QualityControl/Quality.h"
#include "QualityControl/CheckInterface.h"

using namespace AliceO2::QualityControl::Core;

namespace AliceO2 {
namespace QualityControlModules {

/// Namespace containing all the common, non detector specific, checks.
namespace Common {

/// \brief  Check whether the mean of the plot is above a certain limit.
///
/// \author Barthelemy von Haller
class MeanIsAbove : public AliceO2::QualityControl::Checker::CheckInterface
{
  public:
    /// Default constructor
    MeanIsAbove();
    /// Destructor
    ~MeanIsAbove() override = default;

    void configure(std::string name) override;
    Quality check(const MonitorObject *mo) override;
    void beautify(MonitorObject *mo, Quality checkResult = Quality::Null) override;
    std::string getAcceptedType() override;

  private:

    float mThreshold;

    ClassDefOverride(MeanIsAbove, 1)
};

} /* namespace Common */
} /* namespace QualityControlModules */
} /* namespace AliceO2 */

#endif /* QUALITYCONTROL_CHECKER_MEANISABOVE_H_ */
