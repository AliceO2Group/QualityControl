///
/// \file   FakeCheck.h
/// \author Barthelemy von Haller
///

#ifndef QC_MODULE_EXAMPLE_FAKECHECK_H
#define QC_MODULE_EXAMPLE_FAKECHECK_H

#include "QualityControl/MonitorObject.h"
#include "QualityControl/Quality.h"
#include "QualityControl/CheckInterface.h"

namespace o2 {
namespace quality_control_modules {
namespace example {

/// \brief  Check whether a plot is empty or not.
///
/// \author Barthelemy von Haller
class FakeCheck : public o2::quality_control::checker::CheckInterface
{
  public:
    /// Default constructor
    FakeCheck();
    /// Destructor
    ~FakeCheck() override;

    void configure(std::string name) override;
    Quality check(const MonitorObject *mo) override;
    void beautify(MonitorObject *mo, Quality checkResult = Quality::Null) override;
    std::string getAcceptedType() override;

    ClassDefOverride(FakeCheck,1);
};

} // namespace example
} // namespace quality_control_modules
} // namespace o2

#endif // QC_MODULE_EXAMPLE_FAKECHECK_H
