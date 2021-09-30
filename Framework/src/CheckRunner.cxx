// Copyright 2019-2020 CERN and copyright holders of ALICE O2.
// See https://alice-o2.web.cern.ch/copyright for details of the copyright holders.
// All rights not expressly granted are reserved.
//
// This software is distributed under the terms of the GNU General Public
// License v3 (GPL Version 3), copied verbatim in the file "COPYING".
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.

///
/// \file   CheckRunner.cxx
/// \author Barthelemy von Haller
/// \author Piotr Konopka
/// \author Rafal Pacholek
///

#include "QualityControl/CheckRunner.h"

// O2
#include <Common/Exceptions.h>
#include <Configuration/ConfigurationFactory.h>
#include <Framework/DataSpecUtils.h>
#include <Monitoring/MonitoringFactory.h>
#include <Monitoring/Monitoring.h>
// QC
#include "QualityControl/DatabaseFactory.h"
#include "QualityControl/ServiceDiscovery.h"
#include "QualityControl/runnerUtils.h"
// Fairlogger
#include <fairlogger/Logger.h>

using namespace std::chrono;
using namespace AliceO2::Common;
using namespace AliceO2::InfoLogger;
using namespace o2::framework;
using namespace o2::configuration;
using namespace o2::monitoring;
using namespace o2::quality_control::core;
using namespace o2::quality_control::repository;
using namespace std;

const auto current_diagnostic = boost::current_exception_diagnostic_information;

namespace o2::quality_control::checker
{
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

std::string CheckRunner::createCheckRunnerFacility(std::string deviceName)
{
  // it starts with "check/" and is followed by the unique part of the device name truncated to a maximum of 32 characters.f
  string facilityName = "check/" + deviceName.substr(CheckRunner::createCheckRunnerIdString().length() + 1, string::npos);
  facilityName = facilityName.substr(0, 32);
  return facilityName;
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
    ILOG(Fatal, Ops) << "Unexpected exception during initialization:\n"
                     << boost::current_exception_diagnostic_information(true) << ENDM;
    throw;
  }
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
    ILOG(Fatal, Ops) << "Unexpected exception during initialization:\n"
                     << boost::current_exception_diagnostic_information(true) << ENDM;
    throw;
  }
}

CheckRunner::~CheckRunner()
{
  if (mServiceDiscovery != nullptr) {
    mServiceDiscovery->deregister();
  }
}

void CheckRunner::init(framework::InitContext& iCtx)
{
  InfoLoggerContext* ilContext = nullptr;
  try {
    ilContext = &iCtx.services().get<AliceO2::InfoLogger::InfoLoggerContext>();
  } catch (const RuntimeErrorRef& err) {
    ILOG(Error) << "Could not find the DPL InfoLogger Context." << ENDM;
  }

  try {
    ILOG_INST.init(createCheckRunnerFacility(mDeviceName), mConfigFile->getRecursive(), ilContext);
    initDatabase();
    initMonitoring();
    initServiceDiscovery();

    // registering state machine callbacks
    iCtx.services().get<CallbackService>().set(CallbackService::Id::Start, [this, &services = iCtx.services()]() { start(services); });
    iCtx.services().get<CallbackService>().set(CallbackService::Id::Stop, [this]() { stop(); });
    iCtx.services().get<CallbackService>().set(CallbackService::Id::Reset, [this]() { reset(); });

    for (auto& check : mChecks) {
      check.init();
      updatePolicyManager.addPolicy(check.getName(), check.getPolicyName(), check.getObjectsNames(), check.getAllObjectsOption(), false);
    }
  } catch (...) {
    // catch the exceptions and print it (the ultimate caller might not know how to display it)
    ILOG(Fatal, Ops) << "Unexpected exception during initialization:\n"
                     << current_diagnostic(true) << ENDM;
    throw;
  }
}

void CheckRunner::run(framework::ProcessingContext& ctx)
{
  prepareCacheData(ctx.inputs());

  auto qualityObjects = check();

  store(qualityObjects);
  store(mMonitorObjectStoreVector);

  send(qualityObjects, ctx.outputs());

  updatePolicyManager.updateGlobalRevision();

  sendPeriodicMonitoring();
  updateServiceDiscovery(qualityObjects);
}

