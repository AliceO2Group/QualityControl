///
/// \file   MySqlDatabase.h
/// \author Barthelemy von Haller
///

#ifndef QUALITYCONTROL_REPOSITORY_MYSQL_DATABASE_H_
#define QUALITYCONTROL_REPOSITORY_MYSQL_DATABASE_H_

#include "QualityControl/DatabaseInterface.h"
#include "TMySQLServer.h"

class TMySQLResult;

namespace AliceO2 {
namespace QualityControl {
namespace Repository {

class MySqlDatabase: public DatabaseInterface
{
  public:
    /// Default constructor
    MySqlDatabase();
    /// Destructor
    virtual ~MySqlDatabase();

    void Connect(std::string host, std::string database, std::string username, std::string password) override;
    void Store(AliceO2::QualityControl::Core::MonitorObject *mo) override;
    AliceO2::QualityControl::Core::MonitorObject* Retrieve(std::string name) override;
    void Disconnect() override;

  private:
    /**
     * \brief Execute the query.
     * The result object must be deleted by the user.
     */
    TMySQLResult* Query(std::string sql);

    /**
     * \brief Execute a query that doesn't return results;
     * Return true if successful.
     */
    bool Execute(std::string sql);

    /**
     * \brief Create a new index on table 'table'.
     * The name of the index is <table>_i_<column>.
     */
    void AddIndex(std::string table, std::string column);

    void PrepareTaskDataContainer(std::string taskName) override;

    TMySQLServer* mServer;

};

} // namespace Core
} // namespace QualityControl
} // namespace AliceO2

#endif // QUALITYCONTROL_REPOSITORY_MYSQL_DATABASE_H_
