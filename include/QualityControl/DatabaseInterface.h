///
/// \file   DatabaseInterface.h
/// \author Barthelemy von Haller
///

#ifndef QUALITYCONTROL_REPOSITORY_DATABASE_INTERFACE_H_
#define QUALITYCONTROL_REPOSITORY_DATABASE_INTERFACE_H_

#include "QualityControl/MonitorObject.h"
#include <memory>

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
    DatabaseInterface()
    {
    }
    /// Destructor
    virtual ~DatabaseInterface()
    {
    }

    virtual void connect(std::string username, std::string password) = 0;
    virtual void connect(std::string host, std::string database, std::string username, std::string password) = 0;
    virtual void store(AliceO2::QualityControl::Core::MonitorObject* mo) = 0;

    /**
     * Look up an object of a task and return it.
     * \details It returns the object if found or nullptr if not.
     * TODO evaluate whether we should have more methods to retrieve objects of different types (with or without templates)
     * TODO evaluate whether we should have a method to retrieve a list of objects (optimization)
     */
    virtual AliceO2::QualityControl::Core::MonitorObject* retrieve(std::string taskName, std::string objectName) = 0;
    virtual void disconnect() = 0;
    /**
     * \brief Prepare the container, such as a table in a relational database, that will contain the MonitorObject's for the given Task.
     * If the container already exists, we do nothing.
     */
    virtual void prepareTaskDataContainer(std::string taskName) = 0;
    virtual std::vector<std::string> getListOfTasksWithPublications() = 0;
    virtual std::vector<std::string> getPublishedObjectNames(std::string taskName) = 0;
};

} /* namespace Repository */
} /* namespace QualityControl */
} /* namespace AliceO2 */

#endif /* QUALITYCONTROL_REPOSITORY_DATABASE_INTERFACE_H_ */