void CheckRunner::prepareCacheData(framework::InputRecord& inputRecord)
{
  mMonitorObjectStoreVector.clear();

  for (const auto& input : mInputs) {
    auto dataRef = inputRecord.get(input.binding.c_str());
    if (dataRef.header != nullptr && dataRef.payload != nullptr) {

      // We don't know what we receive, so we test for an array and then try a tobject.
      // If we received a tobject, it gets encapsulated in the tobjarray.
      shared_ptr<TObjArray> array = nullptr;
      auto tobj = DataRefUtils::as<TObject>(dataRef);
      // if the object has not been found, it will raise an exception that we just let go.
      if (tobj->InheritsFrom("TObjArray")) {
        array.reset(dynamic_cast<TObjArray*>(tobj.release()));
        array->SetOwner(false);
        mLogger << AliceO2::InfoLogger::InfoLogger::Info << "CheckRunner " << mDeviceName
                << " received an array with " << array->GetEntries()
                << " entries from " << input.binding << ENDM;
      } else {
        // it is just a TObject not embedded in a TObjArray. We build a TObjArray for it.
        auto* newArray = new TObjArray();    // we cannot use `array` to add an object as it is const
        TObject* newTObject = tobj->Clone(); // we need a copy to avoid that it gets deleted behind our back.
        newArray->Add(newTObject);
        array.reset(newArray); // now that the array is ready we can adopt it.
        mLogger << AliceO2::InfoLogger::InfoLogger::Info << "CheckRunner " << mDeviceName
                << " received a tobject named " << tobj->GetName()
                << " from " << input.binding << ENDM;
      }

      // for each item of the array, check whether it is a MonitorObject. If not, create one and encapsulate.
      // Then, store the MonitorObject in the various maps and vectors we will use later.
      bool store = mInputStoreSet.count(DataSpecUtils::label(input)) > 0; // Check if this CheckRunner stores this input
      for (const auto tObject : *array) {
        std::shared_ptr<MonitorObject> mo{ dynamic_cast<MonitorObject*>(tObject) };

        if (mo == nullptr) {
          mLogger << AliceO2::InfoLogger::InfoLogger::Info << "The MO is null, probably a TObject could not be casted into an MO." << ENDM;
          mLogger << AliceO2::InfoLogger::InfoLogger::Info << "    Creating an ad hoc MO." << ENDM;
          header::DataOrigin origin = DataSpecUtils::asConcreteOrigin(input);
          mo = std::make_shared<MonitorObject>(tObject, input.binding, origin.str);
        }

        if (mo) {
          mo->setIsOwner(true);
          mMonitorObjects[mo->getFullName()] = mo;
          updatePolicyManager.updateObjectRevision(mo->getFullName());
          mTotalNumberObjectsReceived++;

          if (store) { // Monitor Object will be stored later, after possible beautification
            mMonitorObjectStoreVector.push_back(mo);
          }
        }
      }
    }
  }
}

void CheckRunner::sendPeriodicMonitoring()
{
  if (mTimer.isTimeout()) {
    mTimer.reset(10000000); // 10 s.
    mCollector->send({ mTotalNumberObjectsReceived, "qc_objects_received" }, DerivedMetricMode::RATE);
    mCollector->send({ mTotalNumberCheckExecuted, "qc_checks_executed" }, DerivedMetricMode::RATE);
    mCollector->send({ mTotalNumberQOStored, "qc_qo_stored" }, DerivedMetricMode::RATE);
    mCollector->send({ mTotalNumberMOStored, "qc_mo_stored" }, DerivedMetricMode::RATE);
  }
}

