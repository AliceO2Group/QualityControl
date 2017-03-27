///
/// \file   MockPublisher.h
/// \author Barthelemy von Haller
///

#ifndef QUALITYCONTROL_LIBS_CORE_MOCKPUBLISHER_H_
#define QUALITYCONTROL_LIBS_CORE_MOCKPUBLISHER_H_

#include "QualityControl/PublisherInterface.h"

namespace AliceO2 {
namespace QualityControl {
namespace Core {

class MockPublisher: public PublisherInterface
{
  public:
    /// Default constructor
    MockPublisher();
    /// Destructor
    ~MockPublisher() override;

    void publish(MonitorObject *mo) override;
};

} // namespace Core
} // namespace QualityControl
} // namespace AliceO2

#endif // QUALITYCONTROL_LIBS_CORE_MOCKPUBLISHER_H_
