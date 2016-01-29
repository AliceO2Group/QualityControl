///
/// @file    AlfaReceiverForTests.h
/// @author  Barthelemy von Haller
///

#ifndef PROJECT_TEMPLATE_AlfaReceiverForTests_H
#define PROJECT_TEMPLATE_AlfaReceiverForTests_H

#include <FairMQDevice.h>
#include <TMessage.h>

namespace AliceO2 {
namespace QualityControl {
namespace Core {

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
    virtual void Run() override;

};

} // namespace Core
} // namespace QualityControl
} // namespace AliceO2

#endif // PROJECT_TEMPLATE_HELLO_RECEIVER_H