QualityObjectsType CheckRunner::check()
{
  mLogger << "Trying " << mChecks.size() << " checks for " << mMonitorObjects.size() << " monitor objects"
          << ENDM;

  QualityObjectsType allQOs;
  for (auto& check : mChecks) {
    if (updatePolicyManager.isReady(check.getName())) {
      auto newQOs = check.check(mMonitorObjects);
      mTotalNumberCheckExecuted += newQOs.size();

      allQOs.insert(allQOs.end(), std::make_move_iterator(newQOs.begin()), std::make_move_iterator(newQOs.end()));
      newQOs.clear();

      // Was checked, update latest revision
      updatePolicyManager.updateActorRevision(check.getName());
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
      qo->setActivity(mActivity);
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
      mo->setActivity(mActivity);
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

void CheckRunner::updateServiceDiscovery(const QualityObjectsType& qualityObjects)
{
  if (mServiceDiscovery == nullptr) {
    return;
  }

  // Insert into the list of paths the QOs' paths.
  // The list of paths cannot be computed during initialization and is therefore updated here.
  // It cannot be done in the init because of the case when the policy OnEachSeparately is used with a
  // data source specifying "all" MOs. As a consequence we have to check the QOs we actually receive.
  // A possible optimization would be to collect the list of QOs for all checks where it is possible (i.e.
  // all but OnEachSeparately with "All" MOs). If we can get all of them, then no need to update the list
  // after initialization. Otherwise, we set the list we know in init and then add to it as we go.
  size_t formerNumberQOsNames = mListAllQOPaths.size();
  for (const auto& qo : qualityObjects) {
    mListAllQOPaths.insert(qo->getPath());
  }
  // if nothing was inserted, no need to update
  if (mListAllQOPaths.size() == formerNumberQOsNames) {
    return;
  }

  // prepare the string of comma separated objects and publish it
  std::string objects;
  for (auto path : mListAllQOPaths) {
    objects += path + ",";
  }
  objects.pop_back(); // remove last comma
  mServiceDiscovery->_register(objects);
}

void CheckRunner::initDatabase()
{
  mDatabase = DatabaseFactory::create(mConfigFile->get<std::string>("qc.config.database.implementation"));
  mDatabase->connect(mConfigFile->getRecursiveMap("qc.config.database"));
  ILOG(Info, Support) << "Database that is going to be used : " << ENDM;
  ILOG(Info, Support) << ">> Implementation : " << mConfigFile->get<std::string>("qc.config.database.implementation") << ENDM;
  ILOG(Info, Support) << ">> Host : " << mConfigFile->get<std::string>("qc.config.database.host") << ENDM;
}

void CheckRunner::initMonitoring()
{
  auto monitoringUrl = mConfigFile->get<std::string>("qc.config.monitoring.url", "infologger:///debug?qc");
  mCollector = MonitoringFactory::Get(monitoringUrl);
  mCollector->addGlobalTag(tags::Key::Subsystem, tags::Value::QC);
  mCollector->addGlobalTag("CheckRunnerName", mDeviceName);
  mTimer.reset(10000000); // 10 s.
}

void CheckRunner::initServiceDiscovery()
{
  auto consulUrl = mConfigFile->get<std::string>("qc.config.consul.url", "");
  if (consulUrl.empty()) {
    mServiceDiscovery = nullptr;
    ILOG(Warning, Ops) << "Service Discovery disabled" << ENDM;
    return;
  }
  std::string url = ServiceDiscovery::GetDefaultUrl(ServiceDiscovery::DefaultHealthPort + 1); // we try to avoid colliding with the TaskRunner
  mServiceDiscovery = std::make_shared<ServiceDiscovery>(consulUrl, mDeviceName, mDeviceName, url);
  ILOG(Info, Support) << "ServiceDiscovery initialized" << ENDM;
}

void CheckRunner::start(const ServiceRegistry& services)
{
  mActivity.mId = computeRunNumber(services, mConfigFile->getRecursive());
  mActivity.mPeriodName = computePeriodName(services, mConfigFile->getRecursive());
  mActivity.mPassName = computePassName(mConfigFile->getRecursive());
  mActivity.mProvenance = computeProvenance(mConfigFile->getRecursive());
  ILOG(Info, Ops) << "Starting run " << mActivity.mId << ":"
                  << "\n   - period: " << mActivity.mPeriodName << "\n   - pass type: " << mActivity.mPassName << "\n   - provenance: " << mActivity.mProvenance << ENDM;
}

void CheckRunner::stop()
{
  ILOG(Info, Ops) << "Stopping run " << mActivity.mId << ENDM;
}

void CheckRunner::reset()
{
  ILOG(Info, Ops) << "Reset" << ENDM;

  try {
    mCollector.reset();
    mActivity = Activity();
  } catch (...) {
    // we catch here because we don't know where it will go in DPL's CallbackService
    ILOG(Error, Support) << "Error caught in reset() :\n"
                         << current_diagnostic(true) << ENDM;
    throw;
  }
}

} // namespace o2::quality_control::checker
