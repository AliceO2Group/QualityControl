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

namespace AliceO2 {
namespace QualityControl {
namespace Gui {

SpyDevice::SpyDevice()
    : mFrame(nullptr)
{
  this->SetTransport(new FairMQTransportFactoryZMQ);

  FairMQChannel receivingChannel;
  receivingChannel.UpdateType("sub");
  receivingChannel.UpdateMethod("connect");
  receivingChannel.UpdateAddress("tcp://localhost:5556"); // todo get this from the UI
  receivingChannel.UpdateSndBufSize(10000);
  receivingChannel.UpdateRcvBufSize(10000);
  receivingChannel.UpdateRateLogging(0);
  fChannels["data-in"].push_back(receivingChannel);
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
}

void SpyDevice::stop()
{
  ChangeState("STOP"); // synchronous
  ChangeState("RESET_TASK");
  WaitForEndOfState("RESET_TASK");
  ChangeState("RESET_DEVICE");
  WaitForEndOfState("RESET_DEVICE");
  ChangeState("END");
}

void SpyDevice::Run()
{
  while (CheckCurrentState(RUNNING)) {
    unique_ptr<FairMQMessage> message(fTransportFactory->CreateMessage());

    if (fChannels.at("data-in").at(0).ReceiveAsync(message) > 0) {
      TestTMessage tm(message->GetData(), message->GetSize());
      TObject *tobj = tm.ReadObject(tm.GetClass());
      if (tobj) {
        // TODO once the bug in ROOt that removes spaces in strings passed in signal slot is fixed we can use the normal name.
        string objectName = tobj->GetName();
        boost::erase_all(objectName, " ");
        if(mCache.count(objectName) && mCache[objectName]) {
          delete mCache[objectName];
        }
        mCache[objectName] = tobj;
        mFrame->updateList(objectName);
      } else {
        cerr << "not a tobject !" << endl;
      }
    }
    this_thread::sleep_for(chrono::milliseconds(1000));
  }
}

void SpyDevice::displayObject(const char* objectName)
{
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

