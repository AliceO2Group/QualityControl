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

#include <utility>
#include <memory>
#include <algorithm>
// ROOT
#include <TClass.h>
#include <TSystem.h>
// O2
#include <Common/Exceptions.h>
#include <Configuration/ConfigurationFactory.h>
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

const auto current_diagnostic = boost::current_exception_diagnostic_information;

namespace o2::quality_control::checker
{

/// Static functions
// fixme: this is not actually used. collectOutputs() is used instead.
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
      ILOG(Info) << ">>>> Check name : " << checkName << " input task name: " << taskName << " " << TaskRunner::createTaskDataDescription(taskName).as<std::string>() << ENDM;
      o2::framework::InputSpec input{ taskName, TaskRunner::createTaskDataOrigin(), TaskRunner::createTaskDataDescription(taskName) };
      inputs.push_back(std::move(input));
    }
  }

  return inputs;
}

std::size_t CheckRunner::hash(std::string inputString)
{
  // BSD checksum
  const int mode = 16;
  std::size_t checksum = 0;

  const std::size_t mask = (1 << (mode + 1)) - 1;
  for (char c : inputString) {
    // Rotate the sum
    checksum = (checksum >> 1) + ((checksum & 1) << (mode - 1));
    checksum = (checksum + (std::size_t)c) & mask;
  }
  return checksum;
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
    for (auto& c : checks) {
      names.push_back(c.getName());
    }
    // Be sure that after configuration shuffle, the name will be the same
    std::sort(names.begin(), names.end());

    // Create a single string and hash it
    for (auto& n : names) {
      hash_string += n;
    }
    std::size_t num = hash(hash_string);

    // Change numerical to alphanumeric hash representation
    for (int i = 0; i < NAME_LEN; ++i) {
      name += alphanumeric[num % alphanumeric.size()];
      num = num / alphanumeric.size();
    }
  }
  return name;
}

