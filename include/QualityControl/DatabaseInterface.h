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
/// \author Barthélémy von Haller
class DatabaseInterface
{
  public:
    /// Default constructor
    DatabaseInterface(){}
    /// Destructor
    virtual ~DatabaseInterface(){}

    virtual void Connect(std::string host, std::string database, std::string username, std::string password) = 0;
    virtual void Store(AliceO2::QualityControl::Core::MonitorObject *mo) = 0;
    // TODO evaluate whether we should have more methods to retrieve objects of different types (with or without templates)
    virtual AliceO2::QualityControl::Core::MonitorObject* Retrieve(std::string name) = 0;
    virtual void Disconnect() = 0;
    /**
     * \brief Prepare the container, such as a table in a relational database, that will contain the MonitorObject's for the given Task.
     * If the container already exists, we do nothing.
     */
    virtual void PrepareTaskDataContainer(std::string taskName) = 0;
};

} /* namespace Repository */
} /* namespace QualityControl */
} /* namespace AliceO2 */

#endif /* QUALITYCONTROL_REPOSITORY_DATABASE_INTERFACE_H_ */
