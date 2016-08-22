///
/// \file   AlfaPublisher.cxx
/// \author Barthelemy von Haller
///

#include "QualityControl/AlfaPublisher.h"
#include "FairMQTransportFactoryZMQ.h"
#include "TMessage.h"
#include "TH1F.h"
#include "FairMQProgOptions.h"
#include "FairMQParser.h"
#include "TClass.h"

using namespace std;

namespace AliceO2 {
namespace QualityControl {
namespace Core {

AlfaPublisher::AlfaPublisher(TaskConfig& taskConfig)
    : mCurrentMonitorObject(0)
{
  // set up communication layout and properties
  FairMQChannel histoChannel;
  histoChannel.UpdateType("pub");
  histoChannel.UpdateMethod("bind");
  cout << "address > " << taskConfig.address << endl;
  histoChannel.UpdateAddress(taskConfig.address);
  histoChannel.UpdateSndBufSize(10000);
  histoChannel.UpdateRcvBufSize(10000);
  histoChannel.UpdateRateLogging(0);
  fChannels["data-out"].push_back(histoChannel);

  // Get the transport layer
#ifdef NANOMSG
  FairMQTransportFactory* transportFactory = new FairMQTransportFactoryNN();
#else
  FairMQTransportFactory *transportFactory = new FairMQTransportFactoryZMQ();
#endif
  SetTransport(transportFactory);

  ChangeState(INIT_DEVICE);
  WaitForEndOfState(INIT_DEVICE);

  ChangeState(INIT_TASK);
  WaitForEndOfState(INIT_TASK);
}

AlfaPublisher::~AlfaPublisher()
{
  ChangeState(RESET_TASK);
  WaitForEndOfState(RESET_TASK);

  ChangeState(RESET_DEVICE);
  WaitForEndOfState(RESET_DEVICE);

  ChangeState(END);
}

// helper function to clean up the object holding the data after it is transported.
void AlfaPublisher::CustomCleanup(void *data, void *object)
{
//  delete static_cast<TMessage*>(object); // Does it delete the TH1F ?
}

void AlfaPublisher::CustomCleanupTMessage(void *data, void *object)
{
  delete (TMessage *) object;
}

void AlfaPublisher::publish(MonitorObject *mo)
{

  mCurrentMonitorObject = mo;
//  cout << "alfa publisher : " << mo->getName() << " ; mean of histo : "
//      << dynamic_cast<TH1*>(mo->getObject())->GetMean() << endl;

  ChangeState(RUN);
  WaitForEndOfState(RUN);
}

void AlfaPublisher::Init()
{
//  cout << "Init()" << endl;
}

void AlfaPublisher::Run()
{

  // this is called when the state change to RUN, i.e. when we call publish

  TMessage *message = new TMessage(kMESS_OBJECT); // will be deleted by fairmq using our custom method
  message->WriteObjectAny(mCurrentMonitorObject, mCurrentMonitorObject->IsA());
//  cout << "mCurrentMonitorObject->IsA() : " << mCurrentMonitorObject->IsA()->GetName() << endl;
  unique_ptr<FairMQMessage> msg(NewMessage(message->Buffer(), message->BufferSize(), CustomCleanupTMessage, message));
//  cout << "msg->GetSize() " << msg->GetSize() << endl;

//  std::cout << "Sending \"" << mCurrentMonitorObject->getName() << "\"" << std::endl;

  fChannels.at("data-out").at(0).Send(msg);
}

} // namespace Core
} // namespace QualityControl
} // namespace AliceO2

