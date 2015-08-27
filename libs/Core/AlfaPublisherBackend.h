///
/// \file   AlfaPublisherBackend.h
/// \author Barthelemy von Haller
///

#ifndef QUALITYCONTROL_LIBS_CORE_ALFAPUBLISHERBACKEND_H_
#define QUALITYCONTROL_LIBS_CORE_ALFAPUBLISHERBACKEND_H_

#include "PublisherBackendInterface.h"
#include "FairMQDevice.h"

namespace AliceO2 {
namespace QualityControl {
namespace Core {

class AlfaPublisherBackend: public PublisherBackendInterface, FairMQDevice
{
  public:
    /// Default constructor
    AlfaPublisherBackend();
    /// Destructor
    virtual ~AlfaPublisherBackend();

    void publish(MonitorObject *mo);
    static void CustomCleanup(void* data, void* hint);

  protected:
    virtual void Init();
    virtual void Run();

  private:
    std::string mText;
};

} // namespace Core
} // namespace QualityControl
} // namespace AliceO2

#endif // QUALITYCONTROL_LIBS_CORE_ALFAPUBLISHERBACKEND_H_
