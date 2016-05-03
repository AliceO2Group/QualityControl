///
/// \file   DatabaseInterface.h
/// \author Barthelemy von Haller
///

#ifndef QUALITYCONTROL_REPOSITORY_DATABASE_INTERFACE_H_
#define QUALITYCONTROL_REPOSITORY_DATABASE_INTERFACE_H_

#include "QualityControl/MonitorObject.h"

namespace AliceO2 {
namespace QualityControl {
namespace Repository {

/// \brief The interface to the MonitorObject's repository.
///
/// \author Barthelemy von Haller
class DatabaseInterface
{
  public:
    /// Default constructor
    DatabaseInterface(){}
    /// Destructor
    virtual ~DatabaseInterface(){}

    virtual void Connect() = 0;
    virtual void Store(AliceO2::QualityControl::Core::MonitorObject *mo) = 0;
    virtual AliceO2::QualityControl::Core::MonitorObject* Retrieve(std::string name) = 0;
    virtual void Disconnect() = 0;
};

} /* namespace Repository */
} /* namespace QualityControl */
} /* namespace AliceO2 */

#endif /* QUALITYCONTROL_REPOSITORY_DATABASE_INTERFACE_H_ */
