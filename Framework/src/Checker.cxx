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
/// \file   Checker.cxx
/// \author Barthelemy von Haller
/// \author Piotr Konopka
///

#include "QualityControl/Checker.h"

// Boost
#include <boost/filesystem/path.hpp>

#include <utility>
#include <memory>
// ROOT
#include <TClass.h>
#include <TSystem.h>
// O2
#include <Common/Exceptions.h>
#include <Configuration/ConfigurationFactory.h>
#include <Framework/DataRefUtils.h>
#include <Framework/DataSpecUtils.h>
#include <Monitoring/MonitoringFactory.h>
#include <Monitoring/Monitoring.h>
// QC
#include "QualityControl/DatabaseFactory.h"
#include "QualityControl/TaskRunner.h"

using namespace std::chrono;
using namespace AliceO2::Common;
using namespace AliceO2::InfoLogger;
using namespace o2::configuration;
using namespace o2::monitoring;
using namespace o2::quality_control::core;
using namespace o2::quality_control::repository;
using namespace o2::quality_control::monitor;
namespace bfs = boost::filesystem;

namespace o2::quality_control::checker
{

/// Static functions
o2::header::DataDescription Checker::createCheckerDataDescription(const std::string taskName)
{
  if (taskName.empty()) {
    BOOST_THROW_EXCEPTION(FatalException() << errinfo_details("Empty taskName for checker's data description"));
  }
  o2::header::DataDescription description;
  description.runtimeInit(std::string(taskName.substr(0, o2::header::DataDescription::size - 4) + "-chk").c_str());
  return description;
}

o2::framework::Inputs Checker::createInputSpec(const std::string checkName, const std::string configSource)
{
  std::unique_ptr<ConfigurationInterface> config = ConfigurationFactory::getConfiguration(configSource);
  o2::framework::Inputs inputs;
  for(auto& [key, sourceConf]: config->getRecursive("qc.checks."+checkName+".dataSource")){
    (void)key;
    if(sourceConf.get<std::string>("type") == "Task"){
      const std::string& taskName = sourceConf.get<std::string>("name"); 
      QcInfoLogger::GetInstance() << ">>>> Check name : " << checkName << " input task name: " << taskName << " " << TaskRunner::createTaskDataDescription(taskName).as<std::string>() << AliceO2::InfoLogger::InfoLogger::endm;
      o2::framework::InputSpec input{taskName, TaskRunner::createTaskDataOrigin(),  TaskRunner::createTaskDataDescription(taskName)};
      inputs.push_back(std::move(input));
    }
  }

  return inputs;
}

std::string Checker::createCheckerIdString(){
  return std::string("QC-CHECKER");
}


/// Members 

// TODO do we need a CheckFactory ? here it is embedded in the Checker
// TODO maybe we could use the CheckerFactory


Checker::Checker(std::vector<std::string> checkerNames, std::string configurationSource)
  : mDeviceName(createCheckerIdString() + "-" + checkerNames.front()),
    mCheckerNames{checkerNames},
    mConfigurationSource(configurationSource),
    mLogger(QcInfoLogger::GetInstance()),
    mInputs(Checker::createInputSpec(checkerNames.front(), configurationSource)),
    mOutputSpec{ "QC", Checker::createCheckerDataDescription(checkerNames.front()), 0 },
    startFirstObject{ system_clock::time_point::min() },
    endLastObject{ system_clock::time_point::min() }
{
  mTotalNumberHistosReceived = 0;
  for (auto& checkerName: checkerNames){
    std::shared_ptr<QualityObject> qo(new QualityObject(checkerName));
    qo->setInputs(mInputs);
    mQualityObjects.insert(std::pair(checkerName, qo));
  }
}

Checker::Checker(std::string checkerName, std::string configurationSource)
  : Checker(std::vector{checkerName}, configurationSource)
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
  initDatabase();
  initMonitoring();
  initPolicy();
  populateConfig();
}

void Checker::populateConfig(){
  try {
    // Init Checker Interface
      std::unique_ptr<ConfigurationInterface> config = ConfigurationFactory::getConfiguration(mConfigurationSource);
      for (const auto& checkerName: mCheckerNames){
        const auto& moduleName = config->get<std::string>("qc.checks."+checkerName+".moduleName");
        loadLibrary(moduleName);
        const std::string& className = config->get<std::string>("qc.checks." + checkerName + ".className");
        mChecks.insert(std::pair<std::string, CheckInterface*>(checkerName, getCheck(checkerName, className)));
      }
  } catch (...) {
    std::string diagnostic = boost::current_exception_diagnostic_information();
    LOG(ERROR) << "Unexpected exception, diagnostic information follows:\n"
               << diagnostic;
    throw;
  }
}
void Checker::initPolicy(){
  try {
      const auto& checkerName = mCheckerNames.front();
      std::unique_ptr<ConfigurationInterface> config = ConfigurationFactory::getConfiguration(mConfigurationSource);
      std::vector<std::string> inputs;
      const auto& conf = config->getRecursive("qc.checks."+ checkerName);
      //const auto& conf = config->getRecursive("qc.checks."+mCheckerName);
      for(const auto& [_key, dataSource]: conf.get_child("dataSource")){
        (void)_key;
        if (dataSource.get<std::string>("type") == "Task"){
          inputs.push_back(dataSource.get_value<std::string>("name"));
        }
      }
      mPolicy = std::shared_ptr<MonitorObjectPolicy>(new MonitorObjectPolicy(config->get<std::string>("qc.checks."+checkerName+".policy"), inputs));
  } catch (...) {
    std::string diagnostic = boost::current_exception_diagnostic_information();
    LOG(ERROR) << "Unexpected exception, diagnostic information follows:\n"
               << diagnostic;
    throw;
  }

}

