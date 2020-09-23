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
#include "QualityControl/MonitorObjectCollection.h"
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
  explicit ObjectsManager(const TaskConfig& taskConfig, bool noDiscovery = false);
  virtual ~ObjectsManager();

  static const std::string gDrawOptionsKey;
  static const std::string gDisplayHintsKey;

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
  void stopPublishing(const std::string& objectName);

  /**
   * Check whether an object is already being published
   * @param objectName
   * @return true if the object is already being published
   */
  bool isBeingPublished(const std::string& name);

  /**
   * Returns the published MonitorObject specified by its name
   * @param objectName The name of the object to find.
   * @return A pointer to the MonitorObject.
   * @throw ObjectNotFoundError if the object is not found.
   */
  MonitorObject* getMonitorObject(std::string objectName);

  MonitorObjectCollection* getNonOwningArray() const;

  /**
   * \brief Add metadata to a MonitorObject.
   * Add a metadata pair to a MonitorObject. This is propagated to the database.
   * @param objectName Name of the MonitorObject.
   * @param key Key of the metadata.
   * @param value Value of the metadata.
   * @throw ObjectNotFoundError if object is not found.
   */
  void addMetadata(const std::string& objectName, const std::string& key, const std::string& value);

  /**
   * \brief Set default draw options for this object.
   * If possible, the object will be drawn with these options (in the ROOT sense).
   * See for example https://root.cern/doc/master/classTHistPainter.html#HP01
   * E.g. manager->setDefaultDRawOptions("histo1", "colz");
   * @param objectName Name of the object affected by these drawOptions.
   * @param options The list of options, space separated.
   * @throw ObjectNotFoundError if object is not found.
   */
  void setDefaultDrawOptions(const std::string& objectName, const std::string& options);

  /**
   * See setDefaultDrawOptions(const std::string& objectName, const std::string& hioptionsnts).
   */
  void setDefaultDrawOptions(TObject* obj, const std::string& options);

  /**
   * \brief Indicate how to display this object.
   * A number of options can be set on a canvas to influence the way the object is displayed.
   * For drawOptions, use setDefaultDrawOptions, for others such as logarithmic scale or grid, use this method.
   * Currently supported by QCG: logx, logy, logz, gridx, gridy, gridz
   * @param objectName Name of the object affected by these drawOptions.
   * @param options The list of hints, space separated.
   * @throw ObjectNotFoundError if object is not found.
   */
  void setDisplayHint(const std::string& objectName, const std::string& hints);

  /**
   * See setDisplayHint(const std::string& objectName, const std::string& hints).
   */
  void setDisplayHint(TObject* obj, const std::string& hints);

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
  std::unique_ptr<MonitorObjectCollection> mMonitorObjects;
  TaskConfig mTaskConfig;
  std::unique_ptr<ServiceDiscovery> mServiceDiscovery;
  bool mUpdateServiceDiscovery;
};

} // namespace o2::quality_control::core

#endif // QC_CORE_OBJECTMANAGER_H
