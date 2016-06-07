///
/// \file   MySqlDatabase.cxx
/// \author Barthelemy von Haller
///

// std
#include <sstream>
// ROOT
#include "TMySQLResult.h"
#include "TMySQLStatement.h"
#include "TMessage.h"
// O2
#include "Common/Exceptions.h"
// QC
#include "QualityControl/MySqlDatabase.h"

using namespace AliceO2::Common;
using namespace std;

namespace AliceO2 {
namespace QualityControl {
namespace Repository {

MySqlDatabase::MySqlDatabase()
    : mServer(nullptr)
{
}

MySqlDatabase::~MySqlDatabase()
{
  Disconnect();
}

void MySqlDatabase::Connect(std::string host, std::string database, std::string username, std::string password)
{
  if (mServer) {
    if (mServer->IsConnected()) {
      mServer->Close();
    }
    delete mServer;
    mServer = 0;
  }
  stringstream connectionString;
  connectionString << "mysql://" << host << "/" << database;
  // Important as the agent can be inactive for more than 8 hours and Mysql will drop idle connections older than 8 hours
  connectionString << "?reconnect=1";
  if (!(mServer = new TMySQLServer(connectionString.str().c_str(), username.c_str(), password.c_str()))) {
    BOOST_THROW_EXCEPTION(FatalException() << errinfo_details("Failed to connect to the database"));
  } else {
    cout << "Connected to the database" << endl;
  }
}

void MySqlDatabase::PrepareTaskDataContainer(std::string taskName)
{
  // one object per run
  string query;
  query += "CREATE TABLE IF NOT EXISTS `data_" + taskName
      + "` (object_name CHAR(64), updatetime TIMESTAMP DEFAULT CURRENT_TIMESTAMP, data LONGBLOB, size INT, run INT, "
      "fill INT, PRIMARY KEY(object_name, run))";
  if (!Execute(query)) {
    BOOST_THROW_EXCEPTION(FatalException() << errinfo_details("Failed to create data table"));
  } else {
    cout << "Create data table for task " << taskName << endl;
  }
}

void MySqlDatabase::Store(AliceO2::QualityControl::Core::MonitorObject *mo)
{
  string taskName = mo->getTaskName();
  // try to insert if it fails we check whether the table is there or not and create it if needed

  // Prepare statement
  bool success = false;
  TMySQLStatement *statement;
  string query;
  query += "REPlACE INTO `data_" + taskName
      + "` (object_name, data, size, run, fill) values (?,?,octet_length(data),?,?)";
  statement = (TMySQLStatement*) mServer->Statement(query.c_str());
  if (mServer->IsError() && mServer->GetErrorCode() == 1146) {// table does not exist
    // try to create the table
    PrepareTaskDataContainer(taskName);
    statement = (TMySQLStatement*) mServer->Statement(query.c_str());
  }

  if (mServer->IsError()) {
    BOOST_THROW_EXCEPTION(DatabaseException() <<
        errinfo_details("Encountered an error when creating statement in PoolConnection") <<
        errinfo_db_message(mServer->GetErrorMsg()) <<
        errinfo_db_errno(mServer->GetErrorCode()));
  }

  // Assign data
  TMessage message(kMESS_OBJECT);
  message.WriteObjectAny(mo, mo->IsA());
  statement->NextIteration();
  statement->SetString(0, mo->getName().c_str());
  statement->SetBinary(1, message.Buffer(), message.Length(), message.Length());
  statement->SetInt(2, 0);
  statement->SetInt(3, 0);
  statement->Process();
  delete statement;
}

AliceO2::QualityControl::Core::MonitorObject* MySqlDatabase::Retrieve(std::string name)
{
  return nullptr;
}

void MySqlDatabase::Disconnect()
{
  if (mServer) {
    if (mServer->IsConnected()) {
      mServer->Close();
    }
    delete mServer;
    mServer = 0;
  }
}

TMySQLResult* MySqlDatabase::Query(string sql)
{
  if (mServer) {
    return ((TMySQLResult*) mServer->Query(sql.c_str()));
  } else {
    return (0);
  }
}

bool MySqlDatabase::Execute(string sql)
{
  if (mServer)
    return mServer->Exec(sql.c_str());

  return false;
}

void MySqlDatabase::AddIndex(string table, string column)
{
  std::ostringstream stringStream;
  stringStream << "CREATE INDEX " << table << "_i_" << column << " on " << table << " (" << column << ")";
  TMySQLResult* res = Query(stringStream.str().c_str());
  if (res) {
    delete (res);
  } else {
    cerr << "Couldn't create the index on table " << table << " on column " << column << endl;
  }
}

} // namespace Repository
} // namespace QualityControl
} // namespace AliceO2
