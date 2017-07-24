///
/// \file   CheckInterface.h
/// \author Barthelemy von Haller
///

#ifndef QUALITYCONTROL_LIBS_CHECKER_CHECKINTERFACE_H_
#define QUALITYCONTROL_LIBS_CHECKER_CHECKINTERFACE_H_

#include "MonitorObject.h"
#include "Quality.h"

using namespace AliceO2::QualityControl::Core;

namespace AliceO2 {
namespace QualityControl {
namespace Checker {

/// \brief  Skeleton of a check.
///
/// Developer note (BvH) : the class is stateless and should stay so as we want to reuse the
///    same object several times in a row and call the methods in whatever order.
///    One could think that the methods should be static but then we would not be able
///    to use polymorphism.
///
/// \author Barthelemy von Haller
class CheckInterface
{
  public:
    /// Default constructor
    CheckInterface();
    /// Destructor
    virtual ~CheckInterface();

    /// \brief Configure the check based on its name.
    ///
    /// The configuration of the object can't be done in the constructor because
    /// ROOT needs an argument-less constructor when streaming it. We use this method
    /// to configure the object. The name might be used to ask the configuration system
    /// for specific parameters.
    virtual void configure(std::string name);

    /// \brief Returns the quality associated with this object.
    ///
    /// @param mo The MonitorObject to check.
    /// @return The quality of the object.
    virtual Quality check(const MonitorObject *mo) = 0;

    /// \brief Modify the aspect of the plot.
    ///
    /// Modify the aspect of the plot.
    /// It is usually based on the result of the check (passed as quality)
    ///
    /// @param mo          The MonitorObject to beautify.
    /// @param checkResult The quality returned by the check. It is not the same as the quality of the mo
    ///                    as the latter represents the combination of all the checks the mo passed. This
    ///                    parameter is to be used to pass the result of the check of the same class.
    virtual void beautify(MonitorObject *mo, Quality checkResult = Quality::Null) = 0;

    /// \brief Returns the name of the class that can be treated by this check.
    ///
    /// The name of the class returned by this method will be checked against the MonitorObject's encapsulated
    /// object's class. If it is the same or a parent then the check will be applied. Therefore, this method
    /// must return the highest class in the hierarchy that this check can use.
    /// If the class does not override it, we return "TObject".
    ///
    /// \author Barthelemy von Haller
    virtual std::string getAcceptedType();

    bool isObjectCheckable(const MonitorObject *mo);

  private:
    std::string mName;

  ClassDef(CheckInterface, 1)
};

} /* namespace Checker */
} /* namespace QualityControl */
} /* namespace AliceO2 */

#endif /* QUALITYCONTROL_LIBS_CHECKER_CHECKINTERFACE_H_ */
