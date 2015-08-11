///
/// \file   Checker.h
/// \author Barthelemy von Haller
///

#ifndef QUALITYCONTROL_LIBS_CHECKER_CHECKER_H_
#define QUALITYCONTROL_LIBS_CHECKER_CHECKER_H_

namespace AliceO2 {
namespace QualityControl {
namespace Checker {

/// \brief The class in charge of running the checks on a MonitorObject.
///
/// A Checker is in charge of loading/instantiating the proper checks for a given MonitorObject, to configure them
/// and to run them on the MonitorObject in order to generate a quality.
///
/// \author Barthelemy von Haller
class Checker
{
  public:
    /// Default constructor
    Checker();
    /// Destructor
    virtual ~Checker();
};

} /* namespace Checker */
} /* namespace QualityControl */
} /* namespace AliceO2 */

#endif /* QUALITYCONTROL_LIBS_CHECKER_CHECKER_H_ */
