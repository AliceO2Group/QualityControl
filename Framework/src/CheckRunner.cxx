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
/// \file   CheckRunner.cxx
/// \author Barthelemy von Haller
/// \author Piotr Konopka
///

#include "QualityControl/CheckRunner.h"

// Boost
#include <boost/filesystem/path.hpp>

#include <utility>
#include <memory>
#include <functional>
#include <algorithm>
#include <set>
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
using namespace o2::framework;
using namespace o2::configuration;
using namespace o2::monitoring;
using namespace o2::quality_control::core;
using namespace o2::quality_control::repository;
namespace bfs = boost::filesystem;

namespace o2::quality_control::checker
{

/// Static functions
o2::header::DataDescription CheckRunner::createCheckRunnerDataDescription(const std::string taskName)
{
  if (taskName.empty()) {
    BOOST_THROW_EXCEPTION(FatalException() << errinfo_details("Empty taskName for checker's data description"));
  }
  o2::header::DataDescription description;
  description.runtimeInit(std::string(taskName.substr(0, o2::header::DataDescription::size - 4) + "-chk").c_str());
  return description;
}

o2::framework::Inputs CheckRunner::createInputSpec(const std::string checkName, const std::string configSource)
{
  std::unique_ptr<ConfigurationInterface> config = ConfigurationFactory::getConfiguration(configSource);
  o2::framework::Inputs inputs;
  for (auto& [key, sourceConf] : config->getRecursive("qc.checks." + checkName + ".dataSource")) {
    (void)key;
    if (sourceConf.get<std::string>("type") == "Task") {
      const std::string& taskName = sourceConf.get<std::string>("name");
      QcInfoLogger::GetInstance() << ">>>> Check name : " << checkName << " input task name: " << taskName << " " << TaskRunner::createTaskDataDescription(taskName).as<std::string>() << AliceO2::InfoLogger::InfoLogger::endm;
      o2::framework::InputSpec input{ taskName, TaskRunner::createTaskDataOrigin(), TaskRunner::createTaskDataDescription(taskName) };
      inputs.push_back(std::move(input));
    }
  }

  return inputs;
}

std::string CheckRunner::createCheckRunnerName(std::vector<Check> checks)
{
  static const std::string alphanumeric =
    "0123456789"
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
    "abcdefghijklmnopqrstuvwxyz";
  const int NAME_LEN = 4;
  std::string name(CheckRunner::createCheckRunnerIdString() + "-");

  if (checks.size() == 1) {
    // If single check, use the check name
    name += checks[0].getName();
  } else {
    std::string hash_string = "";
    std::vector<std::string> names;
    // Fill vector with check names
    for (auto& c: checks){
      names.push_back(c.getName());
    }
    // Be sure that after configuration shuffle, the name will be the same
    std::sort(names.begin(), names.end());

    // Create a single string and hash it
    for (auto& n: names){
      hash_string += n;
    }
    std::size_t num = std::hash<std::string>{}(hash_string);

    // Change numerical to alphanumeric hash representation
    for (int i = 0; i < NAME_LEN; ++i) {
      name += alphanumeric[num%alphanumeric.size()];
      num = num / alphanumeric.size();
    }
  }
  return name;
}

o2::framework::Outputs CheckRunner::collectOutputs(const std::vector<Check>& checks)
{
  o2::framework::Outputs outputs;
  for (auto& check : checks) {
    outputs.push_back(check.getOutputSpec());
  }
  return outputs;
}

/// Members

// TODO do we need a CheckFactory ? here it is embedded in the CheckRunner
// TODO maybe we could use the CheckRunnerFactory

CheckRunner::CheckRunner(std::vector<Check> checks, std::string configurationSource)
  : mDeviceName(createCheckRunnerName(checks)),
    mChecks{ checks },
    mConfigurationSource(configurationSource),
    mLogger(QcInfoLogger::GetInstance()),
    /* All checks has the same Input */
    mInputs(checks.front().getInputs()),
    mOutputs(CheckRunner::collectOutputs(checks)),
    startFirstObject{ system_clock::time_point::min() },
    endLastObject{ system_clock::time_point::min() }
{
  mTotalNumberHistosReceived = 0;
}

CheckRunner::CheckRunner(Check check, std::string configurationSource)
  : CheckRunner(std::vector{ check }, configurationSource)
{
}

CheckRunner::~CheckRunner()
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

void CheckRunner::init(framework::InitContext&)
{
  initDatabase();
  initMonitoring();
  for (auto& check : mChecks) {
    check.init();
  }
}

void CheckRunner::run(framework::ProcessingContext& ctx)
{

  // Save time of first object
  if (startFirstObject == std::chrono::system_clock::time_point::min()) {
    startFirstObject = system_clock::now();
  }
  mMonitorObjectStoreVector.clear();

  for (const auto& input : mInputs) {
    auto dataRef = ctx.inputs().get(input.binding.c_str());
    if (dataRef.header != nullptr && dataRef.payload != nullptr) {
      auto moArray = ctx.inputs().get<TObjArray*>(input.binding.c_str());
      mLogger << "Device " << mDeviceName
              << " received " << moArray->GetEntries()
              << " MonitorObjects from " << input.binding
              << AliceO2::InfoLogger::InfoLogger::endm;

      // Check if this CheckRunner stores this input
      bool store = mInputStoreSet.count(DataSpecUtils::label(input)) > 0;

      for (const auto& to : *moArray) {
        std::shared_ptr<MonitorObject> mo{ dynamic_cast<MonitorObject*>(to) };

        if (mo) {
          update(mo);
          mTotalNumberHistosReceived++;

          // Add monitor object to store later, after possible beautification
          if (store) {
            mMonitorObjectStoreVector.push_back(mo);
          }

        } else {
          mLogger << "The mo is null" << AliceO2::InfoLogger::InfoLogger::endm;
        }
      }
    }
  }

  // Check if compliant with policy
  auto triggeredChecks = check(mMonitorObjects);
  store(triggeredChecks);
  send(triggeredChecks, ctx.outputs());

  // Update global revision number
  updateRevision();

  // monitoring
  endLastObject = system_clock::now();
  if (timer.isTimeout()) {
    timer.reset(1000000); // 10 s.
    mCollector->send({ mTotalNumberHistosReceived, "objects" }, o2::monitoring::DerivedMetricMode::RATE);
  }
}

void CheckRunner::update(std::shared_ptr<MonitorObject> mo)
{
  mMonitorObjects[mo->getFullName()] = mo;
  mMonitorObjectRevision[mo->getFullName()] = mGlobalRevision;
}

std::vector<Check*> CheckRunner::check(std::map<std::string, std::shared_ptr<MonitorObject>> moMap)
{
  mLogger << "Running " << mChecks.size() << " checks for " << moMap.size() << " monitor objects"
          << AliceO2::InfoLogger::InfoLogger::endm;

  std::vector<Check*> triggeredChecks;
  for (auto& check : mChecks) {
    if (check.isReady(mMonitorObjectRevision)) {
      auto qualityObj = check.check(moMap);
      // Check if shared_ptr != nullptr
      if (qualityObj) {
        triggeredChecks.push_back(&check);
      }

      // Was checked, update latest revision
      check.updateRevision(mGlobalRevision);
    } else {
      mLogger << "Monitor Objects for the check '" << check.getName() << "' are not ready, ignoring" << AliceO2::InfoLogger::InfoLogger::endm;
    }
  }
  return triggeredChecks;
}

void CheckRunner::store(std::vector<Check*>& checks)
{
  mLogger << "Storing " << checks.size() << " quality objects" << AliceO2::InfoLogger::InfoLogger::endm;
  try {
    for (auto check : checks) {
      mDatabase->storeQO(check->getQualityObject());
    }
  } catch (boost::exception& e) {
    mLogger << "Unable to " << diagnostic_information(e) << AliceO2::InfoLogger::InfoLogger::endm;
  }

  mLogger << "Storing " << mMonitorObjectStoreVector.size() << " monitor objects" << AliceO2::InfoLogger::InfoLogger::endm;
  try {
    for (auto mo : mMonitorObjectStoreVector) {
      mDatabase->storeMO(mo);
    }
  } catch (boost::exception& e) {
    mLogger << "Unable to " << diagnostic_information(e) << AliceO2::InfoLogger::InfoLogger::endm;
  }
}

void CheckRunner::send(std::vector<Check*>& checks, framework::DataAllocator& allocator)
{
  mLogger << "Send  " << checks.size() << " quality objects" << AliceO2::InfoLogger::InfoLogger::endm;
  for (auto check : checks) {
    auto outputSpec = check->getOutputSpec();
    auto concreteOutput = framework::DataSpecUtils::asConcreteDataMatcher(outputSpec);
    allocator.snapshot(
      framework::Output{ concreteOutput.origin, concreteOutput.description, concreteOutput.subSpec, outputSpec.lifetime }, *check->getQualityObject());
  }
}

void CheckRunner::updateRevision()
{
  ++mGlobalRevision;
  if (mGlobalRevision == 0) {
    // mGlobalRevision cannot be 0
    // 0 means overflow, increment and update all check revisions to 0
    ++mGlobalRevision;
    for (auto& check : mChecks) {
      check.updateRevision(0);
    }
  }
}

void CheckRunner::initDatabase()
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
}

void CheckRunner::initMonitoring()
{
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

} // namespace o2::quality_control::checker
// namespace o2::quality_control::checker
