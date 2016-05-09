///
/// \file   Checker.cxx
/// \author Barthelemy von Haller
///

#include "QualityControl/Checker.h"

#include <iostream>
#include <FairMQTransportFactoryZMQ.h>
#include <TMessage.h>
#include <TBuffer.h>

class TestTMessage : public TMessage
{
  public:
    TestTMessage(void *buf, Int_t len)
      : TMessage(buf, len)
    {
      ResetBit(kIsOwner);
    }
};

using namespace std;

namespace AliceO2 {
namespace QualityControl {
using namespace Core;
namespace Checker {

Checker::Checker()
{
}

Checker::~Checker()
{
}

void Checker::Run()
{
  while (CheckCurrentState(RUNNING)) {

    unique_ptr<FairMQMessage> msg(fTransportFactory->CreateMessage());

    if (fChannels.at("data-in").at(0).Receive(msg) > 0) {
      cout << "Receiving an histo " << endl;
      cout << "msg2->GetSize() " << msg->GetSize() << endl;
      TestTMessage tm(msg->GetData(), msg->GetSize());
      MonitorObject *mo = dynamic_cast<MonitorObject *>(tm.ReadObject(tm.GetClass()));
      if (mo) {
        cout << "mo : " << mo << endl;
        check(mo);
      }
    }
  }
}

void Checker::check(MonitorObject *mo)
{
  // Get the Checks
  std::map<std::string, std::string> checks = mo->getChecks();

  cout << "******** check mo : " << checks.size() << endl;

  // Loop over the Checks and execute them followed by the beautification
  for (const auto& checkTuple : checks) {
    std::cout << checkTuple.first << " has value " << checkTuple.second << std::endl;

    // TODO : load module, instantiate, use check
    // TODO : preload modules and pre-instantiate, or keep a cache

  }
}

} /* namespace Checker */
} /* namespace QualityControl */
} /* namespace AliceO2 */

