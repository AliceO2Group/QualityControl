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
/// \file   DatabaseInterface.h
/// \author Barthelemy von Haller
///

#ifndef QC_REPOSITORY_DATABASEINTERFACE_H
#define QC_REPOSITORY_DATABASEINTERFACE_H

#include <string>
#include <memory>
#include <vector>
#include <unordered_map>

#include <Framework/ServiceRegistry.h>

#include "QualityControl/QualityObject.h"
#include "QualityControl/MonitorObject.h"

namespace o2::quality_control::repository
{

/// \brief The interface to the MonitorObject's repository.
///
/// \author Barthélémy von Haller
class DatabaseInterface
{
 public:
  constexpr static framework::ServiceKind service_kind = framework::ServiceKind::Global;

  /// Default constructor
  DatabaseInterface() = default;
  /// Destructor
  virtual ~DatabaseInterface() = default;

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
   * @param config map of values coming from configuration library.
   */
  virtual void connect(const std::unordered_map<std::string, std::string>& config) = 0;

  /**
  * Store an object of type `typeInfo` (which needs to have a ROOT dictionary).
  * Example usage: `storeAny(reinterpret_cast<const void*>(obj), typeid(T), path, metadata, "TST", "taskname", from, to);`
   *
   * Note that we cannot have a more elegant templated signature due to the fact that it is a virtual method.
  *
  * @param obj Raw pointer to the object to store.
  * @param path The path where the object is going to be stored.
  * @param metadata Key-values representing the metadata for this object.
  * @param detectorName The name of the detector (will appear in the metadata, not used in the path)
  * @param taskName The name of the task (will appear in the metadata, not used in the path)
  * @param from Start of validity. If omitted, current timestamp is used.
  * @param to End of validity. If omitted, current timestamp + 1 year is used.
  */
  virtual void storeAny(const void* obj, std::type_info const& typeInfo, std::string const& path, std::map<std::string, std::string> const& metadata,
                        std::string const& detectorName, std::string const& taskName, long from = -1, long to = -1) = 0;

  /**
   * Stores the serialized MonitorObject in the database.
   * @param mo The MonitorObject to serialize and store.
   * @param from The timestamp indicating the start of object's validity (ms since epoch).
   * @param to The timestamp indicating the end of object's validity (ms since epoch).
   */
  virtual void storeMO(std::shared_ptr<const o2::quality_control::core::MonitorObject> mo, long from = -1, long to = -1) = 0;

  /**
   * Stores the serialized QualityObject in the database.
   * @param qo The QualityObject to serialize and store.
   * @param from The timestamp indicating the start of object's validity (ms since epoch).
   * @param to The timestamp indicating the end of object's validity (ms since epoch).
   */
  virtual void storeQO(std::shared_ptr<const o2::quality_control::core::QualityObject> qo, long from = -1, long to = -1) = 0;

  /**
   * \brief Look up a monitor object and return it.
   * Look up a monitor object and return it if found or nullptr if not.
   * @deprecated
   */
  virtual std::shared_ptr<o2::quality_control::core::MonitorObject> retrieveMO(std::string taskName, std::string objectName, long timestamp = -1) = 0;
  /**
   * \brief Look up a quality object and return it.
   * Look up a quality object and return it if found or nullptr if not.
   * @deprecated
   */
  virtual std::shared_ptr<o2::quality_control::core::QualityObject> retrieveQO(std::string qoPath, long timestamp = -1) = 0;
  /**
   * \brief Look up an object and return it.
   * Look up an object and return it if found or nullptr if not. It is a raw pointer because we might need it to build a MO.
   * \param path the path of the object
   * \param timestamp the timestamp to query the object
   * \param headers Map to be populated with the headers we received, if it is not null.
   * \param metadata filters under the form of key-value pairs to select data
   */
  virtual TObject* retrieveTObject(std::string path, const std::map<std::string, std::string>& metadata, long timestamp = -1, std::map<std::string, std::string>* headers = nullptr) = 0;

  /**
   * \brief Look up a monitor object and return it in JSON format.
   * Look up a monitor object and return it in JSON format if found or an empty string if not.
   * The headers associated with the object are added to the JSON object under the key "metadata".
   * @deprecated
   */
  virtual std::string retrieveMOJson(std::string taskName, std::string objectName, long timestamp = -1) = 0;
  /**
   * \brief Look up a quality object and return it in JSON format.
   * Look up a quality object and return it in JSON format if found or an empty string if not.
   * The headers associated with the object are added to the JSON object under the key "metadata".
   * @deprecated
   */
  virtual std::string retrieveQOJson(std::string qoPath, long timestamp = -1) = 0;
  /**
   * \brief Look up an object and return it in JSON format.
   * Look up an object and return it in JSON format if found or an empty string if not.
   * The headers associated with the object are added to the JSON object under the key "metadata".
   * \param path the path of the object
   * \param timestamp the timestamp to query the object
   * \param metadata filters under the form of key-value pairs to select data
   */
  virtual std::string retrieveJson(std::string path, long timestamp, const std::map<std::string, std::string>& metadata) = 0;
  /**
   * \brief Look up an object and return it in JSON format.
   * Look up an object and return it in JSON format if found or an empty string if not.
   * The headers associated with the object are added to the JSON object under the key "metadata".
   * A default timestamp of -1 is used, usually meaning to use the current timestamp.
   * \param path the path to the object
   */
  virtual std::string retrieveJson(std::string path)
  {
    std::map<std::string, std::string> metadata;
    return retrieveJson(path, -1, metadata);
  }

  virtual void disconnect() = 0;
  /**
   * \brief Prepare the container, such as a table in a relational database, that will contain the MonitorObject's for
   * the given Task. If the container already exists, we do nothing.
   */
  virtual void prepareTaskDataContainer(std::string taskName) = 0;
  virtual std::vector<std::string> getPublishedObjectNames(std::string taskName) = 0;
  /**
   * Delete all versions of a given object
   * @param taskName Task sending the object
   * @param objectName Name of the object
   */
  virtual void truncate(std::string taskName, std::string objectName) = 0;
};

} // namespace o2::quality_control::repository

#endif /* QC_REPOSITORY_DATABASEINTERFACE_H */
