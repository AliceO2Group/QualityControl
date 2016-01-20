///
/// \file   AlfaPublisher.h
/// \author Barthelemy von Haller
///

#ifndef QUALITYCONTROL_LIBS_CORE_AlfaPublisher_H_
#define QUALITYCONTROL_LIBS_CORE_AlfaPublisher_H_

#include "QualityControl/PublisherInterface.h"
#include "FairMQDevice.h"
#include <TH1F.h>

namespace AliceO2 {
namespace QualityControl {
namespace Core {

class AlfaPublisher: public PublisherInterface, FairMQDevice
{
  public:
    /// Default constructor
    AlfaPublisher();
    /// Destructor
    virtual ~AlfaPublisher();

    void publish(MonitorObject *mo);

  protected:
    virtual void Init();
    virtual void Run();
    static void CustomCleanup(void *data, void *object);
    static void CustomCleanupTMessage(void *data, void *object);

  private:
    std::string mText;
    MonitorObject *mCurrentMonitorObject;
};

} // namespace Core
} // namespace QualityControl
} // namespace AliceO2

#endif // QUALITYCONTROL_LIBS_CORE_AlfaPublisher_H_
