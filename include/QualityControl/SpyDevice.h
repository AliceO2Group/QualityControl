///
/// @file    SpyDevice.h
/// @author  Barthelemy von Haller
///

#ifndef QUALITY_CONTROL_SpyDevice_H
#define QUALITY_CONTROL_SpyDevice_H

// ROOT
#include <TMessage.h>
// FairRoot
#include <FairMQDevice.h>
// QC
#include "SpyMainFrame.h"

namespace AliceO2 {
namespace QualityControl {
namespace Gui {

class TestTMessage: public TMessage
{
  public:
    TestTMessage(void *buf, Int_t len)
        : TMessage(buf, len)
    {
      ResetBit(kIsOwner);
    }
};

/**
 * \details It is the Controller and the Model (the cache map). The View is the SpyMainFrame.
 */
class SpyDevice: public FairMQDevice
{
  public:
    SpyDevice();

    ~SpyDevice() override;

    void stopSpy();
    void displayObject(std::string objectName);

    void setFrame(SpyMainFrame *frame);
    /**
     * \param address <url>:<port>
     */
    void startChannel(std::string address, std::string type);
    void stopChannel();

  protected:
    void Run() override;

  private:
    SpyMainFrame *mFrame;
    std::map<std::string, TObject*> mCache;

//  ClassDef(SpyDevice,1);
};

} // namespace Gui
} // namespace QualityControl
} // namespace AliceO2

#endif // QUALITY_CONTROL_SpyDevice_H
