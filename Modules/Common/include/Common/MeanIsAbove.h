///
/// \file   CheckInterface.h
/// \author Barthelemy von Haller
///

#ifndef QUALITYCONTROL_CHECKER_MEANISABOVE_H_
#define QUALITYCONTROL_CHECKER_MEANISABOVE_H_

#include "QualityControl/MonitorObject.h"
#include "QualityControl/Quality.h"
#include "QualityControl/CheckInterface.h"

using namespace o2::quality_control::core;

namespace o2 {
namespace quality_control_modules {

/// Namespace containing all the common, non detector specific, checks.
namespace common {

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
