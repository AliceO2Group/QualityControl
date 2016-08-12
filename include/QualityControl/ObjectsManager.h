///
/// \file   ObjectsManager.h
/// \author Barthelemy von Haller
///

#ifndef QUALITYCONTROL_LIBS_OBJECTSMANAGER_H_
#define QUALITYCONTROL_LIBS_OBJECTSMANAGER_H_

#include <string>
#include "QualityControl/Quality.h"
#include "QualityControl/MonitorObject.h"
#include "QualityControl/PublisherInterface.h"

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
    ObjectsManager(std::string publisherClassName);
    /// Destructor
    virtual ~ObjectsManager();

    void startPublishing(std::string taskName, std::string objectName, TObject *obj);
    void setQuality(std::string objectName, Quality quality);
    Quality getQuality(std::string objectName);

    /// \brief Add a check to the object defined by objectName.
    /// If a check with the same already exist for this object, it will be replaced.
    /// \param objectName
    /// \param checkName
    /// \param checkClassName
    /// \param checkLibraryName
    void addCheck (const std::string objectName, const std::string checkName,
        const std::string checkClassName, const std::string checkLibraryName="");
    MonitorObject *getMonitorObject(std::string objectName);
    TObject *getObject(std::string objectName);

  private:
    void publish();

    std::map<std::string /*object name*/, QualityControl::Core::MonitorObject * /* object */> mMonitorObjects;
    PublisherInterface* mPublisher;

};

} // namespace Core
} // namespace QualityControl
} // namespace AliceO2

#endif // QUALITYCONTROL_LIBS_OBJECTSMANAGER_H_
