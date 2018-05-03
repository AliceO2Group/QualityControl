// Copyright CERN and copyright holders of ALICE O2. This software is
// distributed under the terms of the GNU General Public License v3 (GPL
// Version 3), copied verbatim in the file "COPYING".
//
// See http://alice-o2.web.cern.ch/license for full licensing information.
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.

///
/// \file   SpyDevice.cxx
/// \author Barthelemy von Haller
///

#include <iostream>
#include <chrono>
#include <thread>
#include <boost/algorithm/string.hpp>
#include <FairMQDevice.h>
#include <TMessage.h>
#include <TSystem.h>
#include "QualityControl/SpyDevice.h"
#include "QualityControl/MonitorObject.h"

using namespace std;
using namespace o2::quality_control::core;

namespace o2 {
namespace quality_control {
namespace gui {

SpyDevice::SpyDevice()
  : mFrame(nullptr)
{
  SetTransport("zeromq"); // or "nanomsg", etc
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
      if (tobj != nullptr) {
        // TODO once the bug in ROOt that removes spaces in strings passed in signal slot is fixed we can use the normal name.
        string objectName = tobj->GetName();
        auto *mo = dynamic_cast<MonitorObject*>(tobj);
        Quality q = mo ? mo->getQuality() : Quality::Null;
        boost::erase_all(objectName, " ");
        if (mCache.count(objectName) > 0 && mCache[objectName] != nullptr) {
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
  receivingChannel.UpdateSndBufSize(100);
  receivingChannel.UpdateRcvBufSize(100);
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

} // namespace core
} // namespace quality_control
} // namespace o2

