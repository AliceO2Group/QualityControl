// Copyright 2019-2020 CERN and copyright holders of ALICE O2.
// See https://alice-o2.web.cern.ch/copyright for details of the copyright holders.
// All rights not expressly granted are reserved.
//
// This software is distributed under the terms of the GNU General Public
// License v3 (GPL Version 3), copied verbatim in the file "COPYING".
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

#include "QualityControl/DatabaseInterface.h"
#include <Common/Timer.h>
#include <boost/property_tree/ptree.hpp>
#include <memory>
#include <string>

namespace o2::ccdb
{
class CcdbApi;
}

namespace o2::quality_control::repository
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

  void connect(const std::string& host, const std::string& database, const std::string& username, const std::string& password) override;
  void connect(const std::unordered_map<std::string, std::string>& config) override;

  // storage
  void storeMO(std::shared_ptr<const o2::quality_control::core::MonitorObject> q) override;
  void storeQO(std::shared_ptr<const o2::quality_control::core::QualityObject> q) override;
  void storeTRFC(std::shared_ptr<const o2::quality_control::TimeRangeFlagCollection> trfc) override;
  void storeAny(const void* obj, std::type_info const& typeInfo, std::string const& path, std::map<std::string, std::string> const& metadata,
                std::string const& detectorName, std::string const& taskName, long from = -1, long to = -1) override;

  void* retrieveAny(std::type_info const& tinfo, std::string const& path,
                    std::map<std::string, std::string> const& metadata, long timestamp = -1,
                    std::map<std::string, std::string>* headers = nullptr,
                    const std::string& createdNotAfter = "", const std::string& createdNotBefore = "") override;

  // retrieval - MO - deprecated
  std::shared_ptr<o2::quality_control::core::MonitorObject> retrieveMO(std::string objectPath, std::string objectName, long timestamp = -1, const core::Activity& activity = {}) override;
  // retrieval - QO - deprecated
  std::shared_ptr<o2::quality_control::core::QualityObject> retrieveQO(std::string qoPath, long timestamp = -1, const core::Activity& activity = {}) override;
  std::shared_ptr<o2::quality_control::TimeRangeFlagCollection> retrieveTRFC(const std::string& name, const std::string& detector, int runNumber = 0,
                                                                             const std::string& passName = "", const std::string& periodName = "",
                                                                             const std::string& provenance = "", long timestamp = -1) override;

  // retrieval - general
  std::string retrieveJson(std::string path, long timestamp, const std::map<std::string, std::string>& metadata) override;
  TObject* retrieveTObject(std::string path, const std::map<std::string, std::string>& metadata, long timestamp = -1, std::map<std::string, std::string>* headers = nullptr) override;

  void disconnect() override;
  void prepareTaskDataContainer(std::string taskName) override;
  std::vector<std::string> getPublishedObjectNames(std::string taskName) override;
  void truncate(std::string taskName, std::string objectName) override;
  void storeStreamerInfosToFile(std::string filename);
  static long getCurrentTimestamp();
  static long getFutureTimestamp(int secondsInFuture);
  /**
  * Return the listing of folder and/or objects in the subpath.
  * @param subpath The folder we want to list the children of.
  * @return The listing of folder and/or objects at the subpath.
  */
  std::vector<std::string> getListing(const std::string& subpath = "");

  /**
   * Return the listing of folder and/or objects in the subpath
   * @param path the folder we want to list the children of.
   * @param metadata metadata to filter queried objects.
   * @param latestOnly return only the latest object matching the path and metadata.
   * @return The list of folder and/or objects as Ptree
   */
  boost::property_tree::ptree getListingAsPtree(const std::string& path, const std::map<std::string, std::string>& metadata = {}, bool latestOnly = false);

  /**
   * \brief Returns a vector of all 'valid from' timestamps for an object.
   * \path Path on an object.
   * \return A vector of all 'valid from' timestamps for an object in non-descending order.
   */
  std::vector<uint64_t> getTimestampsForObject(const std::string& path);

  void setMaxObjectSize(size_t maxObjectSize) override;

 private:
  /**
   * \brief Load StreamerInfos from a ROOT file.
   * When we were not saving TFiles in the CCDB, we streamed ROOT objects without their StreamerInfos.
   * As a result we can't read them back. The only way is to load them from a file. This is what we do
   * here.
   */
  static void loadDeprecatedStreamerInfos();
  void init();

  /**
   * Return the listing of folder and/or objects in the subpath.
   * @param subpath The folder we want to list the children of.
   * @param accept The format of the returned string as an \"Accept\", i.e. text/plain, application/json, text/xml
   * @param latestOnly Return only the latest matching object/folder
   * @return The listing of folder and/or objects in the format requested and as returned by the http server.
   */
  std::string getListingAsString(const std::string& subpath = "", const std::string& accept = "text/plain", bool latestOnly = false);

  /**
   * Takes care of the possible errors returned by the storage calls.
   * @param path
   * @param result
   */
  void handleStorageError(const std::string& path, int result);

  /**
   * Check whether the database has encountered a failure previously and if we are still in the
   * period afterwards when no attempt should be done.
   * @return
   */
  bool isDbInFailure();

  /**
   * Add the metadata specific to the QC framework.
   * @param fullMetadata
   * @param detectorName
   * @param taskName
   * @param className
   */
  static void addFrameworkMetadata(std::map<std::string, std::string>& fullMetadata, std::string detectorName, std::string className);

  std::unique_ptr<o2::ccdb::CcdbApi> ccdbApi;
  std::string mUrl;
  size_t mMaxObjectSize = 2097152; // 2MB by default
  int mFailureDelay = 60;          // 60 seconds delay between attempts to store things in the database
  bool mDatabaseFailure = false;
  AliceO2::Common::Timer mFailureTimer;
};

} // namespace o2::quality_control::repository

#endif // QC_REPOSITORY_CCDBDATABASE_H
