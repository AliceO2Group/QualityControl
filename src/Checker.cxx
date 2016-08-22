///
/// \file   Checker.cxx
/// \author Barthelemy von Haller
///

#include "QualityControl/Checker.h"

// std
#include <iostream>
// boost
#include <boost/algorithm/string.hpp>
// ROOT
#include <TMessage.h>
#include <TSystem.h>
#include <TClass.h>
// FairRoot
#include <FairMQTransportFactoryZMQ.h>
#include <FairMQPoller.h>
// O2
#include "Common/Exceptions.h"
// QC
#include "QualityControl/CheckInterface.h"
#include "QualityControl/DatabaseFactory.h"

using namespace AliceO2::Common;

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
using namespace AliceO2::InfoLogger;
namespace QualityControl {
using namespace Core;
using namespace Repository;
namespace Checker {

// TODO do we need a CheckFactory ? here it is embedded in the Checker

Checker::Checker()
  : mLogger(QcInfoLogger::GetInstance())
{
  // TODO load the configuration of the database here
  mDatabase = DatabaseFactory::create("MySql");
  mDatabase->connect("localhost", "quality_control", "qc_user", "qc_user");

  mBroadcast = true; // TODO configure
  if (mBroadcast) {
    createChannel("pub", "bind", "tcp://*:5557", "data-out");
  }
}

void Checker::createChannel(std::string type, std::string method, std::string address, std::string channelName)
{
  FairMQChannel channel;
  channel.UpdateType(type);
  channel.UpdateMethod(method);
  channel.UpdateAddress(address);
  channel.UpdateRateLogging(0);
  fChannels[channelName].push_back(channel);
}

Checker::~Checker()
{
  mDatabase->disconnect();
  delete mDatabase;
}

void Checker::Run()
{
  unique_ptr <FairMQPoller> poller(fTransportFactory->CreatePoller(fChannels["data-in"]));

  while (CheckCurrentState(RUNNING)) {

    poller->Poll(100);

    for (int i = 0; i < fChannels["data-in"].size(); i++) {
      if (poller->CheckInput(i)) {
        unique_ptr <FairMQMessage> msg(fTransportFactory->CreateMessage());
        if (fChannels.at("data-in").at(i).Receive(msg) > 0) {
          mLogger << "Receiving a mo of size " << msg->GetSize() << AliceO2::InfoLogger::InfoLogger::endm;
          TestTMessage tm(msg->GetData(), msg->GetSize());
          MonitorObject *mo = dynamic_cast<MonitorObject *>(tm.ReadObject(tm.GetClass()));
          if (mo) {
            check(mo);
            store(mo);
            send(mo);
          }
        }
      }
    }
  }
}

void Checker::check(MonitorObject *mo)
{
  mLogger << "Checking \"" << mo->getName() << "\"" << AliceO2::InfoLogger::InfoLogger::endm;

  // Get the Checks
  std::vector <CheckDefinition> checks = mo->getChecks();

  // Loop over the Checks and execute them followed by the beautification
  for (const auto &check : checks) {
//    std::cout << "        check name : " << check.name << std::endl;
//    std::cout << "        check className : " << check.className << std::endl;
//    std::cout << "        check libraryName : " << check.libraryName << std::endl;

    // load module, instantiate, use check
    // TODO : preload modules and pre-instantiate, or keep a cache
    loadLibrary(check.libraryName);
    CheckInterface *checkInstance = instantiateCheck(check.name, check.className);
    Quality q = checkInstance->check(mo);

    mLogger << "        result of the check : " << q.getName() << AliceO2::InfoLogger::InfoLogger::endm;

    checkInstance->beautify(mo, q);
  }
}

void Checker::store(MonitorObject *mo)
{
  mLogger << "Storing \"" << mo->getName() << "\"" << AliceO2::InfoLogger::InfoLogger::endm;

  try {
    mDatabase->store(mo);
  } catch (boost::exception &e) {
    mLogger << "Unable to " << diagnostic_information(e) << AliceO2::InfoLogger::InfoLogger::endm;
  }
}

void Checker::CustomCleanupTMessage(void *data, void *object)
{
  delete (TMessage *) object;
}

void Checker::send(MonitorObject *mo)
{
  mLogger << "Sending \"" << mo->getName() << "\"" << AliceO2::InfoLogger::InfoLogger::endm;

  TMessage *message = new TMessage(kMESS_OBJECT);
  message->WriteObjectAny(mo, mo->IsA());
  unique_ptr <FairMQMessage> msg(NewMessage(message->Buffer(), message->BufferSize(), CustomCleanupTMessage, message));
  fChannels.at("data-out").at(0).Send(msg);
}

/// \brief Load a library.
/// \param libraryName The name of the library to load.
void Checker::loadLibrary(const string libraryName) const
{
  if (boost::algorithm::trim_copy(libraryName) == "") {
    mLogger << "no library name specified" << AliceO2::InfoLogger::InfoLogger::endm;
    return;
  }

  string library = "lib" + libraryName + ".so";
  mLogger << "Loading library " << library << AliceO2::InfoLogger::InfoLogger::endm;
  if (gSystem->Load(library.c_str())) {
    BOOST_THROW_EXCEPTION(FatalException() << errinfo_details("Failed to load Detector Publisher Library"));
  }
}

CheckInterface *Checker::instantiateCheck(string checkName, string className) const
{
  CheckInterface *result = 0;
  // Get the class and instantiate
  mLogger << "Loading class " << className << AliceO2::InfoLogger::InfoLogger::endm;
  TClass *cl = TClass::GetClass(className.c_str());
  string tempString("Failed to instantiate Quality Control Module");
  if (!cl) {
    tempString += " because no dictionary for class named \"";
    tempString += className;
    tempString += "\" could be retrieved";
    cerr << tempString << endl;
    BOOST_THROW_EXCEPTION(FatalException() << errinfo_details(tempString));
  }
  mLogger << "Instantiating class " << className << " (" << cl << ")" << AliceO2::InfoLogger::InfoLogger::endm;
  result = static_cast<CheckInterface *>(cl->New());
  if (!result) {
    tempString += " because the class named \"";
    tempString += className;
    tempString += "\" does not follow the TaskInterface interface";
    BOOST_THROW_EXCEPTION(FatalException() << errinfo_details(tempString));
  }
  result->configure(checkName);
  return result;
}

} /* namespace Checker */
} /* namespace QualityControl */
} /* namespace AliceO2 */

