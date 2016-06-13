///
/// @file    SpyDevice.h
/// @author  Barthelemy von Haller
///

#ifndef QUALITY_CONTROL_SpyDevice_H
#define QUALITY_CONTROL_SpyDevice_H

#include <FairMQDevice.h>
#include <TMessage.h>
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

    virtual ~SpyDevice();

    static void CustomCleanup(void *data, void* hint);
    void start();
    void stop();
    void displayObject(const char* objectName); // not a string because it is a slot

    void setFrame(SpyMainFrame *frame);

  protected:
    virtual void Run() override;

  private:
    SpyMainFrame *mFrame;
    std::map<std::string, TObject*> mCache;

  ClassDef(SpyDevice,1)
    ;
};

} // namespace Gui
} // namespace QualityControl
} // namespace AliceO2

#endif // QUALITY_CONTROL_SpyDevice_H
