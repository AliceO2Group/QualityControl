///
/// @file    AlfaReceiverForTests.cxx
/// @author  Barthelemy von Haller
///

#include <iostream>
#include <TH1.h>
#include <TCanvas.h>

#include <FairMQDevice.h>
#include <TMessage.h>
//#include <FairMQTransportFactoryZMQ.h>
#include "QualityControl/MonitorObject.h"
#include "QualityControl/AlfaReceiverForTests.h"

using namespace std;
using namespace AliceO2::QualityControl::Core;

namespace AliceO2 {
namespace QualityControl {
namespace Core {

AlfaReceiverForTests::AlfaReceiverForTests()
{
//  OnData("data", &AlfaReceiverForTests::HandleData);
}

AlfaReceiverForTests::~AlfaReceiverForTests()
{
}


// handler is called whenever a message arrives on "data", with a reference to the message and a sub-channel index (here 0)
//bool AlfaReceiverForTests::HandleData(FairMQMessagePtr &msg, int /*index*/)
//{
//  LOG(INFO) << "Received: \"" << string(static_cast<char *>(msg->GetData()), msg->GetSize()) << "\"";
//
//  // return true if want to be called again (otherwise go to IDLE state)
//  return true;
//}

void AlfaReceiverForTests::Run()
{
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
          c.SaveAs((mo->getName() + ".png").c_str());
        }
  }
}

} // namespace Core
} // namespace QualityControl
} // namespace AliceO2

//using namespace AliceO2::QualityControl::Core;
//
//int main(int argc, char *argv[])
//{
//
//  cout << "hello" << endl;
//  AlfaReceiverForTests receiver;
//
//  try {
//    FairMQChannel histoChannel;
//    histoChannel.UpdateType("sub");
//    histoChannel.UpdateMethod("connect");
//    histoChannel.UpdateAddress("tcp://localhost:5556");
//    histoChannel.UpdateSndBufSize(10000);
//    histoChannel.UpdateRcvBufSize(10000);
//    histoChannel.UpdateRateLogging(0);
//    receiver.fChannels["data-in"].push_back(histoChannel);
//
//    // Get the proper transport factory
//#ifdef NANOMSG
//    FairMQTransportFactory* transportFactory = new FairMQTransportFactoryNN();
//#else
//    FairMQTransportFactory *transportFactory = new FairMQTransportFactoryZMQ();
//#endif
//
//    receiver.SetTransport(transportFactory);
//
//    receiver.ChangeState("INIT_DEVICE");
//    receiver.WaitForEndOfState("INIT_DEVICE");
//
//    receiver.ChangeState("INIT_TASK");
//    receiver.WaitForEndOfState("INIT_TASK");
//
//    receiver.ChangeState("RUN");
//    receiver.InteractiveStateLoop();
//  }
//  catch (std::exception &e) {
//    cout << e.what();
//    return EXIT_FAILURE;
//  }
//
//  return EXIT_SUCCESS;
//}
