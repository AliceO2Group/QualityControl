// Copyright CERN and copyright holders of ALICE O2. This software is
// distributed under the terms of the GNU General Public License v3 (GPL
// Version 3), copied verbatim in the file "COPYING".
//
// See http://alice-o2.web.cern.ch/license for full licensing information.
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.

///
/// \file   MySqlDatabase.cxx
/// \author Barthelemy von Haller
///

// std
#include <sstream>
// ROOT
#include <TMessage.h>
#include <TMySQLResult.h>
#include <TMySQLRow.h>
#include <TMySQLStatement.h>
#include <TBufferJSON.h>
// O2
#include <Common/Exceptions.h>
// QC
#include "QualityControl/MySqlDatabase.h"
#include "QualityControl/QcInfoLogger.h"

using namespace AliceO2::Common;
using namespace std;
using namespace o2::quality_control::core;

namespace o2::quality_control::repository
{

MySqlDatabase::MySqlDatabase() : mServer(nullptr), queueSize(0) { lastStorage.reset(); }

MySqlDatabase::~MySqlDatabase() { disconnect(); }

void MySqlDatabase::connect(std::string host, std::string database, std::string username, std::string password)
{
  if (mServer) {
    if (mServer->IsConnected()) {
      mServer->Close();
    }
    delete mServer;
    mServer = nullptr;
  }
  stringstream connectionString;
  connectionString << "mysql://" << host << "/" << database;
  // Important as the agent can be inactive for more than 8 hours and Mysql will drop idle connections older than 8
  // hours
  connectionString << "?reconnect=1";
  mServer = new TMySQLServer(connectionString.str().c_str(), username.c_str(), password.c_str());
  if (!mServer || mServer->GetErrorCode()) {
    string s = "Failed to connect to the database\n";
    if (mServer->GetErrorCode()) {
      s += mServer->GetErrorMsg();
    }
    BOOST_THROW_EXCEPTION(FatalException() << errinfo_details(s));
  } else {
    QcInfoLogger::GetInstance() << "Connected to the database" << infologger::endm;
  }
}

void MySqlDatabase::connect(const std::unordered_map<std::string, std::string>& config)
{
  this->connect(config.at("host"),
                config.at("name"),
                config.at("username"),
                config.at("password"));
}

void MySqlDatabase::prepareTaskDataContainer(std::string taskName)
{
  // one object per run
  string query;
  query += "CREATE TABLE IF NOT EXISTS `data_" + taskName +
           "` (object_name CHAR(64), updatetime TIMESTAMP DEFAULT CURRENT_TIMESTAMP, data LONGBLOB, size INT, run INT, "
           "fill INT, PRIMARY KEY(object_name, run)) ENGINE=MyISAM";
  if (!execute(query)) {
    BOOST_THROW_EXCEPTION(FatalException() << errinfo_details("Failed to create data table"));
  } else {
    QcInfoLogger::GetInstance() << "Create data table for task " << taskName << infologger::endm;
  }
}

void MySqlDatabase::store(std::shared_ptr<o2::quality_control::core::MonitorObject> mo)
{
  // TODO we take ownership here to delete later -> clearly to be improved
  // we execute grouped insertions. Here we just register that we should keep this mo in memory.
  mObjectsQueue[mo->getTaskName()].push_back(mo);
  queueSize++;
  if (queueSize > 4 || lastStorage.getTime() > 10 /*sec*/) { // TODO use a configuration to set the max limits
    storeQueue();
  }
}

void MySqlDatabase::storeQueue()
{
  QcInfoLogger::GetInstance() << "Database queue will now be processed (" << queueSize << " objects)"
                              << infologger::endm;

  for (auto& kv : mObjectsQueue) {
    storeForTask(kv.first);
  }
  mObjectsQueue.clear();
  queueSize = 0;
  lastStorage.reset();
}

void MySqlDatabase::storeForTask(std::string taskName)
{
  std::vector<std::shared_ptr<o2::quality_control::core::MonitorObject>> objects = mObjectsQueue[taskName];

  if (objects.size() == 0) {
    return;
  }

  cout << "** Store for task " << taskName << endl;
  cout << "        # objects : " << objects.size() << endl;

  // build statement string
  string query;
  query +=
    "REPlACE INTO `data_" + taskName + "` (object_name, data, size, run, fill) values (?,?,octet_length(data),?,?)";

  // Prepare statement
  TMySQLStatement* statement;
  // try to insert if it fails we check whether the table is there or not and create it if needed
  statement = (TMySQLStatement*)mServer->Statement(query.c_str());
  if (mServer->IsError() && mServer->GetErrorCode() == 1146) { // table does not exist
    // try to create the table
    prepareTaskDataContainer(taskName);
    statement = (TMySQLStatement*)mServer->Statement(query.c_str());
  }
  if (mServer->IsError()) {
    BOOST_THROW_EXCEPTION(DatabaseException()
                          << errinfo_details("Encountered an error when creating statement in MySqlDatabase")
                          << errinfo_db_message(mServer->GetErrorMsg()) << errinfo_db_errno(mServer->GetErrorCode()));
  }

  // Assign data
  TMessage message(kMESS_OBJECT);
  for (auto mo : objects) {
    message.Reset();
    message.WriteObjectAny(mo.get(), mo->IsA());
    statement->NextIteration();
    statement->SetString(0, mo->getName().c_str());
    statement->SetBinary(1, message.Buffer(), message.Length(), message.Length());
    statement->SetInt(2, 0);
    statement->SetInt(3, 0);
  }
  statement->Process();
  delete statement;

  //  for (auto mo : objects) {
  //    delete mo;
  //  }
  objects.clear();
}

o2::quality_control::core::MonitorObject* MySqlDatabase::retrieve(std::string taskName, std::string objectName, long /*timestamp*/)
{
  // TODO use the timestamp

  string query;
  TMySQLStatement* statement = nullptr;

  query += "SELECT object_name, data, updatetime, run, fill FROM data_" + taskName + " WHERE object_name = ?";
  statement = (TMySQLStatement*)mServer->Statement(query.c_str());
  if (mServer->IsError()) {
    if (statement) {
      delete statement;
    }
    BOOST_THROW_EXCEPTION(DatabaseException()
                          << errinfo_details("Encountered an error when creating statement in MySqlDatabase")
                          << errinfo_db_message(mServer->GetErrorMsg()) << errinfo_db_errno(mServer->GetErrorCode()));
  }
  statement->NextIteration();
  statement->SetString(0, objectName.c_str());

  if (!(statement->Process() && statement->StoreResult())) {
    delete statement;
    BOOST_THROW_EXCEPTION(DatabaseException()
                          << errinfo_details(
                               "Encountered an error when processing and storing results in MySqlDatabase")
                          << errinfo_db_message(mServer->GetErrorMsg()) << errinfo_db_errno(mServer->GetErrorCode()));
  }

  o2::quality_control::core::MonitorObject* mo = nullptr;
  if (statement->NextResultRow()) { // Consider only the first result
    string name = statement->GetString(0);
    void* blob = nullptr;
    Long_t blobSize = 0;
    statement->GetBinary(1, blob, blobSize); // retrieve the data
                                             //    TDatime updatetime(statement->GetYear(2), statement->GetMonth(2), statement->GetDay(2), statement->GetHour(2),
                                             //                       statement->GetMinute(2), statement->GetSecond(2));
                                             //    int run = statement->IsNull(3) ? -1 : statement->GetInt(3);
                                             //    int fill = statement->IsNull(4) ? -1 : statement->GetInt(4);

    TMessage mess(kMESS_OBJECT);
    mess.SetBuffer(blob, blobSize, kFALSE);
    mess.SetReadMode();
    mess.Reset();
    try {
      mo = (o2::quality_control::core::MonitorObject*)(mess.ReadObjectAny(mess.GetClass()));
    } catch (...) {
      QcInfoLogger::GetInstance() << "Node: unable to parse TObject from MySQL" << infologger::endm;
      throw;
    }
  }
  delete statement;

  return mo;
}

std::string MySqlDatabase::retrieveJson(std::string taskName, std::string objectName)
{
  std::unique_ptr<o2::quality_control::core::MonitorObject> monitor(retrieve(taskName, objectName));
  if (monitor == nullptr) {
    return std::string();
  }
  std::unique_ptr<TObject> obj(monitor->getObject());
  monitor->setIsOwner(false);
  TString json = TBufferJSON::ConvertToJSON(obj.get());
  return json.Data();
}

void MySqlDatabase::disconnect()
{
  storeQueue();

  if (mServer) {
    if (mServer->IsConnected()) {
      mServer->Close();
    }
    delete mServer;
    mServer = nullptr;
  }
}

TMySQLResult* MySqlDatabase::query(string sql)
{
  if (mServer) {
    return ((TMySQLResult*)mServer->Query(sql.c_str()));
  } else {
    return (nullptr);
  }
}

bool MySqlDatabase::execute(string sql)
{
  if (mServer) {
    return mServer->Exec(sql.c_str());
  }

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
  queryString += "data_" + taskName;

  TMySQLResult* mysqlResult = query(queryString);
  if (mysqlResult) { // only if we got a result
    TMySQLRow* row;
    while ((row = (TMySQLRow*)mysqlResult->Next())) {
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
    while ((row = (TMySQLRow*)mysqlResult->Next())) {
      result.push_back(row->GetField(0));
      delete row;
    }
    delete mysqlResult;
  }

  return result;
}

void MySqlDatabase::truncate(std::string taskName, std::string objectName)
{
  string queryString = string("delete ignore from `data_") + taskName + "` where object_name='" + objectName + "'";

  if (!execute(queryString)) {
    string s = string("Failed to delete object ") + objectName + " from task " + taskName;
    BOOST_THROW_EXCEPTION(FatalException() << errinfo_details(s));
  } else {
    QcInfoLogger::GetInstance() << "Delete object " << objectName << " from task " << taskName << infologger::endm;
  }
}

} // namespace o2::quality_control::repository
