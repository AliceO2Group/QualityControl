///
/// \file   ObjectsManager.h
/// \author Barthelemy von Haller
///

#ifndef QC_CORE_OBJECTMANAGER_H
#define QC_CORE_OBJECTMANAGER_H

#include "QualityControl/MonitorObject.h"
#include "QualityControl/Quality.h"
#include "QualityControl/TaskConfig.h"
#include <TObjArray.h>
#include <TObjString.h>
#include <boost/concept_check.hpp>
#include <string>

namespace o2::quality_control::core
{

/// \brief  Keeps the list of encapsulated objects to publish and does the actual publication.
///
/// Keeps a list of the objects to publish, encapsulates them and does the actual publication.
/// Tasks set/get properties of the MonitorObjects via this class.
///
/// \author Barthelemy von Haller
class ObjectsManager
{
  friend class TaskControl; // TaskControl must be able to call "publish()" whenever needed. Nobody else can.

 public:
  ObjectsManager(TaskConfig& taskConfig);
  virtual ~ObjectsManager();

  /**
   * Start publishing the object obj, i.e. it will be pushed forward in the workflow at regular intervals.
   * The ownership remains to the caller.
   * In most cases, objectName parameter can be ignored.
   * @param obj The object to publish.
   * @param objectName Optional, to publish something under a different name.
   */
  void startPublishing(TObject* obj, std::string objectName = "");
  // todo stoppublishing

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

  TObjArray* getNonOwningArray() const { return new TObjArray(mMonitorObjects); };

  void addMetadata(const std::string& objectName, const std::string& key, const std::string& value);

 private:
  TObjArray mMonitorObjects;
  std::string mTaskName;
};

} // namespace o2::quality_control::core

#endif // QC_CORE_OBJECTMANAGER_H
