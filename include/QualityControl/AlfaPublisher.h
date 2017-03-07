///
/// \file   AlfaPublisher.h
/// \author Barthelemy von Haller
///

#ifndef QUALITYCONTROL_LIBS_CORE_AlfaPublisher_H_
#define QUALITYCONTROL_LIBS_CORE_AlfaPublisher_H_

#include "QualityControl/PublisherInterface.h"
#include "FairMQDevice.h"
#include <TH1F.h>
#include "QualityControl/TaskConfig.h"

namespace AliceO2 {
namespace QualityControl {
namespace Core {

/**
 * \brief A publisher that uses FairMQ.
 * It is a device. It sends MonitorObjects.
 */
class AlfaPublisher: public PublisherInterface, FairMQDevice
{
  public:
    /// Constructor
    AlfaPublisher(TaskConfig& taskConfig);
    /// Destructor
    virtual ~AlfaPublisher();

    void publish(MonitorObject *mo);

  protected:
    virtual void Init();
    virtual void Run();
    static void CustomCleanup(void *data, void *object);
    static void CustomCleanupTMessage(void *data, void *object);
    virtual void PreRun() override {std::cout << "PreRun Alfa !!!!\n   " << fChannels["data-out"].size() << std::endl;};

  private:
    MonitorObject *mCurrentMonitorObject;
    bool mAvailableData;
};

} // namespace Core
} // namespace QualityControl
} // namespace AliceO2

#endif // QUALITYCONTROL_LIBS_CORE_AlfaPublisher_H_
