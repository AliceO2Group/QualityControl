///
/// \file   Checker.cxx
/// \author Barthelemy von Haller
/// \author Piotr Konopka
///

#include "QualityControl/Checker.h"

// ROOT
#include <TClass.h>
#include <TMessage.h>
#include <TSystem.h>
// O2
#include <Common/Exceptions.h>
#include <Configuration/ConfigurationFactory.h>
#include <Framework/DataRefUtils.h>
#include <TMap.h>
// QC
#include "QualityControl/DatabaseFactory.h"
#include "QualityControl/TaskRunner.h"

using namespace std::chrono;
using namespace AliceO2::Common;
using namespace AliceO2::InfoLogger;
using namespace o2::configuration;
using namespace o2::monitoring;

namespace o2
{
namespace quality_control
{
using namespace core;
using namespace repository;
namespace checker
{

// TODO do we need a CheckFactory ? here it is embedded in the Checker
// TODO maybe we could use the CheckerFactory

Checker::Checker(std::string checkerName, std::string taskName, std::string configurationSource)
  : mCheckerName(checkerName),
    mConfigurationSource(configurationSource),
    mInputSpec{ "mo", TaskRunner::createTaskDataOrigin(), TaskRunner::createTaskDataDescription(taskName), 0 },
    mOutputSpec{ "QC", Checker::createCheckerDataDescription(taskName), 0 },
    mLogger(QcInfoLogger::GetInstance()),
    startFirstObject{ system_clock::time_point::min() },
    endLastObject{ system_clock::time_point::min() },
    mTotalNumberHistosReceived(0)
{
}

Checker::~Checker()
{
  // Monitoring
  if (mCollector) {
    std::chrono::duration<double> diff = endLastObject - startFirstObject;
    mCollector->send({ diff.count(), "QC_checker_Time_between_first_and_last_objects_received" });
    mCollector->send({ mTotalNumberHistosReceived, "QC_checker_Total_number_histos_treated" });
    double rate = mTotalNumberHistosReceived / diff.count();
    mCollector->send({ rate, "QC_checker_Rate_objects_treated_per_second_whole_run" });
  }
}

void Checker::init(framework::InitContext&)
{
  // configuration
  try {
    std::unique_ptr<ConfigurationInterface> config = ConfigurationFactory::getConfiguration(mConfigurationSource);
    // configuration of the database
    mDatabase = DatabaseFactory::create(config->get<std::string>("qc.config.database.implementation"));
    mDatabase->connect(config->getRecursiveMap("qc.config.database"));
    LOG(INFO) << "Database that is going to be used : ";
    LOG(INFO) << ">> Implementation : " << config->get<std::string>("qc.config.database.implementation");
    LOG(INFO) << ">> Host : " << config->get<std::string>("qc.config.database.host");
  } catch (
    std::string const& e) { // we have to catch here to print the exception because the device will make it disappear
    LOG(ERROR) << "exception : " << e;
    throw;
  } catch (...) {
    std::string diagnostic = boost::current_exception_diagnostic_information();
    LOG(ERROR) << "Unexpected exception, diagnostic information follows:\n"
               << diagnostic;
    throw;
  }

  // monitoring
  try {
    mCollector = MonitoringFactory::Get("infologger://");
  } catch (...) {
    std::string diagnostic = boost::current_exception_diagnostic_information();
    LOG(ERROR) << "Unexpected exception, diagnostic information follows:\n" << diagnostic;
    throw;
  }
  startFirstObject = system_clock::time_point::min();
  timer.reset(1000000); // 10 s.
}

void Checker::run(framework::ProcessingContext& ctx)
{
  mLogger << "Receiving " << ctx.inputs().size() << " MonitorObjects" << AliceO2::InfoLogger::InfoLogger::endm;

  // Save time of first object
  if (startFirstObject == std::chrono::system_clock::time_point::min()) {
    startFirstObject = system_clock::now();
  }

  std::shared_ptr<TObjArray> moArray{ std::move(framework::DataRefUtils::as<TObjArray>(*ctx.inputs().begin())) };
  moArray->SetOwner(false);
  auto checkedMoArray = std::make_unique<TObjArray>();
  checkedMoArray->SetOwner();

  for (const auto& to : *moArray) {
    std::shared_ptr<MonitorObject> mo{dynamic_cast<MonitorObject*>(to)};
    moArray->RemoveFirst();
    if (mo) {
      check(mo);
      store(mo);
      mTotalNumberHistosReceived++;
      checkedMoArray->Add(new MonitorObject(*mo));
    } else {
      mLogger << "the mo is null" << AliceO2::InfoLogger::InfoLogger::endm;
    }
  }

  send(checkedMoArray, ctx.outputs());

  // monitoring
  endLastObject = system_clock::now();

  // if 10 seconds elapsed publish stats
  if (timer.isTimeout()) {
    timer.reset(1000000); // 10 s.
    mCollector->send({ mTotalNumberHistosReceived, "objects" }, o2::monitoring::DerivedMetricMode::RATE);
  }
}

o2::header::DataDescription Checker::createCheckerDataDescription(const std::string taskName)
{
  o2::header::DataDescription description;
  description.runtimeInit(std::string(taskName.substr(0, o2::header::DataDescription::size - 4) + "-chk").c_str());
  return description;
}

void Checker::check(std::shared_ptr<MonitorObject> mo)
{
  std::map<std::string /*checkName*/, CheckDefinition> checks = mo->getChecks();

  mLogger << "Running " << checks.size() << " checks for \"" << mo->getName() << "\""
          << AliceO2::InfoLogger::InfoLogger::endm;
  // Get the Checks

  // Loop over the Checks and execute them followed by the beautification
  for (const auto& [checkName, check] : checks) {
    mLogger << "        check name : " << checkName << AliceO2::InfoLogger::InfoLogger::endm;
    mLogger << "        check className : " << check.className << AliceO2::InfoLogger::InfoLogger::endm;
    mLogger << "        check libraryName : " << check.libraryName << AliceO2::InfoLogger::InfoLogger::endm;

    // load module, instantiate, use check
    // TODO : preload modules and pre-instantiate, or keep a cache
    loadLibrary(check.libraryName);
    CheckInterface* checkInstance = getCheck(checkName, check.className);
    Quality q = checkInstance->check(mo.get());

    mLogger << "  result of the check " << checkName << ": " << q.getName()
            << AliceO2::InfoLogger::InfoLogger::endm;

    checkInstance->beautify(mo.get(), q);
  }
}

void Checker::store(std::shared_ptr<MonitorObject> mo)
{
  mLogger << "Storing \"" << mo->getName() << "\"" << AliceO2::InfoLogger::InfoLogger::endm;
  try {
    mDatabase->store(mo);
  } catch (boost::exception& e) {
    mLogger << "Unable to " << diagnostic_information(e) << AliceO2::InfoLogger::InfoLogger::endm;
  }
}

void Checker::send(std::unique_ptr<TObjArray>& moArray, framework::DataAllocator& allocator)
{
  mLogger << "Sending Monitor Object array with " << moArray->GetEntries() << " objects inside." << AliceO2::InfoLogger::InfoLogger::endm;

  allocator.adopt(
    framework::Output{ mOutputSpec.origin, mOutputSpec.description, mOutputSpec.subSpec, mOutputSpec.lifetime }, moArray.release());
}

void Checker::loadLibrary(const std::string libraryName)
{
  if (boost::algorithm::trim_copy(libraryName).empty()) {
    mLogger << "no library name specified" << AliceO2::InfoLogger::InfoLogger::endm;
    return;
  }

  std::string library = "lib" + libraryName;
  // if vector does not contain -> first time we see it
  if (std::find(mLibrariesLoaded.begin(), mLibrariesLoaded.end(), library) == mLibrariesLoaded.end()) {
    mLogger << "Loading library " << library << AliceO2::InfoLogger::InfoLogger::endm;
    int libLoaded = gSystem->Load(library.c_str(), "", true);
    if (libLoaded == 1) {
      mLogger << "Already loaded before" << AliceO2::InfoLogger::InfoLogger::endm;
    } else if (libLoaded < 0 || libLoaded > 1) {
      BOOST_THROW_EXCEPTION(FatalException() << errinfo_details("Failed to load Detector Publisher Library"));
    }
    mLibrariesLoaded.push_back(library);
  }
}

CheckInterface* Checker::getCheck(std::string checkName, std::string className)
{
  CheckInterface* result = nullptr;
  // Get the class and instantiate
  TClass* cl;
  std::string tempString("Failed to instantiate Quality Control Module");

  if (mClassesLoaded.count(className) == 0) {
    mLogger << "Loading class " << className << AliceO2::InfoLogger::InfoLogger::endm;
    cl = TClass::GetClass(className.c_str());
    if (!cl) {
      tempString += R"( because no dictionary for class named ")";
      tempString += className;
      tempString += R"(" could be retrieved)";
      LOG(ERROR) << tempString;
      BOOST_THROW_EXCEPTION(FatalException() << errinfo_details(tempString));
    }
    mClassesLoaded[className] = cl;
  } else {
    cl = mClassesLoaded[className];
  }

  if (mChecksLoaded.count(checkName) == 0) {
    mLogger << "Instantiating class " << className << " (" << cl << ")" << AliceO2::InfoLogger::InfoLogger::endm;
    result = static_cast<CheckInterface*>(cl->New());
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

} /* namespace checker */
} /* namespace quality_control */
} /* namespace o2 */
