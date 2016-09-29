///
/// \file   Checker.cxx
/// \author Barthelemy von Haller
///

#include "QualityControl/Checker.h"

// std
#include <iostream>
#include <chrono>
#include <algorithm>
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
#include "Common/Timer.h"
// QC
#include "QualityControl/CheckInterface.h"
#include "QualityControl/DatabaseFactory.h"
#include "QualityControl/CheckerConfig.h"

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
using namespace AliceO2::InfoLogger;
using namespace std::chrono;

namespace AliceO2 {
namespace QualityControl {
using namespace Core;
using namespace Repository;
namespace Checker {

// TODO do we need a CheckFactory ? here it is embedded in the Checker

Checker::Checker(std::string checkerName, std::string configurationSource)
  : mLogger(QcInfoLogger::GetInstance()), mTotalNumberHistosReceived(0)
{
  // configuration
  ConfigFile configFile;
  configFile.load(configurationSource);
  populateConfig(configFile, checkerName);

  // monitoring
  mCollector = std::shared_ptr<Monitoring::Core::Collector>(new Monitoring::Core::Collector(configurationSource));
  //mMonitor = std::unique_ptr<Monitoring::Core::ProcessMonitor>(
   // new Monitoring::Core::ProcessMonitor(mCollector, configFile));
  //mMonitor->startMonitor();

  // load the configuran of the database here
  mDatabase = DatabaseFactory::create("MySql");
  mDatabase->connect(configFile.getValue<string>("database.host"), configFile.getValue<string>("database.name"), configFile.getValue<string>("database.username"), configFile.getValue<string>("database.password"));

  if (mCheckerConfig.broadcast) {
    createChannel("pub", "bind", mCheckerConfig.broadcastAddress, "data-out");
  }
}

void Checker::populateConfig(ConfigFile &configFile, std::string checkerName)
{
  mCheckerConfig.checkerName = checkerName;
  try {
    mCheckerConfig.broadcast = (bool) configFile.getValue<int>(checkerName + ".broadcast");
    mCheckerConfig.broadcastAddress = configFile.getValue<string>(checkerName + ".broadcastAddress");
  } catch (const std::string &s) {
    // ignore, we don't care that it is not there. Would be nice to have a proper way to test a key.
    mCheckerConfig.broadcast = false;
  }
  mCheckerConfig.id = configFile.getValue<int>(checkerName + ".id");

  mCheckerConfig.numberCheckers = configFile.getValue<int>("checkers.numberCheckers");
  mCheckerConfig.tasksAddresses = configFile.getValue<string>("checkers.tasksAddresses");
  mCheckerConfig.numberTasks = configFile.getValue<int>("checkers.numberTasks");
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
  // statistics
  int numberHistosLastTime = 0;
  auto startFirstObject = system_clock::now();
  auto endLastObject = system_clock::now();
  bool first = true;
  Timer timer, timerProcessing;
  timer.reset(10000000); // 10 s.

  unique_ptr<FairMQPoller> poller(fTransportFactory->CreatePoller(fChannels["data-in"]));

  while (CheckCurrentState(RUNNING)) {
    poller->Poll(1);

    for (int i = 0; i < fChannels["data-in"].size(); i++) {
      if (poller->CheckInput(i)) {
        unique_ptr<FairMQMessage> msg(fTransportFactory->CreateMessage());
        if (fChannels.at("data-in").at(i).Receive(msg) > 0) {

          if (first == true) {
            startFirstObject = system_clock::now();
            first = false;
          }

          mLogger << "Receiving a mo of size " << msg->GetSize() << AliceO2::InfoLogger::InfoLogger::endm;
          timerProcessing.reset();
          TestTMessage tm(msg->GetData(), msg->GetSize());
          MonitorObject *mo = dynamic_cast<MonitorObject *>(tm.ReadObject(tm.GetClass()));
          mo->setIsOwner(true);
          if (mo) {
//            shared_ptr<MonitorObject> mo_shared(mo);
            string name = mo->getName();
            mTotalNumberHistosReceived++;
            check(mo);
            send(mo);
            store(mo);

            endLastObject = system_clock::now();
            mAccProcessTime(timerProcessing.getTime());
            mLogger << "Finished processing \"" << name << "\"" << AliceO2::InfoLogger::InfoLogger::endm;
          } else {
            mLogger << "the mo is null" << AliceO2::InfoLogger::InfoLogger::endm;
          }
        }
      }
    }

    // every 10 seconds publish stats
    if (timer.isTimeout()) {
      double current = timer.getTime();
      int objectsPublished = mTotalNumberHistosReceived - numberHistosLastTime;
      numberHistosLastTime = mTotalNumberHistosReceived;
      QcInfoLogger::GetInstance() << "Rate in the last 10 seconds : " << (objectsPublished / current)
                                  << " events/second" << AliceO2::InfoLogger::InfoLogger::endm;
      mCollector->send((objectsPublished / current), "QC_checker_Rate_objects_checked_per_second");
      timer.increment();

      //std::vector<std::string> pidStatus = mMonitor->getPIDStatus(::getpid());
      //pcpus(std::stod(pidStatus[3]));
      //pmems(std::stod(pidStatus[4]));

      std::chrono::duration<double> diff = endLastObject - startFirstObject;
      mCollector->send(diff.count(), "QC_checker_Time_between_first_and_last_objects_received");
      mCollector->send(mTotalNumberHistosReceived, "QC_checker_Total_number_histos_treated");
      double rate = mTotalNumberHistosReceived / diff.count();
      mCollector->send(rate, "QC_checker_Rate_objects_treated_per_second_whole_run");
//      mCollector->send(std::stod(pidStatus[3]), "QC_checker_Mean_pcpu_whole_run");
      mCollector->send(ba::mean(pmems), "QC_checker_Mean_pmem_whole_run");
      mCollector->send(ba::mean(mAccProcessTime), "QC_checker_Mean_processing_time_per_event");
    }
  }

  // Monitoring
//  std::chrono::duration<double> diff = endLastObject - startFirstObject;
//  mCollector->send(diff.count(), "QC_checker_Time_between_first_and_last_objects_received");
//  mCollector->send(mTotalNumberHistosReceived, "QC_checker_Total_number_histos_treated");
//  double rate = mTotalNumberHistosReceived / diff.count();
//  mCollector->send(rate, "QC_checker_Rate_objects_treated_per_second_whole_run");
//  mCollector->send(ba::mean(pcpus), "QC_checker_Mean_pcpu_whole_run");
//  mCollector->send(ba::mean(pmems), "QC_checker_Mean_pmem_whole_run");
//  mCollector->send(ba::mean(mAccProcessTime), "QC_checker_Mean_processing_time_per_event");
}

void Checker::check(MonitorObject* mo)
{
  mLogger << "Checking \"" << mo->getName() << "\"" << AliceO2::InfoLogger::InfoLogger::endm;

  // Get the Checks
  std::vector<CheckDefinition> checks = mo->getChecks();

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

void Checker::store(MonitorObject* mo)
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

void Checker::send(MonitorObject*  mo)
{
  if (!mCheckerConfig.broadcast) {
    return;
  }
  mLogger << "Sending \"" << mo->getName() << "\"" << AliceO2::InfoLogger::InfoLogger::endm;

  TMessage *message = new TMessage(kMESS_OBJECT);
  message->WriteObjectAny(mo, mo->IsA());
  unique_ptr<FairMQMessage> msg(NewMessage(message->Buffer(), message->BufferSize(), CustomCleanupTMessage, message));
  fChannels.at("data-out").at(0).Send(msg);
}

/// \brief Load a library.
/// \param libraryName The name of the library to load.
void Checker::loadLibrary(const string libraryName)
{
  if (boost::algorithm::trim_copy(libraryName) == "") {
    mLogger << "no library name specified" << AliceO2::InfoLogger::InfoLogger::endm;
    return;
  }

  string library = "lib" + libraryName + ".so";
  // if vector does not contain -> first time we see it
  if(std::find(mLibrariesLoaded.begin(), mLibrariesLoaded.end(), library) == mLibrariesLoaded.end()) {
    mLogger << "Loading library " << library << AliceO2::InfoLogger::InfoLogger::endm;
    if (gSystem->Load(library.c_str())) {
      BOOST_THROW_EXCEPTION(FatalException() << errinfo_details("Failed to load Detector Publisher Library"));
    }
    mLibrariesLoaded.push_back(library);
  }
}

CheckInterface *Checker::instantiateCheck(string checkName, string className)
{
  CheckInterface *result = 0;
  // Get the class and instantiate
  TClass *cl;
  string tempString("Failed to instantiate Quality Control Module");

  if(mClassesLoaded.count(className) == 0) {
    mLogger << "Loading class " << className << AliceO2::InfoLogger::InfoLogger::endm;
    cl = TClass::GetClass(className.c_str());
    if (!cl) {
      tempString += " because no dictionary for class named \"";
      tempString += className;
      tempString += "\" could be retrieved";
      cerr << tempString << endl;
      BOOST_THROW_EXCEPTION(FatalException() << errinfo_details(tempString));
    }
    mClassesLoaded[className] = cl;
  } else {
    cl = mClassesLoaded[className];
  }

  if(mChecksLoaded.count(checkName) == 0) {
    mLogger << "Instantiating class " << className << " (" << cl << ")" << AliceO2::InfoLogger::InfoLogger::endm;
    result = static_cast<CheckInterface *>(cl->New());
    if (!result) {
      tempString += " because the class named \"";
      tempString += className;
      tempString += "\" does not follow the TaskInterface interface";
      BOOST_THROW_EXCEPTION(FatalException() << errinfo_details(tempString));
    }
    result->configure(checkName);
    mChecksLoaded[checkName] = result;
  } else {
    result = mChecksLoaded[checkName];
  }

  return result;
}

} /* namespace Checker */
} /* namespace QualityControl */
} /* namespace AliceO2 */

