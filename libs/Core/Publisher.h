///
/// \file   Publisher.h
/// \author Barthelemy von Haller
///

#ifndef QUALITYCONTROL_LIBS_PUBLISHER_PUBLISHER_H_
#define QUALITYCONTROL_LIBS_PUBLISHER_PUBLISHER_H_

#include <string>
#include "Quality.h"
#include "MonitorObject.h"

namespace AliceO2 {
namespace QualityControl {
namespace Core {

/// \brief   Contains the list of objects to publish and does the actual publication.
///
/// Keeps a list of the objects to publish, encapsulates them and does the actual publication.
///
/// \author   Barthelemy von Haller
class Publisher
{
  public:
    /// Default constructor
    Publisher();
    /// Destructor
    virtual ~Publisher();

    void startPublishing(std::string objectName, void* object); // TODO what type here ?
    void setQuality(std::string objectName, Quality quality);
    Quality getQuality(std::string objectName);
    void addChecker(std::string objectName, std::string checkerName, std::string checkerClassName);
    MonitorObject* getMonitorObject(std::string objectName);
    void* getObject(std::string objectName);

  private:
    std::map<std::string /*object name*/, QualityControl::Core::MonitorObject* /* object */> mMonitorObjects;
};

} /* namespace Publisher */
} /* namespace QualityControl */
} /* namespace AliceO2 */

#endif /* QUALITYCONTROL_LIBS_PUBLISHER_PUBLISHER_H_ */
