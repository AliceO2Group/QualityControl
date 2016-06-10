///
/// \file   SpyDevice.cxx
/// \author Barthelemy von Haller
///

#include <iostream>
#include <TH1.h>
#include <TCanvas.h>

#include <FairMQDevice.h>
#include <TMessage.h>
#include <FairMQTransportFactoryZMQ.h>
#include "QualityControl/SpyDevice.h"
#include <TSystem.h>
#include <chrono>
#include <thread>

#include <boost/algorithm/string.hpp>

using namespace std;
using namespace AliceO2::QualityControl::Core;

namespace AliceO2 {
namespace QualityControl {
namespace Gui {

SpyDevice::SpyDevice()
    : mFrame(nullptr)
{
  this->SetTransport(new FairMQTransportFactoryZMQ);

  FairMQChannel histoChannel;
  histoChannel.UpdateType("sub");
  histoChannel.UpdateMethod("connect");
  histoChannel.UpdateAddress("tcp://localhost:5556"); // todo get this from the UI
  histoChannel.UpdateSndBufSize(10000);
  histoChannel.UpdateRcvBufSize(10000);
  histoChannel.UpdateRateLogging(0);
  fChannels["data-in"].push_back(histoChannel);
}

SpyDevice::~SpyDevice()
{
}

void SpyDevice::setFrame(SpyMainFrame *frame)
{
  mFrame = frame;
}

void SpyDevice::start()
{
  ChangeState("INIT_DEVICE");
  WaitForEndOfState("INIT_DEVICE");
  ChangeState("INIT_TASK");
  WaitForEndOfState("INIT_TASK");
  ChangeState("RUN");
//  InteractiveStateLoop();
}

void SpyDevice::Run()
{
  cout << "in run" << endl;
  while (CheckCurrentState(RUNNING)) {
    unique_ptr<FairMQMessage> message(fTransportFactory->CreateMessage());

    if (fChannels.at("data-in").at(0).ReceiveAsync(message) > 0) {
      cout << "Receiving an histo " << endl;
      cout << "msg2->GetSize() " << message->GetSize() << endl;
      TestTMessage tm(message->GetData(), message->GetSize());
      MonitorObject *mo = dynamic_cast<MonitorObject *>(tm.ReadObject(tm.GetClass()));
      if (mo) {
        cout << "mo->getObject()->GetName() : " << mo->getObject()->GetName() << endl;
        // TODO do we delete the old object ?
        // TODO once the bug in ROOt that removes spaces in strings passed in signal slot is fixed we can use the normal name.
        string objectName = mo->getName();
        boost::erase_all(objectName, " ");
        cout << "* adding mo '" << objectName << "' to cache " << endl;
        mCache[objectName] = mo;
        mFrame->updateList(objectName);
      } else {
        cerr << "not a MonitorObject !" << endl;
      }
    }
    this_thread::sleep_for(chrono::milliseconds(1000));
  }
  cout << "out run " << endl;
}

void SpyDevice::displayObject(const char* objectName)
{
  cout << "*display object (slot) " << objectName << endl;
  if (mCache.count(objectName) > 0) {
    mFrame->displayObject(mCache[objectName]);
  }
  else {
    cout << "object unknown" << endl;
  }
}

} // namespace Core
} // namespace QualityControl
} // namespace AliceO2

