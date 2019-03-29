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
/// \file   CcdbDatabase.h
/// \author Barthelemy von Haller
///

#ifndef QC_REPOSITORY_CCDBDATABASE_H
#define QC_REPOSITORY_CCDBDATABASE_H

#include "CCDB/CcdbApi.h"
#include "QualityControl/DatabaseInterface.h"

namespace o2
{
namespace quality_control
{
namespace repository
{

/*
 * Notes (also concerning the underlying CcdbApi)
 * - having 1 file per object per version server-side might lead to a tremendous number of files.
 *    --> they are aware of it
 * - how to add a new filter ? such as expert/shifter flag
 *    --> new metadata
 * - what are those time intervals ? what does it mean for us ?
 *    --> epoch milliseconds as long values
 * - how to know the real time at which the object was stored ?
 *    --> new api should allow for it. To be confirmed.
 * - we rather have a task_name/X/Y/Z/object_name/.../time where X/Y/Z are actually part of object_name but happen to
 * have slashes, to build a hierarchy of objects
 *    --> we can.
 * - we need to have a way to query for all objects in a certain path, e.g. in "task_name/X/Y" or in "task_name"
 *    --> the new api should allow it. To be tested.
 * - when retrieving an object, despite what the usage menu says, the time can't be omitted.
 *    --> doc has been udpated.
 * - initial tests show that it seems pretty slow.
 *    --> ok on their server with the new metadata database (postgresql)
 * - Current path to objects : .../task/object with object possibly a slash separated subpath (up to 6 levels). Also
 * consider having a task name such as "TPC/Task1" such as to build a tree of tasks with subsystems prefix.
 *
 */

class CcdbDatabase : public DatabaseInterface
{
 public:
  CcdbDatabase();
  virtual ~CcdbDatabase();

  void connect(std::string host, std::string database, std::string username, std::string password) override;
  void connect(const std::unordered_map<std::string, std::string>& config) override;
  void store(std::shared_ptr<o2::quality_control::core::MonitorObject> mo) override;
  core::MonitorObject* retrieve(std::string taskName, std::string objectName) override;
  std::string retrieveJson(std::string taskName, std::string objectName) override;
  void disconnect() override;
  void prepareTaskDataContainer(std::string taskName) override;
  std::vector<std::string> getListOfTasksWithPublications() override;
  std::vector<std::string> getPublishedObjectNames(std::string taskName) override;
  void truncate(std::string taskName, std::string objectName) override;

 private:
  long getCurrentTimestamp();
  std::string getTimestampString(long timestamp);
  long getFutureTimestamp(int secondsInFuture);
  /**
   * Return the listing of folder and/or objects in the subpath.
   * @param subpath The folder we want to list the children of.
   * @param accept The format of the returned string as an \"Accept\", i.e. text/plain, application/json, text/xml
   * @return The listing of folder and/or objects in the format requested and as returned by the http server.
   */
  std::string getListing(std::string subpath = "", std::string accept = "text/plain");
  o2::ccdb::CcdbApi ccdbApi;
  std::string mUrl;
};

} // namespace repository
} // namespace quality_control
} // namespace o2

#endif // QC_REPOSITORY_CCDBDATABASE_H
