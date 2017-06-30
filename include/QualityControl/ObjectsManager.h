///
/// \file   ObjectsManager.h
/// \author Barthelemy von Haller
///

#ifndef QUALITYCONTROL_LIBS_OBJECTSMANAGER_H_
#define QUALITYCONTROL_LIBS_OBJECTSMANAGER_H_

#include <string>
#include <boost/concept_check.hpp>
#include "QualityControl/Quality.h"
#include "QualityControl/MonitorObject.h"
#include "QualityControl/TaskConfig.h"

namespace AliceO2 {
namespace QualityControl {
namespace Core {

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
    /// Default constructor
    ObjectsManager();

    ObjectsManager(TaskConfig &taskConfig);

    /// Destructor
    virtual ~ObjectsManager();

    void startPublishing(TObject *obj, std::string objectName = "");

    void setQuality(std::string objectName, Quality quality);

    Quality getQuality(std::string objectName);

    /// \brief Add a check to the object defined by objectName.
    /// If a check with the same already exist for this object, it will be replaced.
    /// \param objectName
    /// \param checkName
    /// \param checkClassName
    /// \param checkLibraryName
    void addCheck(const std::string &objectName, const std::string &checkName,
                  const std::string &checkClassName, const std::string &checkLibraryName = "");

    void addCheck(const TObject *object, const std::string &checkName,
                  const std::string &checkClassName, const std::string &checkLibraryName = "");

    MonitorObject *getMonitorObject(std::string objectName);

    TObject *getObject(std::string objectName);

    typedef typename std::map<std::string, QualityControl::Core::MonitorObject *>::iterator iterator;
    typedef typename std::map<std::string, QualityControl::Core::MonitorObject *>::const_iterator const_iterator;

    const_iterator begin() const
    { return mMonitorObjects.begin(); }

    const_iterator end() const
    { return mMonitorObjects.end(); }

    iterator begin()
    { return mMonitorObjects.begin(); }

    iterator end()
    { return mMonitorObjects.end(); }

  private:
    std::map<std::string /*object name*/, QualityControl::Core::MonitorObject * /* object */> mMonitorObjects;
    std::string mTaskName;
};

} // namespace Core
} // namespace QualityControl
} // namespace AliceO2

#endif // QUALITYCONTROL_LIBS_OBJECTSMANAGER_H_
