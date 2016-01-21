///
/// @file    Receiver.cxx
/// @author  Barthelemy von Haller
///

#include <iostream>
#include <TH1.h>
#include <TCanvas.h>
#include "FairMQProgOptions.h"

#include <FairMQDevice.h>
#include <TMessage.h>
#include <FairMQTransportFactoryZMQ.h>
#include "QualityControl/MonitorObject.h"

using namespace std;
using namespace AliceO2::QualityControl::Core;

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
  public :
    AlfaReceiverForTests()
    {
    }

    ~AlfaReceiverForTests()
    {
    }

    void Run()
    {
      // TODO : clean up memory

      while (CheckCurrentState(RUNNING)) {
        unique_ptr<FairMQMessage> msg2(fTransportFactory->CreateMessage());

        if (fChannels.at("data-in").at(0).Receive(msg2) > 0) {
          cout << "Receiving an histo " << endl;
          cout << "msg2->GetSize() " << msg2->GetSize() << endl;
          TestTMessage tm(msg2->GetData(), msg2->GetSize());
          MonitorObject *mo = dynamic_cast<MonitorObject *>(tm.ReadObject(tm.GetClass()));
//          TH1 *histo = dynamic_cast<TH1 *>(tm.ReadObject(tm.GetClass()));
          TCanvas c;
          cout << "mo : " << mo << endl;
          cout << "mo->getObject() : " << mo->getObject() << endl;
//          cout << "dynamic_cast<TH1*>(mo->getObject()) : " << dynamic_cast<TH1*>(mo->getObject()) << endl;
//          cout << "   name : " << mo->GetName() << " ; mean : " <<  dynamic_cast<TH1*>(mo->getObject())->GetMean() << endl;
//          histo->Draw();
          mo->Draw();
          c.SaveAs("test.png");
        }
      }
    }
};

} // namespace Core
} // namespace QualityControl
} // namespace AliceO2

using namespace AliceO2::QualityControl::Core;

int main(int argc, char *argv[])
{

  cout << "hello" << endl;
  AlfaReceiverForTests receiver;
  FairMQProgOptions config;

  try {
    FairMQChannel histoChannel;
    histoChannel.UpdateType("pull");
    histoChannel.UpdateMethod("connect");
    histoChannel.UpdateAddress("tcp://localhost:5556");
    histoChannel.UpdateSndBufSize(10000);
    histoChannel.UpdateRcvBufSize(10000);
    histoChannel.UpdateRateLogging(0);
    receiver.fChannels["data-in"].push_back(histoChannel);

    // Get the proper transport factory
#ifdef NANOMSG
    FairMQTransportFactory* transportFactory = new FairMQTransportFactoryNN();
#else
    FairMQTransportFactory *transportFactory = new FairMQTransportFactoryZMQ();
#endif

    receiver.SetTransport(transportFactory);

    receiver.ChangeState("INIT_DEVICE");
    receiver.WaitForEndOfState("INIT_DEVICE");

    receiver.ChangeState("INIT_TASK");
    receiver.WaitForEndOfState("INIT_TASK");

    receiver.ChangeState("RUN");
    receiver.InteractiveStateLoop();
  }
  catch (std::exception &e) {
    cout << e.what();
    cout << "Command line options are the following: ";
    config.PrintHelp();
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}