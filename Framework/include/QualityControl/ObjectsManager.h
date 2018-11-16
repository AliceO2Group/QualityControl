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

namespace o2
{
namespace quality_control
{
namespace core
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
  void startPublishing(TObject* obj, std::string objectName = "");
  // todo stoppublishing
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

  std::string getObjectsListString() { return mObjectsList.GetString().Data(); }

 private:
  void UpdateIndex(const std::string& nonEmptyName);

 private:
  TObjArray mMonitorObjects;
  std::string mTaskName;
  // todo make it a vector of string when support added
  TObjString mObjectsList; // the list of objects we publish. (comma separated)
                           // Possibly needed on the client side to know what was there at a given time.
};

} // namespace core
} // namespace quality_control
} // namespace o2

#endif // QC_CORE_OBJECTMANAGER_H
