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
/// \file   TaskRunner.cxx
/// \author Barthelemy von Haller
/// \author Piotr Konopka
///

#include "QualityControl/TaskRunner.h"

#include <memory>

// O2
#include <Common/Exceptions.h>
#include <Configuration/ConfigurationFactory.h>
#include <Monitoring/MonitoringFactory.h>
#if __has_include(<Framework/DataSampling.h>)
#include <Framework/DataSampling.h>
#else
#include <DataSampling/DataSampling.h>
using namespace o2::utilities;
#endif
#include <Framework/CallbackService.h>
#include <Framework/CompletionPolicyHelpers.h>
#include <Framework/TimesliceIndex.h>
#include <Framework/DataSpecUtils.h>
#include <Framework/DataDescriptorQueryBuilder.h>
#include <Framework/ConfigParamRegistry.h>

#include "QualityControl/QcInfoLogger.h"
#include "QualityControl/TaskFactory.h"

#include <string>
#include <memory>
#include <boost/property_tree/ini_parser.hpp>

using namespace std;

const auto current_diagnostic = boost::current_exception_diagnostic_information;

namespace o2::quality_control::core
{

using namespace o2::framework;
using namespace o2::header;
using namespace o2::configuration;
using namespace o2::monitoring;
using namespace std::chrono;
using namespace AliceO2::Common;

TaskRunner::TaskRunner(const std::string& taskName, const std::string& configurationSource, size_t id)
  : mDeviceName(createTaskRunnerIdString() + "-" + taskName),
    mRunNumber(0),
    mMonitorObjectsSpec({ "mo" }, createTaskDataOrigin(), createTaskDataDescription(taskName), id)
{
  // setup configuration
  try {
    mConfigFile = ConfigurationFactory::getConfiguration(configurationSource);
    populateConfig(taskName, id);
  } catch (...) {
    // catch the configuration exception and print it to avoid losing it
    ILOG << LogFatalSupport << "Unexpected exception during configuration:\n"
         << current_diagnostic(true);
    throw;
  }
}

void TaskRunner::init(InitContext& iCtx)
{
  ILOG << LogInfoSupport << "initializing TaskRunner" << ENDM;

  // registering state machine callbacks
  iCtx.services().get<CallbackService>().set(CallbackService::Id::Start, [this, &options = iCtx.options()]() { start(options); });
  iCtx.services().get<CallbackService>().set(CallbackService::Id::Stop, [this]() { stop(); });
  iCtx.services().get<CallbackService>().set(CallbackService::Id::Reset, [this]() { reset(); });

  // setup monitoring
  std::string monitoringUrl = mConfigFile->get<std::string>("qc.config.monitoring.url", "infologger:///debug?qc");
  mCollector = MonitoringFactory::Get(monitoringUrl);
  mCollector->enableProcessMonitoring();
  mCollector->addGlobalTag(tags::Key::Subsystem, tags::Value::QC);
  mCollector->addGlobalTag("TaskName", mTaskConfig.taskName);

  // setup publisher
  mObjectsManager = std::make_shared<ObjectsManager>(mTaskConfig);

  // setup user's task
  TaskFactory f;
  mTask.reset(f.create(mTaskConfig, mObjectsManager));

  // init user's task
  mTask->loadCcdb(mTaskConfig.conditionUrl);
  mTask->initialize(iCtx);

  mNoMoreCycles = false;
  mCycleNumber = 0;
}

void TaskRunner::run(ProcessingContext& pCtx)
{
  if (mNoMoreCycles) {
    ILOG << LogInfoSupport << "The maximum number of cycles (" << mTaskConfig.maxNumberCycles << ") has been reached"
         << " or the device has received an EndOfStream signal. Won't start a new cycle." << ENDM;
    return;
  }

  if (!mCycleOn) {
    startCycle();
  }

  auto [dataReady, timerReady] = validateInputs(pCtx.inputs());

  if (dataReady) {
    mTask->monitorData(pCtx);
    mNumberMessages++;
  }

  if (timerReady) {
    finishCycle(pCtx.outputs());
    if (mResetAfterPublish) {
      mTask->reset();
    }
    if (mTaskConfig.maxNumberCycles < 0 || mCycleNumber < mTaskConfig.maxNumberCycles) {
      startCycle();
    } else {
      mNoMoreCycles = true;
    }
  }
}

CompletionPolicy::CompletionOp TaskRunner::completionPolicyCallback(o2::framework::CompletionPolicy::InputSet inputs)
{
  // fixme: we assume that there is one timer input and the rest are data inputs. If some other implicit inputs are
  //  added, this will break.
  size_t dataInputsExpected = inputs.size() - 1;
  size_t dataInputsPresent = 0;

  CompletionPolicy::CompletionOp action = CompletionPolicy::CompletionOp::Wait;

  for (auto& input : inputs) {
    if (input.header == nullptr || input.payload == nullptr) {
      continue;
    }

    const auto* dataHeader = CompletionPolicyHelpers::getHeader<DataHeader>(input);
    assert(dataHeader);

    if (!strncmp(dataHeader->dataDescription.str, "TIMER", 5)) {
      action = CompletionPolicy::CompletionOp::Consume;
    } else {
      dataInputsPresent++;
    }
  }

  LOG(DEBUG) << "Completion policy callback. "
             << "Total inputs possible: " << inputs.size()
             << ", data inputs: " << dataInputsPresent
             << ", timer inputs: " << (action == CompletionPolicy::CompletionOp::Consume);

  if (dataInputsPresent == dataInputsExpected) {
    action = CompletionPolicy::CompletionOp::Consume;
  }

  LOG(DEBUG) << "Action: " << action;

  return action;
}

void TaskRunner::setResetAfterPublish(bool resetAfterPublish) { mResetAfterPublish = resetAfterPublish; }

std::string TaskRunner::createTaskRunnerIdString()
{
  return std::string("QC-TASK-RUNNER");
}

header::DataOrigin TaskRunner::createTaskDataOrigin()
{
  return header::DataOrigin{ "QC" };
}

header::DataDescription TaskRunner::createTaskDataDescription(const std::string& taskName)
{
  if (taskName.empty()) {
    BOOST_THROW_EXCEPTION(FatalException() << errinfo_details("Empty taskName for task's data description"));
  }
  o2::header::DataDescription description;
  description.runtimeInit(std::string(taskName.substr(0, header::DataDescription::size - 3) + "-mo").c_str());
  return description;
}

void TaskRunner::endOfStream(framework::EndOfStreamContext& eosContext)
{
  ILOG << LogInfoSupport << "Received an EndOfStream, finishing the current cycle" << ENDM;
  finishCycle(eosContext.outputs());
  mNoMoreCycles = true;
}

void TaskRunner::start(const ConfigParamRegistry& options)
{
  try {
    mRunNumber = options.get<int>("runNumber");
    ILOG(Info) << "Run number found in options: " << mRunNumber << ENDM;
  } catch (std::invalid_argument& ia) {
    ILOG(Info) << "Run number not found in options, using 0 instead." << ENDM;
    mRunNumber = 0;
  }

  startOfActivity();

  if (mNoMoreCycles) {
    ILOG << LogInfoSupport << "The maximum number of cycles (" << mTaskConfig.maxNumberCycles << ") has been reached"
         << " or the device has received an EndOfStream signal. Won't start a new cycle." << ENDM;
    return;
  }

  startCycle();
}

void TaskRunner::stop()
{
  if (mCycleOn) {
    mTask->endOfCycle();
    mCycleNumber++;
    mCycleOn = false;
  }
  endOfActivity();
  mTask->reset();
  mRunNumber = 0;
}

void TaskRunner::reset()
{
  mTask.reset();
  mCollector.reset();
  mObjectsManager.reset();
  mRunNumber = 0;
}

std::tuple<bool /*data ready*/, bool /*timer ready*/> TaskRunner::validateInputs(const framework::InputRecord& inputs)
{
  size_t dataInputsPresent = 0;
  bool timerReady = false;

  for (auto& input : inputs) {
    if (input.header != nullptr && input.payload != nullptr) {

      const auto* dataHeader = get<DataHeader*>(input.header);
      assert(dataHeader);

      if (!strncmp(dataHeader->dataDescription.str, "TIMER", 5)) {
        timerReady = true;
      } else {
        dataInputsPresent++;
      }
    }
  }
  bool dataReady = dataInputsPresent == inputs.size() - 1;

  return { dataReady, timerReady };
}

void TaskRunner::populateConfig(std::string taskName, int id)
{
  auto tasksConfigList = mConfigFile->getRecursive("qc.tasks");
  auto taskConfigTree = tasksConfigList.find(taskName);
  if (taskConfigTree == tasksConfigList.not_found()) {
    std::string message = "No configuration found for task \"" + taskName + "\"";
    BOOST_THROW_EXCEPTION(AliceO2::Common::FatalException() << AliceO2::Common::errinfo_details(message));
  }

  mTaskConfig.taskName = taskName;
  string test = taskConfigTree->second.get<std::string>("detectorName", "MISC");
  mTaskConfig.detectorName = validateDetectorName(taskConfigTree->second.get<std::string>("detectorName", "MISC"));
  mTaskConfig.moduleName = taskConfigTree->second.get<std::string>("moduleName");
  mTaskConfig.className = taskConfigTree->second.get<std::string>("className");
  mTaskConfig.cycleDurationSeconds = taskConfigTree->second.get<int>("cycleDurationSeconds", 10);
  mTaskConfig.maxNumberCycles = taskConfigTree->second.get<int>("maxNumberCycles", -1);
  mTaskConfig.consulUrl = mConfigFile->get<std::string>("qc.config.consul.url", "http://consul-test.cern.ch:8500");
  mTaskConfig.conditionUrl = mConfigFile->get<std::string>("qc.config.conditionDB.url", "http://ccdb-test.cern.ch:8080");
  try {
    mTaskConfig.customParameters = mConfigFile->getRecursiveMap("qc.tasks." + taskName + ".taskParameters");
  } catch (...) {
    ILOG << LogInfoSupport << "No custom parameters for " << taskName << ENDM;
  }
  mTaskConfig.parallelTaskID = id;

  auto policiesFilePath = mConfigFile->get<std::string>("dataSamplingPolicyFile", "");
  ConfigurationInterface* config = policiesFilePath.empty() ? mConfigFile.get() : ConfigurationFactory::getConfiguration(policiesFilePath).get();
  auto policiesTree = config->getRecursive("dataSamplingPolicies");
  auto dataSourceTree = taskConfigTree->second.get_child("dataSource");
  std::string type = dataSourceTree.get<std::string>("type");

  if (type == "dataSamplingPolicy") {
    auto policyName = dataSourceTree.get<std::string>("name");
    ILOG << LogInfoSupport << "policyName : " << policyName << ENDM;
    mInputSpecs = DataSampling::InputSpecsForPolicy(config, policyName);
  } else if (type == "direct") {
    auto inputsQuery = dataSourceTree.get<std::string>("query");
    mInputSpecs = DataDescriptorQueryBuilder::parse(inputsQuery.c_str());
  } else {
    std::string message = std::string("Configuration error : dataSource type unknown : ") + type;
    BOOST_THROW_EXCEPTION(AliceO2::Common::FatalException() << AliceO2::Common::errinfo_details(message));
  }

  mInputSpecs.emplace_back(InputSpec{ "timer-cycle", createTaskDataOrigin(), createTaskDataDescription("TIMER-" + taskName), 0, Lifetime::Timer });
  mOptions.push_back({ "period-timer-cycle", framework::VariantType::Int, static_cast<int>(mTaskConfig.cycleDurationSeconds * 1000000), { "timer period" } });

  ILOG << LogInfoSupport << "Configuration loaded : " << ENDM;
  ILOG << LogInfoSupport << ">> Task name : " << mTaskConfig.taskName << ENDM;
  ILOG << LogInfoSupport << ">> Module name : " << mTaskConfig.moduleName << ENDM;
  ILOG << LogInfoSupport << ">> Detector name : " << mTaskConfig.detectorName << ENDM;
  ILOG << LogInfoSupport << ">> Cycle duration seconds : " << mTaskConfig.cycleDurationSeconds << ENDM;
  ILOG << LogInfoSupport << ">> Max number cycles : " << mTaskConfig.maxNumberCycles << ENDM;
}

std::string TaskRunner::validateDetectorName(std::string name)
{
  // name must be a detector code from DetID or one of the few allowed general names
  int nDetectors = 16;
  const char* detNames[16] = // once we can use DetID, remove this hard-coded list
    { "ITS", "TPC", "TRD", "TOF", "PHS", "CPV", "EMC", "HMP", "MFT", "MCH", "MID", "ZDC", "FT0", "FV0", "FDD", "ACO" };
  vector<string> permitted = { "MISC", "DAQ", "GENERAL", "TST", "BMK", "CTP", "TRG", "DCS" };
  for (auto i = 0; i < nDetectors; i++) {
    permitted.push_back(detNames[i]);
    //    permitted.push_back(o2::detectors::DetID::getName(i));
  }
  auto it = std::find(permitted.begin(), permitted.end(), name);

  if (it == permitted.end()) {
    std::string permittedString;
    for (auto i : permitted)
      permittedString += i + ' ';
    ILOG << LogErrorSupport << "Invalid detector name : " << name << "\n"
         << "    Placeholder 'MISC' will be used instead\n"
         << "    Note: list of permitted detector names :" << permittedString << ENDM;
    return "MISC";
  }
  return name;
}

void TaskRunner::startOfActivity()
{
  // stats
  mTimerTotalDurationActivity.reset();
  mTotalNumberObjectsPublished = 0;

  // We take the run number as set from the FairMQ options if it is there, otherwise the one from the config file
  int run = mRunNumber > 0 ? mRunNumber : mConfigFile->get<int>("qc.config.Activity.number");
  Activity activity(run,
                    mConfigFile->get<int>("qc.config.Activity.type"));
  mTask->startOfActivity(activity);
  mObjectsManager->updateServiceDiscovery();
}

void TaskRunner::endOfActivity()
{
  Activity activity(mRunNumber,
                    mConfigFile->get<int>("qc.config.Activity.type"));
  mTask->endOfActivity(activity);
  mObjectsManager->removeAllFromServiceDiscovery();

  double rate = mTotalNumberObjectsPublished / mTimerTotalDurationActivity.getTime();
  mCollector->send(Metric{ "qc_objects_published" }.addValue(rate, "per_second_whole_run"));
}

void TaskRunner::startCycle()
{
  QcInfoLogger::GetInstance() << "cycle " << mCycleNumber << " in " << mTaskConfig.taskName << ENDM;
  mTask->startOfCycle();
  mNumberMessages = 0;
  mNumberObjectsPublishedInCycle = 0;
  mTimerDurationCycle.reset();
  mCycleOn = true;
}

void TaskRunner::finishCycle(DataAllocator& outputs)
{
  mTask->endOfCycle();

  mNumberObjectsPublishedInCycle += publish(outputs);
  mTotalNumberObjectsPublished += mNumberObjectsPublishedInCycle;

  publishCycleStats();
  mObjectsManager->updateServiceDiscovery();

  mCycleNumber++;
  mCycleOn = false;

  if (mTaskConfig.maxNumberCycles == mCycleNumber) {
    ILOG << LogInfoSupport << "The maximum number of cycles (" << mTaskConfig.maxNumberCycles << ") has been reached."
         << " The task will not do anything from now on." << ENDM;
  }
}

void TaskRunner::publishCycleStats()
{
  double cycleDuration = mTimerDurationCycle.getTime();
  double rate = mNumberObjectsPublishedInCycle / (cycleDuration + mLastPublicationDuration);
  double wholeRunRate = mTotalNumberObjectsPublished / mTimerTotalDurationActivity.getTime();
  double totalDurationActivity = mTimerTotalDurationActivity.getTime();

  mCollector->send({ mNumberMessages, "qc_messages_received_in_cycle" });

  // monitoring metrics
  mCollector->send(Metric{ "qc_duration" }
                     .addValue(cycleDuration, "module_cycle")
                     .addValue(mLastPublicationDuration, "publication")
                     .addValue(totalDurationActivity, "activity_whole_run"));

  mCollector->send(Metric{ "qc_objects_published" }
                     .addValue(mNumberObjectsPublishedInCycle, "in_cycle")
                     .addValue(rate, "per_second")
                     .addValue(mTotalNumberObjectsPublished, "whole_run")
                     .addValue(wholeRunRate, "per_second_whole_run"));
}

int TaskRunner::publish(DataAllocator& outputs)
{
  ILOG << LogInfoSupport << "Send data from " << mTaskConfig.taskName << " len: " << mObjectsManager->getNumberPublishedObjects() << ENDM;
  AliceO2::Common::Timer publicationDurationTimer;

  auto concreteOutput = framework::DataSpecUtils::asConcreteDataMatcher(mMonitorObjectsSpec);
  // getNonOwningArray creates a TObjArray containing the monitoring objects, but not
  // owning them. The array is created by new and must be cleaned up by the caller
  std::unique_ptr<MonitorObjectCollection> array(mObjectsManager->getNonOwningArray());
  int objectsPublished = array->GetEntries();

  outputs.snapshot(
    Output{ concreteOutput.origin,
            concreteOutput.description,
            concreteOutput.subSpec,
            mMonitorObjectsSpec.lifetime },
    *array);

  mLastPublicationDuration = publicationDurationTimer.getTime();
  return objectsPublished;
}

} // namespace o2::quality_control::core
