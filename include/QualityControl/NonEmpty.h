///
/// \file   CheckInterface.h
/// \author Barthelemy von Haller
///

#ifndef QUALITYCONTROL_CHECKER_NONEMPTY_H_
#define QUALITYCONTROL_CHECKER_NONEMPTY_H_

#include "QualityControl/MonitorObject.h"
#include "QualityControl/Quality.h"
#include "QualityControl/CheckInterface.h"

using namespace AliceO2::QualityControl::Core;

namespace AliceO2 {
namespace QualityControl {
namespace Checker {

/// \brief  Check whether a plot is empty or not.
///
/// \author Barthelemy von Haller
class NonEmpty : public CheckInterface
{
  public:
    /// Default constructor
    NonEmpty();
    /// Destructor
    virtual ~NonEmpty();

    virtual void configure(std::string name);
    virtual Quality check(const MonitorObject *mo);
    virtual void beautify(MonitorObject *mo, Quality checkResult = Quality::Null);
    virtual std::string getAcceptedType();

    ClassDef(NonEmpty, 1)
};

} /* namespace Checker */
} /* namespace QualityControl */
} /* namespace AliceO2 */

#endif /* QUALITYCONTROL_CHECKER_NONEMPTY_H_ */
