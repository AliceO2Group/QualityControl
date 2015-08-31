///
/// \file   AlfaPublisherBackend.cxx
/// \author Barthelemy von Haller
///

#include "QualityControl/AlfaPublisherBackend.h"
#include "FairMQTransportFactoryZMQ.h"

using namespace std;

namespace AliceO2 {
namespace QualityControl {
namespace Core {

AlfaPublisherBackend::AlfaPublisherBackend()
  : mText("asdf")
{
  // TODO Auto-generated constructor stub
#ifdef NANOMSG
  FairMQTransportFactory* transportFactory = new FairMQTransportFactoryNN();
#else
  FairMQTransportFactory *transportFactory = new FairMQTransportFactoryZMQ();
#endif
  SetTransport(transportFactory);

  //SetProperty(O2EPNex::Id, options.id);

  FairMQChannel outputChannel("pub" /*"Output socket type: pub/push"*/, "bind" /* "Output method: bind/connect"*/, "tcp://localhost:5555" /*output address*/);
  outputChannel.UpdateSndBufSize(2); // "Output buffer size in number of messages (ZeroMQ)
//  outputChannel.UpdateRcvBufSize(2);
  outputChannel.UpdateRateLogging(1);
  fChannels["data-out"].push_back(outputChannel);

}

AlfaPublisherBackend::~AlfaPublisherBackend()
{
}

void AlfaPublisherBackend::CustomCleanup(void *data, void *object)
{
  delete (std::string *) object;
}

void AlfaPublisherBackend::publish(MonitorObject *mo)
{
  std::string *text = new std::string(mText);
  FairMQMessage *msg = fTransportFactory->CreateMessage(const_cast<char *>(text->c_str()), text->length(),
                                                        CustomCleanup,
                                                        text);
  cout << "test : " << fChannels["data-out"].at(0).GetType() << endl;
  fChannels["data-out"].at(0).Send(msg);
}

void AlfaPublisherBackend::Init()
{
  cout << "Init()" << endl;
}

void AlfaPublisherBackend::Run()
{
  cout << "Run()" << endl;
}

} // namespace Core
} // namespace QualityControl
} // namespace AliceO2

