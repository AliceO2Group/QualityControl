///
/// \file   NonEmpty.h
/// \author Barthelemy von Haller
///

#ifndef QC_MODULE_COMMON_NONEMPTY_H
#define QC_MODULE_COMMON_NONEMPTY_H

#include "QualityControl/MonitorObject.h"
#include "QualityControl/Quality.h"
#include "QualityControl/CheckInterface.h"

namespace o2 {
namespace quality_control_modules {
namespace common {

/// \brief  Check whether a plot is empty or not.
///
/// \author Barthelemy von Haller
class NonEmpty : public o2::quality_control::checker::CheckInterface
{
  public:
    /// Default constructor
    NonEmpty();
    /// Destructor
    ~NonEmpty() override;

    void configure(std::string name) override;
    Quality check(const MonitorObject *mo) override;
    void beautify(MonitorObject *mo, Quality checkResult = Quality::Null) override;
    std::string getAcceptedType() override;

    ClassDefOverride(NonEmpty,1);
};

} // namespace common
} // namespace quality_control_modules
} // namespace o2

#endif // QC_MODULE_COMMON_NONEMPTY_H
