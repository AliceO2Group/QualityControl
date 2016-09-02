///
/// \file   MySqlDatabase.h
/// \author Barthelemy von Haller
///

#ifndef QUALITYCONTROL_REPOSITORY_MYSQL_DATABASE_H_
#define QUALITYCONTROL_REPOSITORY_MYSQL_DATABASE_H_

#include "QualityControl/DatabaseInterface.h"
#include "TMySQLServer.h"
#include "Common/Timer.h"

class TMySQLResult;

namespace AliceO2 {
namespace QualityControl {
namespace Repository {

/**
 * TODO: consider storing directly the TObject, not the MonitorObject, and to put all its attributes as columns
 * TODO: handle ROOT IO streamers
 */

class MySqlDatabase: public DatabaseInterface
{
  public:
    /// Default constructor
    MySqlDatabase();
    /// Destructor
    virtual ~MySqlDatabase();

    void connect(std::string username, std::string password) override;
    void connect(std::string host, std::string database, std::string username, std::string password) override;
    void store(std::shared_ptr<AliceO2::QualityControl::Core::MonitorObject> mo) override;
    AliceO2::QualityControl::Core::MonitorObject* retrieve(std::string taskName, std::string objectName) override;
    void disconnect() override;
    std::vector<std::string> getPublishedObjectNames(std::string taskName) override;
    std::vector<std::string> getListOfTasksWithPublications() override;

  private:
    /**
     * \brief Execute the query.
     * The result object must be deleted by the user.
     */
    TMySQLResult* query(std::string sql);

    /**
     * \brief Execute a query that doesn't return results;
     * Return true if successful.
     */
    bool execute(std::string sql);

    /**
     * \brief Create a new index on table 'table'.
     * The name of the index is <table>_i_<column>.
     */
    void addIndex(std::string table, std::string column);

    void prepareTaskDataContainer(std::string taskName) override;

    void storeQueue();
    void storeForTask(std::string taskName);

    TMySQLServer* mServer;

    // Queue
    // name of tasks -> vector of mo
    std::map<std::string, std::vector<std::shared_ptr<AliceO2::QualityControl::Core::MonitorObject>>> mObjectsQueue;
    size_t queueSize;
    AliceO2::Common::Timer lastStorage;
};

} // namespace Core
} // namespace QualityControl
} // namespace AliceO2

#endif // QUALITYCONTROL_REPOSITORY_MYSQL_DATABASE_H_
