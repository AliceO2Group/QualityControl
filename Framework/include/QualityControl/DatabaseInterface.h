///
/// \file   DatabaseInterface.h
/// \author Barthelemy von Haller
///

#ifndef QUALITYCONTROL_REPOSITORY_DATABASE_INTERFACE_H_
#define QUALITYCONTROL_REPOSITORY_DATABASE_INTERFACE_H_

#include "QualityControl/MonitorObject.h"
#include <memory>
#include <Configuration/ConfigurationInterface.h>
//#include <bits/unique_ptr.h>

namespace o2 {
namespace quality_control {
namespace repository {

using namespace o2::configuration;

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

    /**
     * Connects to the database.
     * For some implementations, this is a noop.
     * @param host
     * @param database
     * @param username
     * @param password
     * @deprecated
     */
    virtual void connect(std::string host, std::string database, std::string username, std::string password) = 0;
    /**
     * Connects to the database.
     * For some implementations, this is a noop.
     * @param config Configuration where the connection parameters are stored.
     */
    virtual void connect(std::unique_ptr<ConfigurationInterface> &config) = 0;

    /**
     * Stores the serialized MonitorObject in the database.
     * @param mo The MonitorObject to serialize and store.
     */
    virtual void store(o2::quality_control::core::MonitorObject* mo) = 0;

    /**
     * Look up an object of a task and return it.
     * \details It returns the object if found or nullptr if not.
     * TODO evaluate whether we should have more methods to retrieve objects of different types (with or without templates)
     * TODO evaluate whether we should have a method to retrieve a list of objects (optimization)
     */
    virtual o2::quality_control::core::MonitorObject* retrieve(std::string taskName, std::string objectName) = 0;
    virtual void disconnect() = 0;
    /**
     * \brief Prepare the container, such as a table in a relational database, that will contain the MonitorObject's for the given Task.
     * If the container already exists, we do nothing.
     */
    virtual void prepareTaskDataContainer(std::string taskName) = 0;
    virtual std::vector<std::string> getListOfTasksWithPublications() = 0;
    virtual std::vector<std::string> getPublishedObjectNames(std::string taskName) = 0;
};

} /* namespace repository */
} /* namespace QualityControl */
} /* namespace o2 */

#endif /* QUALITYCONTROL_REPOSITORY_DATABASE_INTERFACE_H_ */
