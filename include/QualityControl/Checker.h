///
/// \file   Checker.h
/// \author Barthelemy von Haller
///

#ifndef QUALITYCONTROL_LIBS_CHECKER_CHECKER_H_
#define QUALITYCONTROL_LIBS_CHECKER_CHECKER_H_

#include "QualityControl/MonitorObject.h"
#include <FairMQDevice.h>
// QC
#include "QualityControl/QcInfoLogger.h"
#include "QualityControl/CheckInterface.h"

namespace AliceO2 {
namespace QualityControl {
namespace Checker {

/// \brief The class in charge of running the checks on a MonitorObject.
///
/// A Checker is in charge of loading/instantiating the proper checks for a given MonitorObject, to configure them
/// and to run them on the MonitorObject in order to generate a quality.
///
/// \author Barthelemy von Haller
class Checker : public FairMQDevice
{
  public:
    /// Default constructor
    Checker();
    /// Destructor
    virtual ~Checker();

    /**
     * \brief Evaluate the quality of a MonitorObject.
     *
     * The Check's associated with this MonitorObject are run and a global quality is built by
     * taking the worse quality encountered. The MonitorObject is modified by setting its quality
     * and by calling the "beautifying" methods of the Check's.
     *
     * @param mo The MonitorObject to evaluate and whose quality will be set according
     *        to the worse quality encountered while running the Check's.
     */
    void check(AliceO2::QualityControl::Core::MonitorObject *mo);

  protected:
    virtual void Run() override;

  private:
    void loadLibrary(const std::string libraryName) const;
    CheckInterface* instantiateCheck(std::string checkName, std::string className) const;

    AliceO2::QualityControl::Core::QcInfoLogger &mLogger;

};

} /* namespace Checker */
} /* namespace QualityControl */
} /* namespace AliceO2 */

#endif /* QUALITYCONTROL_LIBS_CHECKER_CHECKER_H_ */
