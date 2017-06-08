///
/// @file    AlfaReceiverForTests.cxx
/// @author  Barthelemy von Haller
///

#include <iostream>
#include <TH1.h>
#include <TCanvas.h>

#include <FairMQDevice.h>
#include <TMessage.h>
#include "QualityControl/MonitorObject.h"
#include "QualityControl/AlfaReceiverForTests.h"

using namespace std;
using namespace AliceO2::QualityControl::Core;

namespace AliceO2 {
namespace QualityControl {
namespace Core {

AlfaReceiverForTests::AlfaReceiverForTests()
{
  OnData("data", &AlfaReceiverForTests::HandleData);
}

AlfaReceiverForTests::~AlfaReceiverForTests()
{
}

// handler is called whenever a message arrives on "data", with a reference to the message and a sub-channel index (here 0)
bool AlfaReceiverForTests::HandleData(FairMQMessagePtr &msg, int /*index*/)
{
  LOG(INFO) << "Received an object of size " << msg->GetSize();

  TestTMessage tm(msg->GetData(), msg->GetSize());
  MonitorObject *mo = dynamic_cast<MonitorObject *>(tm.ReadObject(tm.GetClass()));

  LOG(INFO) << "    Name : \"" << mo->GetName() << "\"";

//  TCanvas c;
//  mo->Draw();
//  c.SaveAs((mo->getName() + ".png").c_str());

  // return true if want to be called again (otherwise go to IDLE state)
  return true;
}
