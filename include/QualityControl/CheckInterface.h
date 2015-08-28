///
/// \file   CheckInterface.h
/// \author Barthelemy von Haller
///

#ifndef QUALITYCONTROL_LIBS_CHECKER_CHECKER_H_
#define QUALITYCONTROL_LIBS_CHECKER_CHECKER_H_

#include "QualityControl/MonitorObject.h"
#include "QualityControl/Quality.h"

using namespace AliceO2::QualityControl::Core;

namespace AliceO2 {
namespace QualityControl {
namespace Checker {

/// \brief  Skeleton of a check.
///
/// \author Barthelemy von Haller
class CheckInterface
{
  public:
    /// Default constructor
    CheckInterface();
    /// Destructor
    virtual ~CheckInterface();

    virtual Quality check(MonitorObject *mo) = 0;

    /// \brief Returns the name of the class that can be treated by this check.
    ///
    /// The name of the class returned by this method will be checked against the MonitorObject's encapsulated
    /// object's class. If it is the same or a parent then the check will be applied. Therefore, this method
    /// must return the highest class in the hierarchy that this check can use.
    /// If the class does not override it, we return "TObject".
    ///
    /// \author Barthelemy von Haller
    virtual std::string getAcceptedType();

    bool isObjectCheckable(MonitorObject *mo);
};

} /* namespace Checker */
} /* namespace QualityControl */
} /* namespace AliceO2 */

#endif /* QUALITYCONTROL_LIBS_CHECKER_CHECKER_H_ */