std::string CheckRunner::createSinkCheckRunnerName(InputSpec input)
{
  std::string name(CheckRunner::createCheckRunnerIdString() + "-sink-");
  name += DataSpecUtils::label(input);
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
    mLogger(QcInfoLogger::GetInstance()),
    /* All checks have the same Input */
    mInputs(checks.front().getInputs()),
    mOutputs(CheckRunner::collectOutputs(checks)),
    mTotalNumberObjectsReceived(0),
    mTotalNumberCheckExecuted(0),
    mTotalNumberQOStored(0),
    mTotalNumberMOStored(0)
{
  try {
    mConfigFile = ConfigurationFactory::getConfiguration(configurationSource);
  } catch (...) {
    // catch the exceptions and print it (the ultimate caller might not know how to display it)
    ILOG(Fatal) << "Unexpected exception during initialization:\n"
                << boost::current_exception_diagnostic_information(true) << ENDM;
    throw;
  }
}

CheckRunner::CheckRunner(Check check, std::string configurationSource)
  : CheckRunner(std::vector{ check }, configurationSource)
{
}

CheckRunner::CheckRunner(InputSpec input, std::string configurationSource)
  : mDeviceName(createSinkCheckRunnerName(input)),
    mChecks{},
    mLogger(QcInfoLogger::GetInstance()),
    mInputs{ input },
    mOutputs{},
    mTotalNumberObjectsReceived(0),
    mTotalNumberCheckExecuted(0),
    mTotalNumberQOStored(0),
    mTotalNumberMOStored(0)
{
  try {
    mConfigFile = ConfigurationFactory::getConfiguration(configurationSource);
  } catch (...) {
    // catch the exceptions and print it (the ultimate caller might not know how to display it)
    ILOG(Fatal) << "Unexpected exception during initialization:\n"
                << boost::current_exception_diagnostic_information(true) << ENDM;
    throw;
  }
}

CheckRunner::~CheckRunner()
{
}

void CheckRunner::init(framework::InitContext&)
{
  try {
    initDatabase();
    initMonitoring();
    for (auto& check : mChecks) {
      check.init();
    }
  } catch (...) {
    // catch the exceptions and print it (the ultimate caller might not know how to display it)
    ILOG(Fatal) << "Unexpected exception during initialization:\n"
                << current_diagnostic(true) << ENDM;
    throw;
  }
}

void CheckRunner::run(framework::ProcessingContext& ctx)
{
  mMonitorObjectStoreVector.clear();

  for (const auto& input : mInputs) {
    auto dataRef = ctx.inputs().get(input.binding.c_str());
    if (dataRef.header != nullptr && dataRef.payload != nullptr) {
      auto moArray = ctx.inputs().get<TObjArray*>(input.binding.c_str());
      mLogger << "Device " << mDeviceName
              << " received " << moArray->GetEntries()
              << " MonitorObjects from " << input.binding
              << ENDM;

      // Check if this CheckRunner stores this input
      bool store = mInputStoreSet.count(DataSpecUtils::label(input)) > 0;

      for (const auto tObject : *moArray) {
        std::shared_ptr<MonitorObject> mo{ dynamic_cast<MonitorObject*>(tObject) };

        if (mo) {
          update(mo);
          mTotalNumberObjectsReceived++;

          // Add monitor object to store later, after possible beautification
          if (store) {
            mMonitorObjectStoreVector.push_back(mo);
          }

        } else {
          mLogger << AliceO2::InfoLogger::InfoLogger::Error << "The MO is null, probably a TObject could not be casted into an MO" << ENDM;
        }
      }
    }
  }

  auto qualityObjects = check(mMonitorObjects);

  store(qualityObjects);
  store(mMonitorObjectStoreVector);

  send(qualityObjects, ctx.outputs());

  // Update global revision number
  updateRevision();

  // monitoring
  if (timer.isTimeout()) {
    timer.reset(1000000); // 10 s.
    mCollector->send({ mTotalNumberObjectsReceived, "qc_objects_received" }, DerivedMetricMode::RATE);
    mCollector->send({ mTotalNumberCheckExecuted, "qc_checks_executed" }, DerivedMetricMode::RATE);
    mCollector->send({ mTotalNumberQOStored, "qc_qo_stored" }, DerivedMetricMode::RATE);
    mCollector->send({ mTotalNumberMOStored, "qc_mo_stored" }, DerivedMetricMode::RATE);
  }
}

void CheckRunner::update(std::shared_ptr<MonitorObject> mo)
{
  mMonitorObjects[mo->getFullName()] = mo;
  mMonitorObjectRevision[mo->getFullName()] = mGlobalRevision;
}

QualityObjectsType CheckRunner::check(std::map<std::string, std::shared_ptr<MonitorObject>> moMap)
{
  mLogger << "Trying " << mChecks.size() << " checks for " << moMap.size() << " monitor objects"
          << ENDM;

  QualityObjectsType allQOs;
  for (auto& check : mChecks) {
    if (check.isReady(mMonitorObjectRevision)) {
      auto newQOs = check.check(moMap);
      mTotalNumberCheckExecuted += newQOs.size();

      allQOs.insert(allQOs.end(), std::make_move_iterator(newQOs.begin()), std::make_move_iterator(newQOs.end()));
      newQOs.clear();

      // Was checked, update latest revision
      check.updateRevision(mGlobalRevision);
    } else {
      mLogger << "Monitor Objects for the check '" << check.getName() << "' are not ready, ignoring" << ENDM;
    }
  }
  return allQOs;
}

void CheckRunner::store(QualityObjectsType& qualityObjects)
{
  mLogger << "Storing " << qualityObjects.size() << " QualityObjects" << ENDM;
  try {
    for (auto& qo : qualityObjects) {
      mDatabase->storeQO(qo);
      mTotalNumberQOStored++;
    }
  } catch (boost::exception& e) {
    mLogger << "Unable to " << diagnostic_information(e) << ENDM;
  }
}

void CheckRunner::store(std::vector<std::shared_ptr<MonitorObject>>& monitorObjects)
{
  mLogger << "Storing " << monitorObjects.size() << " MonitorObjects" << ENDM;
  try {
    for (auto& mo : monitorObjects) {
      mDatabase->storeMO(mo);
      mTotalNumberMOStored++;
    }
  } catch (boost::exception& e) {
    mLogger << "Unable to " << diagnostic_information(e) << ENDM;
  }
}

void CheckRunner::send(QualityObjectsType& qualityObjects, framework::DataAllocator& allocator)
{
  // Note that we might send multiple QOs in one output, as separate parts.
  // This should be fine if they are retrieved on the other side with InputRecordWalker.

  mLogger << "Sending " << qualityObjects.size() << " quality objects" << ENDM;
  for (const auto& qo : qualityObjects) {

    const auto& correspondingCheck = std::find_if(mChecks.begin(), mChecks.end(), [checkName = qo->getCheckName()](const auto& check) {
      return check.getName() == checkName;
    });

    auto outputSpec = correspondingCheck->getOutputSpec();
    auto concreteOutput = framework::DataSpecUtils::asConcreteDataMatcher(outputSpec);
    allocator.snapshot(
      framework::Output{ concreteOutput.origin, concreteOutput.description, concreteOutput.subSpec, outputSpec.lifetime }, *qo);
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
  mDatabase = DatabaseFactory::create(mConfigFile->get<std::string>("qc.config.database.implementation"));
  mDatabase->connect(mConfigFile->getRecursiveMap("qc.config.database"));
  LOG(INFO) << "Database that is going to be used : ";
  LOG(INFO) << ">> Implementation : " << mConfigFile->get<std::string>("qc.config.database.implementation");
  LOG(INFO) << ">> Host : " << mConfigFile->get<std::string>("qc.config.database.host");
}

void CheckRunner::initMonitoring()
{
  std::string monitoringUrl = mConfigFile->get<std::string>("qc.config.monitoring.url", "infologger:///debug?qc");
  mCollector = MonitoringFactory::Get(monitoringUrl);
  mCollector->enableProcessMonitoring();
  mCollector->addGlobalTag(tags::Key::Subsystem, tags::Value::QC);
  mCollector->addGlobalTag("CheckRunnerName", mDeviceName);
  timer.reset(1000000); // 10 s.
}

} // namespace o2::quality_control::checker
