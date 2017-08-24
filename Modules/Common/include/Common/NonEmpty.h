///
/// \file   NonEmpty.h
/// \author Barthelemy von Haller
///

#ifndef QUALITYCONTROL_LIBS_CHECKER_NONEMPTY_H_
#define QUALITYCONTROL_LIBS_CHECKER_NONEMPTY_H_

#include "QualityControl/MonitorObject.h"
#include "QualityControl/Quality.h"
#include "QualityControl/CheckInterface.h"

namespace AliceO2 {
namespace QualityControlModules {
namespace Common {

/// \brief  Check whether a plot is empty or not.
///
/// \author Barthelemy von Haller
class NonEmpty : public AliceO2::QualityControl::Checker::CheckInterface
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

} // namespace Checker 
} // namespace QualityControl 
} // namespace AliceO2 

#endif // QUALITYCONTROL_LIBS_CHECKER_NONEMPTY_H_ 
