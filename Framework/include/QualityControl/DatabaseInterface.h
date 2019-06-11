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

#include "QualityControl/MonitorObject.h"
#include <memory>
#include <unordered_map>
//#include <bits/unique_ptr.h>

namespace o2::quality_control::repository
{

/// \brief The interface to the MonitorObject's repository.
///
/// \author Barthélémy von Haller
class DatabaseInterface
{
 public:
  /// Default constructor
  DatabaseInterface() {}
  /// Destructor
  virtual ~DatabaseInterface() {}

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
   * Stores the serialized MonitorObject in the database.
   * @param mo The MonitorObject to serialize and store.
   */
  virtual void store(std::shared_ptr<o2::quality_control::core::MonitorObject> mo) = 0;

  /**
   * Look up an object of a task and return it.
   * \details It returns the object if found or nullptr if not.
   * TODO evaluate whether we should have more methods to retrieve objects of different types (with or without
   * templates)
   * TODO evaluate whether we should have a method to retrieve a list of objects (optimization)
   */
  virtual o2::quality_control::core::MonitorObject* retrieve(std::string taskName, std::string objectName, long timestamp = 0) = 0;

  /**
   * Returns JSON encoded object
   */
  virtual std::string retrieveJson(std::string taskName, std::string objectName) = 0;
  virtual void disconnect() = 0;
  /**
   * \brief Prepare the container, such as a table in a relational database, that will contain the MonitorObject's for
   * the given Task. If the container already exists, we do nothing.
   */
  virtual void prepareTaskDataContainer(std::string taskName) = 0;
  virtual std::vector<std::string> getListOfTasksWithPublications() = 0;
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
