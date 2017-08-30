///
/// @file    AlfaReceiverForTests.h
/// @author  Barthelemy von Haller
///

#ifndef QUALITY_CONTROL_AlfaReceiverForTests_H
#define QUALITY_CONTROL_AlfaReceiverForTests_H

#include <FairMQDevice.h>
#include <TMessage.h>

namespace o2 {
namespace quality_control {
namespace core {

class TestTMessage : public TMessage
{
  public:
    TestTMessage(void *buf, Int_t len)
      : TMessage(buf, len)
    {
      ResetBit(kIsOwner);
    }
};

class AlfaReceiverForTests : public FairMQDevice
{
  public:
    AlfaReceiverForTests();
    virtual ~AlfaReceiverForTests();

  protected:
    bool HandleData(FairMQMessagePtr&, int);

};

} // namespace core
} // namespace QualityControl
} // namespace o2

#endif // QUALITY_CONTROL_AlfaReceiverForTests_H
