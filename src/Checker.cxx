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
// O2
#include "Common/Exceptions.h"
// QC
#include "QualityControl/CheckInterface.h"

using namespace AliceO2::Common;

class TestTMessage: public TMessage
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
namespace QualityControl {
using namespace Core;
namespace Checker {

// TODO do we need a CheckFactory ? here it is embedded in the Checker

Checker::Checker()
    : mLogger(QcInfoLogger::GetInstance())
{
}

Checker::~Checker()
{
}

void Checker::Run()
{
  while (CheckCurrentState(RUNNING)) {

    unique_ptr<FairMQMessage> msg(fTransportFactory->CreateMessage());

    if (fChannels.at("data-in").at(0).Receive(msg) > 0) {
      cout << "Receiving a mo of size " << msg->GetSize() << endl;
      TestTMessage tm(msg->GetData(), msg->GetSize());
      MonitorObject *mo = dynamic_cast<MonitorObject *>(tm.ReadObject(tm.GetClass()));
      if (mo) {
        check(mo);
      }
    }
  }
}

void Checker::check(MonitorObject *mo)
{
  // Get the Checks
//  std::map<std::string, std::string> checks = mo->getChecks();
  std::vector<CheckDefinition> checks = mo->getChecks();

  cout << "******** check mo : " << mo->getName() << endl;

  // Loop over the Checks and execute them followed by the beautification
  for (const auto& check : checks) {
    std::cout << "        check name : " << check.name << std::endl;
    std::cout << "        check className : " << check.className << std::endl;
    std::cout << "        check libraryName : " << check.libraryName << std::endl;

    // load module, instantiate, use check
    // TODO : preload modules and pre-instantiate, or keep a cache
    loadLibrary(check.libraryName);
    CheckInterface *checkInstance = instantiateCheck(check.name, check.className);
    Quality q = checkInstance->check(mo);

    std::cout << "        result of the check : " << q.getName() << std::endl;

    checkInstance->beautify(mo, q);
  }
}

/// \brief Load a library.
/// \param libraryName The name of the library to load.
void Checker::loadLibrary(const string libraryName) const
{
  if (boost::algorithm::trim_copy(libraryName) == "") {
    cout << "no library name specified" << endl;
    return;
  }

  string library = "lib" + libraryName + ".so";
  mLogger << "Loading library " << library << AliceO2::InfoLogger::InfoLogger::endm;
  if (gSystem->Load(library.c_str())) {
    BOOST_THROW_EXCEPTION(FatalException() << errinfo_details("Failed to load Detector Publisher Library"));
  }
}

CheckInterface* Checker::instantiateCheck(string checkName, string className) const
{
  CheckInterface *result = 0;
  // Get the class and instantiate
  mLogger << "Loading class " << className << AliceO2::InfoLogger::InfoLogger::endm;
  TClass* cl = TClass::GetClass(className.c_str());
  string tempString("Failed to instantiate Quality Control Module");
  if (!cl) {
    tempString += " because no dictionary for class named \"";
    tempString += className;
    tempString += "\" could be retrieved";
    BOOST_THROW_EXCEPTION(FatalException() << errinfo_details(tempString));
  }
  mLogger << "Instantiating class " << className << " (" << cl << ")" << AliceO2::InfoLogger::InfoLogger::endm;
  result = static_cast<CheckInterface*>(cl->New());
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

