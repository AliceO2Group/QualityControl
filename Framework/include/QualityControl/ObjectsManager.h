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
/// \file   ObjectsManager.h
/// \author Barthelemy von Haller
///

#ifndef QC_CORE_OBJECTMANAGER_H
#define QC_CORE_OBJECTMANAGER_H

// QC
#include "QualityControl/Activity.h"
#include "QualityControl/MonitorObject.h"
#include "QualityControl/MonitorObjectCollection.h"
#include <Mergers/Mergeable.h>
// stl
#include <concepts>
#include <string>
#include <memory>
#include <type_traits>

class TObject;

namespace o2::quality_control::core
{

enum class PublicationPolicy {
  // QC framework will publish the object once after TaskInterface::endOfCycle() or PostProcessingInterface::update()
  // and then will remove it from the list of published objects. Typically to be used in TaskInterface::endOfCycle()
  // and PostProcessingInterface::update()
  Once,
  // QC framework will continue publishing this object after each TaskInterface::endOfCycle() and
  // PostProcessingInterface::update(), up to and including TaskInterface::endOfCycle() at EndOfStream and
  // PostProcessingInterface::finalize(). It will remove it from the list of published objects after that.
  // Typically to be used in TaskInterface::startOfActivity() and PostProcessingInterface::initialize()
  ThroughStop,
  // QC framework will continue publishing this object after each TaskInterface::endOfCycle() and
  // PostProcessingInterface::update() until the user task is destructed.
  // Usually to be used in TaskInterface::initialize() and PostProcessingInterface::configure()
  Forever
};

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
   * @param taskName Task name
   * @param taskClass Task's class
   * @param detectorName Detector 3-letter code
   * @param consulUrl Consul URL, for the service discovery
   * @param parallelTaskID ID of a parallel Task, use 0 if there is only one.
   * @param noDiscovery If true disables the use of ServiceDiscovery
   */
  ObjectsManager(std::string taskName, std::string taskClass, std::string detectorName, std::string consulUrl, int parallelTaskID = 0, bool noDiscovery = false);
  virtual ~ObjectsManager();

  static const std::string gDrawOptionsKey;
  static const std::string gDisplayHintsKey;

  /**
   * Start publishing the object obj, i.e. it will be pushed forward in the workflow at regular intervals.
   * The ownership remains to the caller.
   * @param IgnoreMergeable if you want to ignore static_assert check for Mergeable
   * @param T type of object that we want to publish.
   * @param obj The object to publish.
   * @throws DuplicateObjectError
   */
  template <bool IgnoreMergeable = false, typename T>
  void startPublishing(T obj, PublicationPolicy policy = PublicationPolicy::Forever)
  {
// If you want to ignore Mergeable check for whole module
#ifndef IGNORE_MERGEABLE_CHECK
    static_assert(std::same_as<std::remove_pointer_t<T>, TObject> ||
                    // there is TCanvas mentioned here because users are using it a lot when they should not
                    // std::same_as<std::remove_pointer_t<T>, TCanvas> ||
                    IgnoreMergeable || mergers::Mergeable<T>,
                  "you are trying to startPublishing object that is not mergeable."
                  " If you know what you are doing use startPublishing<true>(...)");
#endif
    startPublishingImpl(obj, policy);
  }

  /**
   * Stop publishing this object
   * @param obj
   * @throw ObjectNotFoundError if object is not found.
   */
  void
    stopPublishing(TObject* obj);

  /**
   * Stop publishing this object
   * @param obj
   * @throw ObjectNotFoundError if object is not found.
   */
  void stopPublishing(const std::string& objectName);

  /// \brief Stop publishing all objects with this publication policy
  void stopPublishing(PublicationPolicy policy);

  /// \brief Stop publishing all registered objects
  void stopPublishingAll();

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
  MonitorObject* getMonitorObject(const std::string& objectName);

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
   * \brief Add or update metadata to a MonitorObject.
   * Add or update a metadata pair to a MonitorObject. This is propagated to the database.
   * @param objectName Name of the MonitorObject.
   * @param key Key of the metadata.
   * @param value Value of the metadata.
   * @throw ObjectNotFoundError if object is not found.
   */
  void addOrUpdateMetadata(const std::string& objectName, const std::string& key, const std::string& value);

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
  size_t getNumberPublishedObjects();

  /**
   * Returns the published MonitorObject specified by its index
   * @param index
   * @return A pointer to the MonitorObject.
   * @throw ObjectNotFoundError if the object is not found.
   */
  MonitorObject* getMonitorObject(size_t index);

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

  /**
   * \brief Sets the validity interval of all registered objects.
   */
  void setValidity(ValidityInterval);

  /**
   * \brief Extends the validity interval of all registered objects to the provided value.
   */
  void updateValidity(validity_time_t);

  const Activity& getActivity() const;
  void setActivity(const Activity& activity);

  void setMovingWindowsList(const std::vector<std::string>&);
  const std::vector<std::string>& getMovingWindowsList() const;

 private:
  std::unique_ptr<MonitorObjectCollection> mMonitorObjects;
  std::map<MonitorObject*, PublicationPolicy> mPublicationPoliciesForMOs;
  std::string mTaskName;
  std::string mTaskClass;
  std::string mDetectorName;
  std::unique_ptr<ServiceDiscovery> mServiceDiscovery;
  bool mUpdateServiceDiscovery;
  Activity mActivity;
  std::vector<std::string> mMovingWindowsList;

  void startPublishingImpl(TObject* obj, PublicationPolicy);
};

} // namespace o2::quality_control::core

#endif // QC_CORE_OBJECTMANAGER_H
