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
#include "TMySQLRow.h"
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
  disconnect();
}

void MySqlDatabase::connect(std::string username, std::string password)
{
  // TODO use the configuration system to connect to the database
  connect("localhost", "quality_control", username, password);
}

void MySqlDatabase::connect(std::string host, std::string database, std::string username, std::string password)
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

void MySqlDatabase::prepareTaskDataContainer(std::string taskName)
{
  // one object per run
  string query;
  query += "CREATE TABLE IF NOT EXISTS `data_" + taskName
      + "` (object_name CHAR(64), updatetime TIMESTAMP DEFAULT CURRENT_TIMESTAMP, data LONGBLOB, size INT, run INT, "
          "fill INT, PRIMARY KEY(object_name, run))";
  if (!execute(query)) {
    BOOST_THROW_EXCEPTION(FatalException() << errinfo_details("Failed to create data table"));
  } else {
    cout << "Create data table for task " << taskName << endl;
  }
}

void MySqlDatabase::store(AliceO2::QualityControl::Core::MonitorObject *mo)
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
  if (mServer->IsError() && mServer->GetErrorCode() == 1146) { // table does not exist
  // try to create the table
    prepareTaskDataContainer(taskName);
    statement = (TMySQLStatement*) mServer->Statement(query.c_str());
  }

  if (mServer->IsError()) {
    BOOST_THROW_EXCEPTION(
        DatabaseException() << errinfo_details("Encountered an error when creating statement in MySqlDatabase")
            << errinfo_db_message(mServer->GetErrorMsg()) << errinfo_db_errno(mServer->GetErrorCode()));
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

AliceO2::QualityControl::Core::MonitorObject* MySqlDatabase::retrieve(std::string taskName, std::string objectName)
{
  string query;
  TMySQLStatement *statement = 0;

  query += "SELECT object_name, data, updatetime, run, fill FROM " + taskName + " WHERE object_name = ?";
  statement = (TMySQLStatement*) mServer->Statement(query.c_str());
  if (mServer->IsError()) {
    if (statement)
      delete statement;
    BOOST_THROW_EXCEPTION(
        DatabaseException() << errinfo_details("Encountered an error when creating statement in MySqlDatabase")
            << errinfo_db_message(mServer->GetErrorMsg()) << errinfo_db_errno(mServer->GetErrorCode()));
  }
  statement->NextIteration();
  statement->SetString(0, objectName.c_str());

  if (!(statement->Process() && statement->StoreResult())) {
    if (statement)
      delete statement;
    BOOST_THROW_EXCEPTION(
        DatabaseException()
            << errinfo_details("Encountered an error when processing and storing results in MySqlDatabase")
            << errinfo_db_message(mServer->GetErrorMsg()) << errinfo_db_errno(mServer->GetErrorCode()));
  }

  AliceO2::QualityControl::Core::MonitorObject* mo = nullptr;
  if (statement->NextResultRow()) { // Consider only the first result
    string name = statement->GetString(0);
    void* blob = 0;
    Long_t blobSize;
    statement->GetBinary(1, blob, blobSize); // retrieve the data
    TDatime updatetime(statement->GetYear(2), statement->GetMonth(2), statement->GetDay(2), statement->GetHour(2),
        statement->GetMinute(2), statement->GetSecond(2));
    int run = statement->IsNull(3) ? -1 : statement->GetInt(3);
    int fill = statement->IsNull(4) ? -1 : statement->GetInt(4);

    TMessage mess(kMESS_OBJECT);
    mess.SetBuffer(blob, blobSize, kFALSE);
    mess.SetReadMode();
    mess.Reset();
    mo = (AliceO2::QualityControl::Core::MonitorObject*) (mess.ReadObjectAny(mess.GetClass()));
  }
  if (statement)
    delete statement;

  return mo;
}

void MySqlDatabase::disconnect()
{
  if (mServer) {
    if (mServer->IsConnected()) {
      mServer->Close();
    }
    delete mServer;
    mServer = 0;
  }
}

TMySQLResult* MySqlDatabase::query(string sql)
{
  if (mServer) {
    return ((TMySQLResult*) mServer->Query(sql.c_str()));
  } else {
    return (0);
  }
}

bool MySqlDatabase::execute(string sql)
{
  if (mServer)
    return mServer->Exec(sql.c_str());

  return false;
}

void MySqlDatabase::addIndex(string table, string column)
{
  std::ostringstream stringStream;
  stringStream << "CREATE INDEX " << table << "_i_" << column << " on " << table << " (" << column << ")";
  TMySQLResult* res = query(stringStream.str().c_str());
  if (res) {
    delete (res);
  } else {
    cerr << "Couldn't create the index on table " << table << " on column " << column << endl;
  }
}

std::vector<std::string> MySqlDatabase::getPublishedObjectNames(std::string taskName)
{
  std::vector<std::string> result;

  string queryString = "select distinct object_name from ";
  queryString += taskName;

  TMySQLResult* mysqlResult = query(queryString);
  if (mysqlResult) { // only if we got a result
    TMySQLRow* row;
    while ((row = (TMySQLRow*) mysqlResult->Next())) {
      result.push_back(row->GetField(0));
      delete row;
    }
    delete mysqlResult;
  }

  return result;
}
std::vector<std::string> MySqlDatabase::getListOfTasksWithPublications()
{
  std::vector<std::string> result;

  string queryString = "select table_name from information_schema.tables where table_schema='quality_control'";

  TMySQLResult* mysqlResult = query(queryString);
  if (mysqlResult) { // only if we got a result
    TMySQLRow* row;
    while ((row = (TMySQLRow*) mysqlResult->Next())) {
      result.push_back(row->GetField(0));
      delete row;
    }
    delete mysqlResult;
  }

  return result;
}

} // namespace Repository
} // namespace QualityControl
} // namespace AliceO2
