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
    /// Destructor
    virtual ~ObjectsManager();

    void startPublishing(std::string objectName, TObject *obj);
    void setQuality(std::string objectName, Quality quality);
    Quality getQuality(std::string objectName);
    void addChecker(std::string objectName, std::string checkerName, std::string checkerClassName);
    MonitorObject *getMonitorObject(std::string objectName);
    TObject *getObject(std::string objectName);

  private:
    void publish();

    std::map<std::string /*object name*/, QualityControl::Core::MonitorObject * /* object */> mMonitorObjects;
    PublisherInterface* mBackend;
};

} // namespace Core
} // namespace QualityControl
} // namespace AliceO2

#endif // QUALITYCONTROL_LIBS_OBJECTSMANAGER_H_
