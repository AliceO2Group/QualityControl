///
/// \file   SpyDevice.cxx
/// \author Barthelemy von Haller
///

#include <iostream>

#include <FairMQDevice.h>
#include <TMessage.h>
#include "QualityControl/SpyDevice.h"
#include "QualityControl/MonitorObject.h"
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
  SetTransport("zeromq"); // or "nanomsg", etc
}

SpyDevice::~SpyDevice()
{
}

void SpyDevice::setFrame(SpyMainFrame *frame)
{
  mFrame = frame;
}

void SpyDevice::stopSpy()
{
  // TODO if current state is running {   CheckCurrentState(RUNNING)?
  ChangeState("STOP"); // synchronous
  ChangeState("RESET_TASK");
  WaitForEndOfState("RESET_TASK");
  ChangeState("RESET_DEVICE");
  WaitForEndOfState("RESET_DEVICE");
  //}
  ChangeState("END");
}

void SpyDevice::Run()
{
  while (CheckCurrentState(RUNNING)) {
    unique_ptr<FairMQMessage> message(fTransportFactory->CreateMessage());

    while (fChannels.at("data-in").at(0).ReceiveAsync(message) > 0) {
      TestTMessage tm(message->GetData(), message->GetSize());
      TObject *tobj = tm.ReadObject(tm.GetClass());
      if (tobj) {
        // TODO once the bug in ROOt that removes spaces in strings passed in signal slot is fixed we can use the normal name.
        string objectName = tobj->GetName();
        MonitorObject *mo = dynamic_cast<MonitorObject*>(tobj);
        Quality q = mo ? mo->getQuality() : Quality::Null;
        boost::erase_all(objectName, " ");
        if (mCache.count(objectName) && mCache[objectName]) {
          delete mCache[objectName];
        }
        mCache[objectName] = tobj;
        mFrame->updateList(objectName/*, q*/); // TODO give quality to show in the gui
      } else {
        cerr << "not a tobject !" << endl;
      }
    }
    this_thread::sleep_for(chrono::milliseconds(1000));
  }
}

void SpyDevice::displayObject(string objectName)
{
  if (mCache.count(objectName) > 0) {
    mFrame->displayObject(mCache[objectName]);
  } else {
    cout << "object unknown" << endl;
  }
}

void SpyDevice::startChannel(std::string address, std::string type)
{
  FairMQChannel receivingChannel;
  receivingChannel.UpdateType(type);
  receivingChannel.UpdateAddress(address);
  receivingChannel.UpdateSndBufSize(10000);
  receivingChannel.UpdateRcvBufSize(10000);
  receivingChannel.UpdateRateLogging(0);
  receivingChannel.UpdateMethod("connect");
  fChannels["data-in"].push_back(receivingChannel);
  ChangeState("INIT_DEVICE");
  WaitForEndOfState("INIT_DEVICE");
  ChangeState("INIT_TASK");
  WaitForEndOfState("INIT_TASK");
  ChangeState("RUN");
}

void SpyDevice::stopChannel()
{
  if (CheckCurrentState(RUNNING)) {
    ChangeState("STOP"); // synchronous
    ChangeState("RESET_TASK");
    WaitForEndOfState("RESET_TASK");
    ChangeState("RESET_DEVICE");
    WaitForEndOfState("RESET_DEVICE");
    fChannels["data-in"].pop_back();
  }
}

} // namespace Core
} // namespace QualityControl
} // namespace AliceO2

