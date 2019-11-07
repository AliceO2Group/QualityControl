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
/// \file   ObjectsManager.h
/// \author Barthelemy von Haller
///

#ifndef QC_CORE_OBJECTMANAGER_H
#define QC_CORE_OBJECTMANAGER_H

// QC
#include "QualityControl/MonitorObject.h"
#include "QualityControl/Quality.h"
#include "QualityControl/TaskConfig.h"
// stl
#include <string>
#include <memory>

class TObject;
class TObjArray;

namespace o2::quality_control::core
{

class ServiceDiscovery;

/// \brief  Keeps the list of encapsulated objects to publish and does the actual publication.
///
/// Keeps a list of the objects to publish, encapsulates them and does the actual publication.
/// Tasks set/get properties of the MonitorObjects via this class.
///
/// \author Barthelemy von Haller
class ObjectsManager
{
 public:
  /**
   * Constructor
   * @param taskConfig The configuration of the task for which we are building this object manager
   * @param noDiscovery If true disables the use of ServiceDiscovery
   */
  explicit ObjectsManager(TaskConfig& taskConfig, bool noDiscovery = false);
  virtual ~ObjectsManager();

  /**
   * Start publishing the object obj, i.e. it will be pushed forward in the workflow at regular intervals.
   * The ownership remains to the caller.
   * @param obj The object to publish.
   * @throws DuplicateObjectError
   */
  void startPublishing(TObject* obj);

  /**
   * Stop publishing this object
   * @param obj
   * @throw ObjectNotFoundError if object is not found.
   */
  void stopPublishing(TObject* obj);

  /**
   * Stop publishing this object
   * @param obj
   * @throw ObjectNotFoundError if object is not found.
   */
  void stopPublishing(const std::string& name);

  /**
   * Return the quality of the object whose name is contained in objectName.
   * @param objectName
   * @return The quality of the object if was found.
   * @throw ObjectNotFoundError if object is not found.
   */
  Quality getQuality(std::string objectName);

  /// \brief Add a check to the object defined by objectName.
  /// If a check with the same already exist for this object, it will be replaced.
  /// \param objectName
  /// \param checkName
  /// \param checkClassName
  /// \param checkLibraryName
  void addCheck(const std::string& objectName, const std::string& checkName, const std::string& checkClassName,
                const std::string& checkLibraryName = "");

  void addCheck(const TObject* object, const std::string& checkName, const std::string& checkClassName,
                const std::string& checkLibraryName = "");

  MonitorObject* getMonitorObject(std::string objectName);

  TObject* getObject(std::string objectName);

  TObjArray* getNonOwningArray() const;

  /**
   * \brief Add metadata to a MonitorObject.
   * Add a metadata pair to a MonitorObject. This is propagated to the database.
   * @param objectName Name of the MonitorObject.
   * @param key Key of the metadata.
   * @param value Value of the metadata.
   */
  void addMetadata(const std::string& objectName, const std::string& key, const std::string& value);

  /**
   * Get the number of objects that have been published.
   * @return an int with the number of objects.
   */
  int getNumberPublishedObjects();

  /**
   * \brief Update the list of objects stored in the Service Discovery.
   * Update the list of objects stored in the Service Discovery.
   */
  void updateServiceDiscovery();

  /**
   * \brief Remove all objects from the ServiceDiscovery.
   * Remove all objects from the ServiceDiscovery even though they still might be published by the task.
   * This is typically used at End of Activity.
   */
  void removeAllFromServiceDiscovery();

 private:
  std::unique_ptr<TObjArray> mMonitorObjects;
  TaskConfig& mTaskConfig;
  std::unique_ptr<ServiceDiscovery> mServiceDiscovery;
  bool mUpdateServiceDiscovery;
};

} // namespace o2::quality_control::core

#endif // QC_CORE_OBJECTMANAGER_H
