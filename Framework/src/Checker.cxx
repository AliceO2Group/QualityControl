///
/// \file   Checker.cxx
/// \author Barthelemy von Haller
///

#include "QualityControl/Checker.h"

// ROOT
#include <TMessage.h>
#include <TSystem.h>
#include <TClass.h>
// O2
#include <Common/Exceptions.h>
#include <Configuration/ConfigurationFactory.h>
// QC
#include "QualityControl/DatabaseFactory.h"

using namespace AliceO2::Common;
using namespace o2::configuration;

/**
 * \brief Handy class to deserialize messages and make sure buffer is not deleted along with the message.
 *
 * Avoids some boilerplate.
 */
class HistoMessage : public TMessage
{
  public:
    HistoMessage(void *buf, Int_t len)
      : TMessage(buf, len)
    {
      ResetBit(kIsOwner);
    }
};

using namespace std;
using namespace AliceO2::InfoLogger;
using namespace std::chrono;
using namespace o2::configuration;
using namespace o2::monitoring;

namespace o2 {
namespace quality_control {
using namespace core;
using namespace repository;
namespace checker {

// TODO do we need a CheckFactory ? here it is embedded in the Checker

Checker::Checker(std::string checkerName, std::string configurationSource)
  : mLogger(QcInfoLogger::GetInstance()), mTotalNumberHistosReceived(0)
{
  SetTransport("zeromq");

  // configuration
  try {
    unique_ptr<ConfigurationInterface> config = ConfigurationFactory::getConfiguration(configurationSource);
    populateConfig(config, checkerName);
  } catch (std::string const &e) { // we have to catch here to print the exception because the device will make it disappear
    std::cerr << "exception : " << e << endl;
    throw;
  } catch (...) {
    string diagnostic = boost::current_exception_diagnostic_information();
    std::cerr << "Unexpected exception, diagnostic information follows:\n" << diagnostic << endl;
    throw;
  }

  // monitoring
  try {
    mCollector = MonitoringFactory::Get("infologger://");
  } catch (...) {
    string diagnostic = boost::current_exception_diagnostic_information();
    std::cerr << "Unexpected exception, diagnostic information follows:\n" << diagnostic << endl;
    throw;
  }
  startFirstObject = std::chrono::system_clock::time_point::min();
  timer.reset(1000000); // 10 s.

  // Setup broadcast channel
  if (mCheckerConfig.broadcast) {
    createChannel("pub", "bind", mCheckerConfig.broadcastAddress, "data-out");
  }
}

void Checker::populateConfig(unique_ptr<ConfigurationInterface> &config, std::string checkerName)
{
  try {
    // General configuration
    mCheckerConfig.checkerName = checkerName;
    std::string prefix = std::string("qc/checkers_config/");
    mCheckerConfig.broadcast = (bool) config->get<int>(prefix + checkerName + "/broadcast").value_or(0);
    if (mCheckerConfig.broadcast) {
      mCheckerConfig.broadcastAddress = config->get<string>(prefix + checkerName + "/broadcastAddress").value();
    }
    mCheckerConfig.id = config->get<int>(prefix + checkerName + "/id").value();
    mCheckerConfig.numberCheckers = config->get<int>(prefix + "numberCheckers").value();
    mCheckerConfig.tasksAddresses = config->get<string>(prefix + "tasksAddresses").value();
    mCheckerConfig.numberTasks = config->get<int>(prefix + "numberTasks").value();

    // configuration of the database
    mDatabase = DatabaseFactory::create(config->get<string>("qc/config/database/implementation").value());
    mDatabase->connect(config);
  } catch (...) { // catch already here the configuration exception and print it
    // because if we are in a constructor, the exception could be lost
    string diagnostic = boost::current_exception_diagnostic_information();
    std::cerr << "Unexpected exception, diagnostic information follows:\n" << diagnostic << endl;
    throw;
  }
}

void Checker::createChannel(std::string type, std::string method, std::string address, std::string channelName,
                            bool createCallback)
{
  FairMQChannel channel;
  channel.UpdateType(type);
  channel.UpdateMethod(method);
  channel.UpdateAddress(address);
  channel.UpdateRateLogging(0);
  fChannels[channelName].push_back(channel);
  if (createCallback) {
    OnData(channelName, &Checker::HandleData);
  }
}

Checker::~Checker()
{
  mDatabase->disconnect();

  // Monitoring
  std::chrono::duration<double> diff = endLastObject - startFirstObject;
  mCollector->send({diff.count(), "QC_checker_Time_between_first_and_last_objects_received"});
  mCollector->send({mTotalNumberHistosReceived, "QC_checker_Total_number_histos_treated"});
  double rate = mTotalNumberHistosReceived / diff.count();
  mCollector->send({rate, "QC_checker_Rate_objects_treated_per_second_whole_run"});
}

int size_t2int(size_t val)
{
  if (val > INT_MAX) {
    throw out_of_range("Conversion from size_t to int failed.");
  }
  return (int) val;
}

bool Checker::HandleData(FairMQMessagePtr &msg, int index)
{
  mLogger << "Receiving a mo of size " << msg->GetSize() << AliceO2::InfoLogger::InfoLogger::endm;

  // Save time of first object
  if (startFirstObject == std::chrono::system_clock::time_point::min()) {
    startFirstObject = system_clock::now();
  }

  // Deserialize the object and process it
  HistoMessage tm(msg->GetData(), size_t2int(msg->GetSize()));
  shared_ptr<MonitorObject> mo(dynamic_cast<MonitorObject *>(tm.ReadObject(tm.GetClass())));
  if (mo) {
    mo->setIsOwner(true);
    check(mo);
    send(mo);
    store(mo);
    mTotalNumberHistosReceived++;
  } else {
    mLogger << "the mo is null" << AliceO2::InfoLogger::InfoLogger::endm;
  }

  // monitoring
  endLastObject = system_clock::now();
  // if 10 seconds elapsed publish stats
  if (timer.isTimeout()) {
    timer.reset(1000000); // 10 s.
    mCollector->send({mTotalNumberHistosReceived, "objects"}, o2::monitoring::DerivedMetricMode::RATE);
  }

  return true; // keep running
}

void Checker::check(shared_ptr<MonitorObject> mo)
{
  mLogger << "Checking \"" << mo->getName() << "\"" << AliceO2::InfoLogger::InfoLogger::endm;

  // Get the Checks
  std::map<std::string /*checkName*/, CheckDefinition> checks = mo->getChecks();

  // Loop over the Checks and execute them followed by the beautification
  for (const auto &check : checks) {
//    std::cout << "        check name : " << check.name << std::endl;
//    std::cout << "        check className : " << check.className << std::endl;
//    std::cout << "        check libraryName : " << check.libraryName << std::endl;

    // load module, instantiate, use check
    // TODO : preload modules and pre-instantiate, or keep a cache
    loadLibrary(check.second.libraryName);
    CheckInterface *checkInstance = instantiateCheck(check.second.name, check.second.className);
    Quality q = checkInstance->check(mo.get());

    mLogger << "  result of the check " << check.second.name << ": " << q.getName() << AliceO2::InfoLogger::InfoLogger::endm;

    checkInstance->beautify(mo.get(), q);
  }
}

void Checker::store(shared_ptr<MonitorObject> mo)
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

void Checker::send(shared_ptr<MonitorObject> mo)
{
  if (!mCheckerConfig.broadcast) {
    return;
  }
  mLogger << "Sending \"" << mo->getName() << "\"" << AliceO2::InfoLogger::InfoLogger::endm;

  auto *message = new TMessage(kMESS_OBJECT);
  message->WriteObjectAny(mo.get(), mo->IsA());
  unique_ptr<FairMQMessage> msg(NewMessage(message->Buffer(), message->BufferSize(), CustomCleanupTMessage, message));
  fChannels.at("data-out").at(0).Send(msg);
}

/// \brief Load a library.
/// \param libraryName The name of the library to load.
void Checker::loadLibrary(const string libraryName)
{
  if (boost::algorithm::trim_copy(libraryName).empty()) {
    mLogger << "no library name specified" << AliceO2::InfoLogger::InfoLogger::endm;
    return;
  }

  string library = "lib" + libraryName + ".so";
  // if vector does not contain -> first time we see it
  if (std::find(mLibrariesLoaded.begin(), mLibrariesLoaded.end(), library) == mLibrariesLoaded.end()) {
    mLogger << "Loading library " << library << AliceO2::InfoLogger::InfoLogger::endm;
    if (gSystem->Load(library.c_str())) {
      BOOST_THROW_EXCEPTION(FatalException() << errinfo_details("Failed to load Detector Publisher Library"));
    }
    mLibrariesLoaded.push_back(library);
  }
}

CheckInterface *Checker::instantiateCheck(string checkName, string className)
{
  CheckInterface *result = nullptr;
  // Get the class and instantiate
  TClass *cl;
  string tempString("Failed to instantiate Quality Control Module");

  if (mClassesLoaded.count(className) == 0) {
    mLogger << "Loading class " << className << AliceO2::InfoLogger::InfoLogger::endm;
    cl = TClass::GetClass(className.c_str());
    if (!cl) {
      tempString += R"( because no dictionary for class named ")";
      tempString += className;
      tempString += R"(" could be retrieved)";
      cerr << tempString << endl;
      BOOST_THROW_EXCEPTION(FatalException() << errinfo_details(tempString));
    }
    mClassesLoaded[className] = cl;
  } else {
    cl = mClassesLoaded[className];
  }

  if (mChecksLoaded.count(checkName) == 0) {
    mLogger << "Instantiating class " << className << " (" << cl << ")" << AliceO2::InfoLogger::InfoLogger::endm;
    result = static_cast<CheckInterface *>(cl->New());
    if (!result) {
      tempString += R"( because the class named ")";
      tempString += className;
      tempString += R"( because the class named ")";
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
} /* namespace quality_control */
} /* namespace o2 */