void Checker::run(framework::ProcessingContext& ctx)
{
  mLogger << mCheckerNames.front() <<" Receiving " << ctx.inputs().size() << " MonitorObjects" << AliceO2::InfoLogger::InfoLogger::endm;

  // Save time of first object
  if (startFirstObject == std::chrono::system_clock::time_point::min()) {
    startFirstObject = system_clock::now();
  }

  for (const auto& input: mInputs){
    auto dataRef = ctx.inputs().get(input.binding.c_str());
    if(dataRef.header != nullptr && dataRef.payload != nullptr){
      auto moArray = ctx.inputs().get<TObjArray*>(input.binding.c_str());
      for (const auto& to : *moArray) {
        std::shared_ptr<MonitorObject> mo{ dynamic_cast<MonitorObject*>(to) };

        if (mo) {
          update(mo);
          mTotalNumberHistosReceived++;
        } else {
          mLogger << "The mo is null" << AliceO2::InfoLogger::InfoLogger::endm;
        }
      }
    }
  }

  // Check if compliant with policy
  if (mPolicy->isReady()){
    auto qualityVector = check(mMonitorObjects);
    store(qualityVector);
    //send(checkedMoArray, ctx.outputs());
  }

  // monitoring
  endLastObject = system_clock::now();
  if (timer.isTimeout()) {
    timer.reset(1000000); // 10 s.
    mCollector->send({ mTotalNumberHistosReceived, "objects" }, o2::monitoring::DerivedMetricMode::RATE);
  }
}

void Checker::update(std::shared_ptr<MonitorObject> mo){
  mLogger << mCheckerNames.front() << " - moMap key: " << mo->getTaskName() << AliceO2::InfoLogger::InfoLogger::endm;
  mMonitorObjects[mo->getTaskName()] = mo;
  mPolicy->update(mo->getTaskName());
}

std::vector<std::shared_ptr<QualityObject>> Checker::check(std::map<std::string, std::shared_ptr<MonitorObject>> moMap)
{

  mLogger << "Running " << mChecks.size() << " checks for " << moMap.size() << " monitor objects"
          << AliceO2::InfoLogger::InfoLogger::endm;
  
  std::vector<std::shared_ptr<QualityObject>> qualityVector;

  // Loop over the Checks and execute them followed by the beautification
  for (const auto& [checkName, checkInstance] : mChecks) {
      mLogger << "        check name : " << checkName << AliceO2::InfoLogger::InfoLogger::endm;

      // load module, instantiate, use check
      // TODO : preload modules and pre-instantiate, or keep a cache
      Quality q = checkInstance->check(&moMap);

      mLogger << "  result of the check " << checkName << ": " << q.getName()
              << AliceO2::InfoLogger::InfoLogger::endm;

      if (mInputs.size() == 1 && moMap.size() == 1){
        auto& mo = moMap.begin()->second;
        checkInstance->beautify(mo, q);
        mLogger << "Beautify " << checkName << AliceO2::InfoLogger::InfoLogger::endm;
      }

      mQualityObjects[checkName]->updateQuality(q);
      qualityVector.push_back(mQualityObjects[checkName]);
  }
  return std::move(qualityVector);
}

void Checker::store(std::vector<std::shared_ptr<QualityObject>> qualityVector)
{
  mLogger << "Storing \"" << qualityVector.size() << "\"" << AliceO2::InfoLogger::InfoLogger::endm;
  try {
    for (auto& obj: qualityVector){
      mDatabase->store(obj);
    }
  } catch (boost::exception& e) {
    mLogger << "Unable to " << diagnostic_information(e) << AliceO2::InfoLogger::InfoLogger::endm;
  }
}

void Checker::send(std::unique_ptr<TObjArray>& moArray, framework::DataAllocator& allocator)
{
  mLogger << "Sending Monitor Object array with " << moArray->GetEntries() << " objects inside." << AliceO2::InfoLogger::InfoLogger::endm;
  auto concreteOutput = framework::DataSpecUtils::asConcreteDataMatcher(mOutputSpec);
  allocator.adopt(
    framework::Output{ concreteOutput.origin, concreteOutput.description, concreteOutput.subSpec, mOutputSpec.lifetime }, moArray.release());
}

void Checker::loadLibrary(const std::string libraryName)
{
  if (boost::algorithm::trim_copy(libraryName).empty()) {
    mLogger << "no library name specified" << AliceO2::InfoLogger::InfoLogger::endm;
    return;
  }

  std::string library = bfs::path(libraryName).is_absolute() ? libraryName : "lib" + libraryName;
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



void Checker::initDatabase(){
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
}

void Checker::initMonitoring(){
  // monitoring
  try {
    mCollector = MonitoringFactory::Get("infologger://");
  } catch (...) {
    std::string diagnostic = boost::current_exception_diagnostic_information();
    LOG(ERROR) << "Unexpected exception, diagnostic information follows:\n"
               << diagnostic;
    throw;
  }
  startFirstObject = system_clock::time_point::min();
  timer.reset(1000000); // 10 s.
}

}
// namespace o2::quality_control::checker
