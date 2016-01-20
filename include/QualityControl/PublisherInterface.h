///
/// \file   PublisherInterface.h
/// \author Barthelemy von Haller
///

#ifndef QUALITYCONTROL_LIBS_CORE_PublisherInterface_H_
#define QUALITYCONTROL_LIBS_CORE_PublisherInterface_H_

#include "QualityControl/MonitorObject.h"

namespace AliceO2 {
namespace QualityControl {
namespace Core {

class PublisherInterface
{
  public:
    /// Default constructor
    PublisherInterface();
    /// Destructor
    virtual ~PublisherInterface();


    virtual void publish(MonitorObject *mo) = 0;
};

} // namespace Core
} // namespace QualityControl
} // namespace AliceO2

#endif // QUALITYCONTROL_LIBS_CORE_PublisherInterface_H_
