///
/// \file   AlfaPublisherBackend.h
/// \author Barthelemy von Haller
///

#ifndef QUALITYCONTROL_LIBS_CORE_ALFAPUBLISHERBACKEND_H_
#define QUALITYCONTROL_LIBS_CORE_ALFAPUBLISHERBACKEND_H_

#include "QualityControl/PublisherBackendInterface.h"
#include "FairMQDevice.h"
#include <TH1F.h>

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

  protected:
    virtual void Init();
    virtual void Run();
    static void CustomCleanup(void *data, void *object);
    static void CustomCleanupTMessage(void *data, void *object);

  private:
    std::string mText;
    TH1F * th1;
};

} // namespace Core
} // namespace QualityControl
} // namespace AliceO2

#endif // QUALITYCONTROL_LIBS_CORE_ALFAPUBLISHERBACKEND_H_
