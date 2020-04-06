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
/// \file   MySqlDatabase.h
/// \author Barthelemy von Haller
///

#ifndef QC_REPOSITORY_MYSQLDATABASE_H
#define QC_REPOSITORY_MYSQLDATABASE_H

#include <Common/Timer.h>

#include "QualityControl/DatabaseInterface.h"

class TMySQLResult;
class TMySQLServer;

namespace o2::quality_control::repository
{

/// \brief Implementation of the DatabaseInterface for MySQL
/// \todo consider storing directly the TObject, not the MonitorObject, and to put all its attributes as columns
/// \todo handle ROOT IO streamers
class MySqlDatabase : public DatabaseInterface
{
 public:
  /// Default constructor
  MySqlDatabase();
  /// Destructor
  ~MySqlDatabase() override;

  void connect(std::string host, std::string database, std::string username, std::string password) override;
  void connect(const std::unordered_map<std::string, std::string>& config) override;
  // MonitorObject
  void storeMO(std::shared_ptr<o2::quality_control::core::MonitorObject> q) override;
  std::shared_ptr<o2::quality_control::core::MonitorObject> retrieveMO(std::string taskName, std::string objectName, long timestamp = 0) override;
  std::string retrieveMOJson(std::string taskName, std::string objectName, long timestamp = 0) override;
  // QualityObject
  void storeQO(std::shared_ptr<o2::quality_control::core::QualityObject> q) override;
  std::shared_ptr<o2::quality_control::core::QualityObject> retrieveQO(std::string qoPath, long timestamp = 0) override;
  std::string retrieveQOJson(std::string qoPath, long timestamp = 0) override;
  // General
  std::string retrieveJson(std::string path, long timestamp, const std::map<string, string>& metadata) override;
  TObject* retrieveTObject(std::string path, const std::map<std::string, std::string>& metadata, long timestamp = -1, std::map<std::string, std::string>* headers = nullptr) override;

  void disconnect() override;
  std::vector<std::string> getPublishedObjectNames(std::string taskName) override;
  std::vector<std::string> getListOfTasksWithPublications();
  void truncate(std::string taskName, std::string objectName) override;

 private:
  /**
   * \brief Execute the query.
   * The result object must be deleted by the user.
   */
  TMySQLResult* query(std::string sql);

  /**
   * \brief Execute a query that doesn't return results;
   * @return true if successful.
   */
  bool execute(std::string sql);

  /**
   * \brief Create a new index on table 'table'.
   * The name of the index is \<table\>_i_\<column\>.
   */
  void addIndex(std::string table, std::string column);

  void prepareTaskDataContainer(std::string taskName) override;
  void prepareTable(std::string table_name);

  void storeQueue();
  void storeForMonitorObject(std::string taskName);
  void storeForQualityObject(std::string checkName);

  TMySQLServer* mServer;

  // Queue
  // name of tasks -> vector of mo
  std::map<std::string, std::vector<std::shared_ptr<o2::quality_control::core::QualityObject>>> mQualityObjectsQueue;
  std::map<std::string, std::vector<std::shared_ptr<o2::quality_control::core::MonitorObject>>> mMonitorObjectsQueue;
  size_t queueSize;
  AliceO2::Common::Timer lastStorage;
};

} // namespace o2::quality_control::repository

#endif // QC_REPOSITORY_MYSQLDATABASE_H
