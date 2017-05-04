///
/// @file    AlfaReceiverForTests.h
/// @author  Barthelemy von Haller
///

#ifndef QUALITY_CONTROL_AlfaReceiverForTests_H
#define QUALITY_CONTROL_AlfaReceiverForTests_H

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
    ~AlfaReceiverForTests();

  protected:
    void Run() override;
//    bool HandleData(FairMQMessagePtr&, int);

};

} // namespace Core
} // namespace QualityControl
} // namespace AliceO2

#endif // QUALITY_CONTROL_AlfaReceiverForTests_H
