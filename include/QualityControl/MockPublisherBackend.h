///
/// \file   MockPublisherBackend.h
/// \author Barthelemy von Haller
///

#ifndef QUALITYCONTROL_LIBS_CORE_MOCKPUBLISHERBACKEND_H_
#define QUALITYCONTROL_LIBS_CORE_MOCKPUBLISHERBACKEND_H_

#include "QualityControl/PublisherBackendInterface.h"

namespace AliceO2 {
namespace QualityControl {
namespace Core {

class MockPublisherBackend: public PublisherBackendInterface
{
  public:
    /// Default constructor
    MockPublisherBackend();
    /// Destructor
    virtual ~MockPublisherBackend();

    void publish(MonitorObject *mo);
};

} // namespace Core
} // namespace QualityControl
} // namespace AliceO2

#endif // QUALITYCONTROL_LIBS_CORE_MOCKPUBLISHERBACKEND_H_
