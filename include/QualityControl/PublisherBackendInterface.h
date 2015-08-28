///
/// \file   PublisherBackendInterface.h
/// \author Barthelemy von Haller
///

#ifndef QUALITYCONTROL_LIBS_CORE_PUBLISHERBACKENDINTERFACE_H_
#define QUALITYCONTROL_LIBS_CORE_PUBLISHERBACKENDINTERFACE_H_

#include "QualityControl/MonitorObject.h"

namespace AliceO2 {
namespace QualityControl {
namespace Core {

class PublisherBackendInterface
{
  public:
    /// Default constructor
    PublisherBackendInterface();
    /// Destructor
    virtual ~PublisherBackendInterface();


    virtual void publish(MonitorObject *mo) = 0;
};

} // namespace Core
} // namespace QualityControl
} // namespace AliceO2

#endif // QUALITYCONTROL_LIBS_CORE_PUBLISHERBACKENDINTERFACE_H_
