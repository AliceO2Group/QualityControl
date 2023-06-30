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
#include <Framework/DataSpecUtils.h>
#include <Monitoring/MonitoringFactory.h>
#include <Monitoring/Monitoring.h>
#include <CommonUtils/ConfigurableParam.h>

#include <utility>
// QC
#include "QualityControl/DatabaseFactory.h"
#include "QualityControl/ServiceDiscovery.h"
#include "QualityControl/runnerUtils.h"
#include "QualityControl/InfrastructureSpecReader.h"
#include "QualityControl/CheckRunnerFactory.h"
#include "QualityControl/RootClassFactory.h"
#include "QualityControl/ConfigParamGlo.h"
#include "QualityControl/Bookkeeping.h"

#include <TSystem.h>

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

std::size_t CheckRunner::hash(const std::string& inputString)
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

std::string CheckRunner::createCheckRunnerName(const std::vector<CheckConfig>& checks)
{
  static const std::string alphanumeric =
    "0123456789"
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
    "abcdefghijklmnopqrstuvwxyz";
  const int NAME_LEN = 4;
  std::string name(CheckRunner::createCheckRunnerIdString() + "-" + getDetectorName(checks) + "-");

  if (checks.size() == 1) {
    // If single check, use the check name
    name += checks[0].name;
  } else {
    std::string hash_string;
    std::vector<std::string> names;
    // Fill vector with check names
    for (const auto& c : checks) {
      names.push_back(c.name);
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
  // it starts with "check/" and is followed by the unique part of the device name truncated to a maximum of 32 characters.
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

o2::framework::Outputs CheckRunner::collectOutputs(const std::vector<CheckConfig>& checkConfigs)
{
  o2::framework::Outputs outputs;
  for (auto& check : checkConfigs) {
    outputs.push_back(check.qoSpec);
  }
  return outputs;
}

CheckRunner::CheckRunner(CheckRunnerConfig checkRunnerConfig, const std::vector<CheckConfig>& checkConfigs)
  : mDetectorName(getDetectorName(checkConfigs)),
    mDeviceName(createCheckRunnerName(checkConfigs)),
    mConfig(std::move(checkRunnerConfig)),
    /* All checks have the same Input */
    mInputs(checkConfigs.front().inputSpecs),
    mOutputs(CheckRunner::collectOutputs(checkConfigs)),
    mTotalNumberObjectsReceived(0),
    mTotalNumberCheckExecuted(0),
    mTotalNumberQOStored(0),
    mTotalNumberMOStored(0),
    mTotalQOSent(0)
{
  for (auto& checkConfig : checkConfigs) {
    mChecks.emplace(checkConfig.name, checkConfig);
  }
}

CheckRunner::CheckRunner(CheckRunnerConfig checkRunnerConfig, InputSpec input)
  : mDeviceName(createSinkCheckRunnerName(input)),
    mConfig(std::move(checkRunnerConfig)),
    mInputs{ input },
    mOutputs{},
    mTotalNumberObjectsReceived(0),
    mTotalNumberCheckExecuted(0),
    mTotalNumberQOStored(0),
    mTotalNumberMOStored(0),
    mTotalQOSent(0)
{
}

CheckRunner::~CheckRunner()
{
  ILOG(Debug, Trace) << "CheckRunner destructor (" << this << ")" << ENDM;
  if (mServiceDiscovery != nullptr) {
    mServiceDiscovery->deregister();
  }
}

void CheckRunner::refreshConfig(InitContext& iCtx)
{
  try {
    // get the tree
    auto updatedTree = iCtx.options().get<boost::property_tree::ptree>("qcConfiguration");

    if (updatedTree.empty()) {
      ILOG(Warning, Devel) << "Templated config tree is empty, we continue with the original one" << ENDM;
    } else {
      if (gSystem->Getenv("O2_QC_DEBUG_CONFIG_TREE")) { // until we are sure it works, keep a backdoor
        ILOG(Debug, Devel) << "We print the tree we got from the ECS via DPL : " << ENDM;
        printTree(updatedTree);
      }

      // prepare the information we need
      auto infrastructureSpec = InfrastructureSpecReader::readInfrastructureSpec(updatedTree);

      // Use the config to reconfigure the check runner.
      // The configs for the checks we find in the config and in our map are updated.
      // Topology changes are ignored: New checks are ignored. Removed checks are ignored.
      for (const auto& checkSpec : infrastructureSpec.checks) {
        // search if we have this check in this runner and replace it
        if (mChecks.find(checkSpec.checkName) != mChecks.end()) {
          auto checkConfig = Check::extractConfig(infrastructureSpec.common, checkSpec);
          mChecks.erase(checkConfig.name);
          mChecks.emplace(checkConfig.name, checkConfig);
          ILOG(Debug, Devel) << "Check " << checkSpec.checkName << " has been updated" << ENDM;
        }
      }
    }
  } catch (std::invalid_argument& error) {
    // ignore the error, we just skip the update of the config file. It can be legit, e.g. in command line mode
    ILOG(Warning, Devel) << "Could not get updated config tree in CheckRunner::init() - `qcConfiguration` could not be retrieved" << ENDM;
  } catch (...) {
    // we catch here because we don't know where it will get lost in DPL, and also we don't care if this part has failed.
    ILOG(Warning, Devel) << "Error caught in CheckRunner::refreshConfig(): "
                         << current_diagnostic(true) << ENDM;
  }
}

void CheckRunner::init(framework::InitContext& iCtx)
{
  try {
    initInfologger(iCtx);
    refreshConfig(iCtx);
    Bookkeeping::getInstance().init(mConfig.bookkeepingUrl);
    initDatabase();
    initMonitoring();
    initServiceDiscovery();
    initLibraries(); // we have to load libraries before we load ConfigurableParams, otherwise the corresponding ROOT dictionaries won't be found

    if (!ConfigParamGlo::keyValues.empty()) {
      conf::ConfigurableParam::updateFromString(ConfigParamGlo::keyValues);
    }
    // registering state machine callbacks
    iCtx.services().get<CallbackService>().set<CallbackService::Id::Start>([this, services = iCtx.services()]() mutable { start(services); });
    iCtx.services().get<CallbackService>().set<CallbackService::Id::Reset>([this]() { reset(); });
    iCtx.services().get<CallbackService>().set<CallbackService::Id::Stop>([this]() { stop(); });

    updatePolicyManager.reset();
    for (auto& [checkName, check] : mChecks) {
      check.init();
      updatePolicyManager.addPolicy(check.getName(), check.getUpdatePolicyType(), check.getObjectsNames(), check.getAllObjectsOption(), false);
    }
  } catch (...) {
    // catch the exceptions and print it (the ultimate caller might not know how to display it)
    ILOG(Fatal, Ops) << "Unexpected exception during initialization: "
                     << current_diagnostic(true) << ENDM;
    throw;
  }
}

long getCurrentTimestamp()
{
  auto now = std::chrono::system_clock::now();
  auto now_ms = std::chrono::time_point_cast<std::chrono::milliseconds>(now);
  auto epoch = now_ms.time_since_epoch();
  auto value = std::chrono::duration_cast<std::chrono::milliseconds>(epoch);
  return value.count();
}

void CheckRunner::run(framework::ProcessingContext& ctx)
{
  prepareCacheData(ctx.inputs());

  auto qualityObjects = check();

  // we want all objects that we are going to store to have the same validFrom values.
  // ideally it should be SOR or the moving window start, but before GUI allows for this,
  // we have to put the current timestamp.
  auto now = getCurrentTimestamp();
  store(qualityObjects, now);
  store(mMonitorObjectStoreVector, now);

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
        ILOG(Debug, Devel) << "CheckRunner " << mDeviceName
                           << " received an array with " << array->GetEntries()
                           << " entries from " << input.binding << ENDM;
      } else {
        // it is just a TObject not embedded in a TObjArray. We build a TObjArray for it.
        auto* newArray = new TObjArray();    // we cannot use `array` to add an object as it is const
        TObject* newTObject = tobj->Clone(); // we need a copy to avoid that it gets deleted behind our back.
        newArray->Add(newTObject);
        array.reset(newArray); // now that the array is ready we can adopt it.
        ILOG(Debug, Devel) << "CheckRunner " << mDeviceName
                           << " received a tobject named " << tobj->GetName()
                           << " from " << input.binding << ENDM;
      }

      // for each item of the array, check whether it is a MonitorObject. If not, create one and encapsulate.
      // Then, store the MonitorObject in the various maps and vectors we will use later.
      bool store = mInputStoreSet.count(DataSpecUtils::label(input)) > 0; // Check if this CheckRunner stores this input
      for (const auto tObject : *array) {
        std::shared_ptr<MonitorObject> mo{ dynamic_cast<MonitorObject*>(tObject) };

        if (mo == nullptr) {
          ILOG(Debug, Devel) << "The MO is null, probably a TObject could not be casted into an MO." << ENDM;
          ILOG(Debug, Devel) << "    Creating an ad hoc MO." << ENDM;
          header::DataOrigin origin = DataSpecUtils::asConcreteOrigin(input);
          mo = std::make_shared<MonitorObject>(tObject, input.binding, "CheckRunner", origin.str);
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
    double timeSinceLastCall = mTimer.getTime();
    timeSinceLastCall = timeSinceLastCall == 0 ? 1 : timeSinceLastCall;
    mTimer.reset(10000000); // 10 s.
    double rateMOs = mNumberMOStored / timeSinceLastCall;
    double rateQOs = mNumberQOStored / timeSinceLastCall;
    mCollector->send({ mTotalNumberObjectsReceived, "qc_checkrunner_objects_received" });
    mCollector->send({ mTotalNumberCheckExecuted, "qc_checkrunner_checks_executed" });
    mCollector->send(Metric{ "qc_checkrunner_stored" }
                       .addValue(mTotalNumberMOStored, "mos")
                       .addValue(rateMOs, "mos_per_second")
                       .addValue(mTotalNumberQOStored, "qos")
                       .addValue(rateQOs, "qos_per_second"));
    mCollector->send({ mTotalQOSent, "qc_checkrunner_qo_sent" });
    mCollector->send({ mTimerTotalDurationActivity.getTime(), "qc_checkrunner_duration" });
    mNumberQOStored = 0;
    mNumberMOStored = 0;
  }
}

QualityObjectsType CheckRunner::check()
{
  ILOG(Debug, Devel) << "Trying " << mChecks.size() << " checks for " << mMonitorObjects.size() << " monitor objects"
                     << ENDM;

  QualityObjectsType allQOs;
  for (auto& [checkName, check] : mChecks) {
    if (updatePolicyManager.isReady(check.getName())) {
      auto newQOs = check.check(mMonitorObjects);
      mTotalNumberCheckExecuted += newQOs.size();

      allQOs.insert(allQOs.end(), std::make_move_iterator(newQOs.begin()), std::make_move_iterator(newQOs.end()));
      newQOs.clear();

      // Was checked, update latest revision
      updatePolicyManager.updateActorRevision(checkName);
    } else {
      ILOG(Info, Support) << "Monitor Objects for the check '" << checkName << "' are not ready, ignoring" << ENDM;
    }
  }
  return allQOs;
}

void CheckRunner::store(QualityObjectsType& qualityObjects, long validFrom)
{
  ILOG(Debug, Devel) << "Storing " << qualityObjects.size() << " QualityObjects" << ENDM;
  try {
    for (auto& qo : qualityObjects) {
      qo->setActivity(*mActivity);
      qo->getActivity().mValidity.set(validFrom, 0);
      mDatabase->storeQO(qo);
      mTotalNumberQOStored++;
      mNumberQOStored++;
    }
  } catch (boost::exception& e) {
    ILOG(Info, Support) << "Unable to " << diagnostic_information(e) << ENDM;
  }
}

void CheckRunner::store(std::vector<std::shared_ptr<MonitorObject>>& monitorObjects, long validFrom)
{
  ILOG(Debug, Devel) << "Storing " << monitorObjects.size() << " MonitorObjects" << ENDM;
  try {
    for (auto& mo : monitorObjects) {
      mo->setActivity(*mActivity);
      mo->getActivity().mValidity.set(validFrom, 0);
      mDatabase->storeMO(mo);
      mTotalNumberMOStored++;
      mNumberMOStored++;
    }
  } catch (boost::exception& e) {
    ILOG(Info, Support) << "Unable to " << diagnostic_information(e) << ENDM;
  }
}

void CheckRunner::send(QualityObjectsType& qualityObjects, framework::DataAllocator& allocator)
{
  // Note that we might send multiple QOs in one output, as separate parts.
  // This should be fine if they are retrieved on the other side with InputRecordWalker.

  ILOG(Debug, Devel) << "Sending " << qualityObjects.size() << " quality objects" << ENDM;
  for (const auto& qo : qualityObjects) {
    const auto& correspondingCheck = mChecks.at(qo->getCheckName());
    auto outputSpec = correspondingCheck.getOutputSpec();
    auto concreteOutput = framework::DataSpecUtils::asConcreteDataMatcher(outputSpec);
    allocator.snapshot(
      framework::Output{ concreteOutput.origin, concreteOutput.description, concreteOutput.subSpec, outputSpec.lifetime }, *qo);
    mTotalQOSent++;
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
  for (const auto& path : mListAllQOPaths) {
    objects += path + ",";
  }
  objects.pop_back(); // remove last comma
  mServiceDiscovery->_register(objects);
}

void CheckRunner::initDatabase()
{
  mDatabase = DatabaseFactory::create(mConfig.database.at("implementation"));
  mDatabase->connect(mConfig.database);
  ILOG(Info, Devel) << "Database that is going to be used > Implementation : " << mConfig.database.at("implementation") << " / Host : " << mConfig.database.at("host") << ENDM;
}

void CheckRunner::initMonitoring()
{
  mCollector = MonitoringFactory::Get(mConfig.monitoringUrl);
  mCollector->addGlobalTag(tags::Key::Subsystem, tags::Value::QC);
  mCollector->addGlobalTag("CheckRunnerName", mDeviceName);
  mTimer.reset(10000000); // 10 s.
}

void CheckRunner::initServiceDiscovery()
{
  if (mConfig.consulUrl.empty()) {
    mServiceDiscovery = nullptr;
    ILOG(Warning, Support) << "Service Discovery disabled" << ENDM;
    return;
  }
  mServiceDiscovery = std::make_shared<ServiceDiscovery>(mConfig.consulUrl, mDeviceName, mDeviceName);
  ILOG(Info, Support) << "ServiceDiscovery initialized" << ENDM;
}

void CheckRunner::initInfologger(framework::InitContext& iCtx)
{
  // TODO : the method should be merged with the other, similar, methods in *Runners

  InfoLoggerContext* ilContext = nullptr;
  AliceO2::InfoLogger::InfoLogger* il = nullptr;
  try {
    ilContext = &iCtx.services().get<AliceO2::InfoLogger::InfoLoggerContext>();
    il = &iCtx.services().get<AliceO2::InfoLogger::InfoLogger>();
  } catch (const RuntimeErrorRef& err) {
    ILOG(Error) << "Could not find the DPL InfoLogger." << ENDM;
  }

  mConfig.infologgerDiscardParameters.discardFile = templateILDiscardFile(mConfig.infologgerDiscardParameters.discardFile, iCtx);
  QcInfoLogger::init(createCheckRunnerFacility(mDeviceName),
                     mConfig.infologgerDiscardParameters,
                     il,
                     ilContext);
}

void CheckRunner::initLibraries()
{
  std::set<std::string> moduleNames;
  for (const auto& [_, check] : mChecks) {
    (void)_;
    moduleNames.insert(check.getConfig().moduleName);
  }
  for (const auto& moduleName : moduleNames) {
    core::root_class_factory::loadLibrary(moduleName);
  }
}

void CheckRunner::endOfStream(framework::EndOfStreamContext& eosContext)
{
  mReceivedEOS = true;
}

void CheckRunner::start(ServiceRegistryRef services)
{
  mActivity = std::make_shared<Activity>(computeActivity(services, mConfig.fallbackActivity));
  string partitionName = computePartitionName(services);
  QcInfoLogger::setRun(mActivity->mId);
  QcInfoLogger::setPartition(partitionName);
  ILOG(Info, Support) << "Starting run " << mActivity->mId << ":" << ENDM;
  ILOG(Debug, Devel) << "   period: " << mActivity->mPeriodName << " / pass type: " << mActivity->mPassName << " / provenance: " << mActivity->mProvenance << ENDM;
  mTimerTotalDurationActivity.reset();
  mCollector->setRunNumber(mActivity->mId);
  mReceivedEOS = false;
  for (auto& [checkName, check] : mChecks) {
    check.setActivity(mActivity);
  }
}

void CheckRunner::stop()
{
  ILOG(Info, Support) << "Stopping run " << mActivity->mId << ENDM;
  if (!mReceivedEOS) {
    ILOG(Warning, Devel) << "The STOP transition happened before an EndOfStream was received. The very last QC objects in this run might not have been stored." << ENDM;
  }
}

void CheckRunner::reset()
{
  ILOG(Info, Devel) << "Reset" << ENDM;

  try {
    mCollector.reset();
    mActivity = make_shared<Activity>();
  } catch (...) {
    // we catch here because we don't know where it will go in DPL's CallbackService
    ILOG(Error, Support) << "Error caught in reset() : "
                         << current_diagnostic(true) << ENDM;
    throw;
  }

  mTotalNumberObjectsReceived = 0;
  mTotalNumberCheckExecuted = 0;
  mTotalNumberMOStored = 0;
  mNumberMOStored = 0;
  mTotalNumberQOStored = 0;
  mNumberQOStored = 0;
  mTotalQOSent = 0;
}

std::string CheckRunner::getDetectorName(const std::vector<CheckConfig> checks)
{
  std::string detectorName;
  for (auto& check : checks) {
    const std::string& thisDetector = check.detectorName;
    if (detectorName.length() == 0) {
      detectorName = thisDetector;
    } else if (thisDetector != detectorName) {
      detectorName = "MANY";
      break;
    }
  }
  return detectorName;
}

} // namespace o2::quality_control::checker
