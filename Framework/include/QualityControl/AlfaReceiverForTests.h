///
/// @file    AlfaReceiverForTests.h
/// @author  Barthelemy von Haller
///

#ifndef QC_CORE_ALFARECEIVERFORTESTS_H
#define QC_CORE_ALFARECEIVERFORTESTS_H

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
} // namespace quality_control
} // namespace o2

#endif // QC_CORE_ALFARECEIVERFORTESTS_H
