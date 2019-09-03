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
#include <Framework/DataSampling.h>
#include <Framework/CallbackService.h>
#include <Framework/TimesliceIndex.h>
#include <Framework/DataSpecUtils.h>
#include <Framework/DataDescriptorQueryBuilder.h>
//#include <DetectorsCommonDataFormats/DetID.h>

#include "QualityControl/QcInfoLogger.h"
#include "QualityControl/TaskFactory.h"

#include <string>

using namespace std;

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
    mTask(nullptr),
    mResetAfterPublish(false),
    mMonitorObjectsSpec({ "mo" }, createTaskDataOrigin(), createTaskDataDescription(taskName), id),
    mNumberBlocks(0),
    mLastNumberObjects(0),
    mCycleOn(false),
    mCycleNumber(0),
    mTotalNumberObjectsPublished(0)
{
  // setup configuration
  mConfigFile = ConfigurationFactory::getConfiguration(configurationSource);
  populateConfig(taskName);
}

void TaskRunner::init(InitContext& iCtx)
{
  QcInfoLogger::GetInstance() << "initializing TaskRunner" << AliceO2::InfoLogger::InfoLogger::endm;

  // registering state machine callbacks
  iCtx.services().get<CallbackService>().set(CallbackService::Id::Start, [this]() { start(); });
  iCtx.services().get<CallbackService>().set(CallbackService::Id::Stop, [this]() { stop(); });
  iCtx.services().get<CallbackService>().set(CallbackService::Id::Reset, [this]() { reset(); });

  // setup monitoring
  std::string monitoringUrl = mConfigFile->get<std::string>("qc.config.monitoring.url", "infologger:///debug?qc"); // "influxdb-udp://aido2mon-gpn.cern.ch:8087"
  mCollector = MonitoringFactory::Get(monitoringUrl);
  mCollector->enableProcessMonitoring();

  // setup publisher
  mObjectsManager = std::make_shared<ObjectsManager>(mTaskConfig);

  // setup user's task
  TaskFactory f;
  mTask.reset(f.create(mTaskConfig, mObjectsManager));

  // init user's task
  mTask->loadCcdb(mTaskConfig.conditionUrl);
  mTask->initialize(iCtx);
}

void TaskRunner::run(ProcessingContext& pCtx)
{
  if (mTaskConfig.maxNumberCycles >= 0 && mCycleNumber >= mTaskConfig.maxNumberCycles) {
    LOG(INFO) << "The maximum number of cycles (" << mTaskConfig.maxNumberCycles << ") has been reached.";
    return;
  }

  if (!mCycleOn) {
    startCycle();
  }

  auto [dataReady, timerReady] = validateInputs(pCtx.inputs());

  if (dataReady) {
    mTask->monitorData(pCtx);
    mNumberBlocks++;
  }

  if (timerReady) {
    finishCycle(pCtx.outputs());
    if (mResetAfterPublish) {
      mTask->reset();
    }
    if (mTaskConfig.maxNumberCycles < 0 || mCycleNumber < mTaskConfig.maxNumberCycles) {
      startCycle();
    }
  }

  // if 10 s we publish stats
  if (mStatsTimer.isTimeout()) {
    double current = mStatsTimer.getTime();
    int objectsPublished = (mTotalNumberObjectsPublished - mLastNumberObjects);
    mLastNumberObjects = mTotalNumberObjectsPublished;
    mCollector->send({ objectsPublished / current, "QC_task_Rate_objects_published_per_10_seconds" });
    mStatsTimer.increment();
  }
}

CompletionPolicy::CompletionOp TaskRunner::completionPolicyCallback(gsl::span<PartRef const> const& inputs)
{
  // fixme: we assume that there is one timer input and the rest are data inputs. If some other implicit inputs are
  //  added, this will break.
  size_t dataInputsExpected = inputs.size() - 1;
  size_t dataInputsPresent = 0;
  size_t allInputs = 0;

  CompletionPolicy::CompletionOp action = CompletionPolicy::CompletionOp::Wait;

  for (auto& input : inputs) {
    if (input.header == nullptr || input.payload == nullptr) {
      continue;
    }

    const auto* dataHeader = get<DataHeader*>(input.header.get()->GetData());
    assert(dataHeader);

    if (!strncmp(dataHeader->dataDescription.str, "TIMER", 5)) {
      action = CompletionPolicy::CompletionOp::Consume;
    } else {
      dataInputsPresent++;
    }
  }

  LOG(DEBUG) << "Completion policy callback. "
             << "Total inputs possible: " << inputs.size()
             << ", inputs present: " << allInputs
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

void TaskRunner::start()
{
  startOfActivity();

  mStatsTimer.reset(10000000); // 10 s.
  mLastNumberObjects = 0;

  if (mTaskConfig.maxNumberCycles >= 0 && mCycleNumber >= mTaskConfig.maxNumberCycles) {
    LOG(INFO) << "The maximum number of cycles (" << mTaskConfig.maxNumberCycles << ") has been reached.";
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
}

void TaskRunner::reset()
{
  mTask.reset();
  mCollector.reset();
  mObjectsManager.reset();
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

void TaskRunner::populateConfig(std::string taskName)
{
  try {
    auto tasksConfigList = mConfigFile->getRecursive("qc.tasks");
    auto taskConfigTree = tasksConfigList.find(taskName);
    if (taskConfigTree == tasksConfigList.not_found()) {
      throw;
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
      LOG(INFO) << "No custom parameters for " << taskName;
    }

    auto policiesFilePath = mConfigFile->get<std::string>("dataSamplingPolicyFile", "");
    ConfigurationInterface* config = policiesFilePath.empty() ? mConfigFile.get() : ConfigurationFactory::getConfiguration(policiesFilePath).get();
    auto policiesTree = config->getRecursive("dataSamplingPolicies");
    auto dataSourceTree = taskConfigTree->second.get_child("dataSource");
    std::string type = dataSourceTree.get<std::string>("type");

    if (type == "dataSamplingPolicy") {
      auto policyName = dataSourceTree.get<std::string>("name");
      LOG(INFO) << "policyName : " << policyName;
      mInputSpecs = DataSampling::InputSpecsForPolicy(config, policyName);
    } else if (type == "direct") {
      auto inputsQuery = dataSourceTree.get<std::string>("query");
      mInputSpecs = DataDescriptorQueryBuilder::parse(inputsQuery.c_str());
    } else {
      std::string message = std::string("Configuration error : dataSource type unknown : ") + type; // TODO pass this message to the exception
      BOOST_THROW_EXCEPTION(AliceO2::Common::FatalException() << AliceO2::Common::errinfo_details(message));
    }

    mInputSpecs.emplace_back(InputSpec{ "timer-cycle", createTaskDataOrigin(), createTaskDataDescription("TIMER-" + taskName), 0, Lifetime::Timer });
    mOptions.push_back({ "period-timer-cycle", framework::VariantType::Int, static_cast<int>(mTaskConfig.cycleDurationSeconds * 1000000), { "timer period" } });
  } catch (...) { // catch already here the configuration exception and print it
    // because if we are in a constructor, the exception could be lost
    std::string diagnostic = boost::current_exception_diagnostic_information();
    LOG(ERROR) << "Unexpected exception, diagnostic information follows:\n"
               << diagnostic;
    throw;
  }
  LOG(INFO) << "Configuration loaded : ";
  LOG(INFO) << ">> Task name : " << mTaskConfig.taskName;
  LOG(INFO) << ">> Module name : " << mTaskConfig.moduleName;
  LOG(INFO) << ">> Detector name : " << mTaskConfig.detectorName;
  LOG(INFO) << ">> Cycle duration seconds : " << mTaskConfig.cycleDurationSeconds;
  LOG(INFO) << ">> Max number cycles : " << mTaskConfig.maxNumberCycles;
}

std::string TaskRunner::validateDetectorName(std::string name)
{
  // name must be a detector code from DetID or one of the few allowed general names
  int nDetectors = 16;
  const char* detNames[16] = // once we can use DetID, remove this hard-coded list
    {"ITS", "TPC", "TRD", "TOF", "PHS", "CPV", "EMC", "HMP", "MFT", "MCH", "MID", "ZDC", "FT0", "FV0", "FDD", "ACO"};
  vector<string> permitted = {"MISC", "DAQ", "GENERAL", "TST", "BMK", "CTP", "TRG", "DCS"};
  for(auto i = 0 ; i < nDetectors ; i++) {
    permitted.push_back(detNames[i]);
//    permitted.push_back(o2::detectors::DetID::getName(i));
  }
  auto it = std::find(permitted.begin(), permitted.end(), name);

  if (it == permitted.end()) {
    std::string permittedString;
    for (auto i: permitted) permittedString += i + ' ';
    LOG(ERROR) << "Invalid detector name : " << name << "\n" <<
                  "    Placeholder 'MISC' will be used instead\n" <<
                  "    Note: list of permitted detector names :" << permittedString;
    return "MISC";
  }
  return name;
}

void TaskRunner::startOfActivity()
{
  mTimerTotalDurationActivity.reset();
  Activity activity(mConfigFile->get<int>("qc.config.Activity.number"),
                    mConfigFile->get<int>("qc.config.Activity.type"));
  mTask->startOfActivity(activity);
}

void TaskRunner::endOfActivity()
{
  Activity activity(mConfigFile->get<int>("qc.config.Activity.number"),
                    mConfigFile->get<int>("qc.config.Activity.type"));
  mTask->endOfActivity(activity);

  double rate = mTotalNumberObjectsPublished / mTimerTotalDurationActivity.getTime();
  mCollector->send({ rate, "QC_task_Rate_objects_published_per_second_whole_run" });
  //  mCollector->send({ ba::mean(mPCpus), "QC_task_Mean_pcpu_whole_run" });
  //  mCollector->send({ ba::mean(mPMems), "QC_task_Mean_pmem_whole_run" });
}

void TaskRunner::startCycle()
{
  QcInfoLogger::GetInstance() << "cycle " << mCycleNumber << AliceO2::InfoLogger::InfoLogger::endm;
  mTask->startOfCycle();
  mNumberBlocks = 0;
  mCycleOn = true;
}

void TaskRunner::finishCycle(DataAllocator& outputs)
{
  mTask->endOfCycle();

  double durationCycle = 0; // (boost::posix_time::seconds(mTaskConfig.cycleDurationSeconds) -
                            // mCycleTimer->expires_from_now()).total_nanoseconds() / double(1e9);
  // mCycleTimer->expires_at(mCycleTimer->expires_at() +
  // boost::posix_time::seconds(mTaskConfig.cycleDurationSeconds));

  // publication
  unsigned long numberObjectsPublished = publish(outputs);
  mObjectsManager->updateServiceDiscovery();

  // monitoring metrics
  double durationPublication = 0; // (boost::posix_time::seconds(mTaskConfig.cycleDurationSeconds) -
                                  // mCycleTimer->expires_from_now()).total_nanoseconds() / double(1e9);
  mCollector->send({ mNumberBlocks, "QC_task_Numberofblocks_in_cycle" });
  mCollector->send({ durationCycle, "QC_task_Module_cycle_duration" });
  mCollector->send({ durationPublication, "QC_task_Publication_duration" });
  mCollector->send({ (int)numberObjectsPublished,
                     "QC_task_Number_objects_published_in_cycle" }); // cast due to Monitoring accepting only int
  double rate = numberObjectsPublished / (durationCycle + durationPublication);
  mCollector->send({ rate, "QC_task_Rate_objects_published_per_second" });
  mTotalNumberObjectsPublished += numberObjectsPublished;
  // std::vector<std::string> pidStatus = mMonitor->getPIDStatus(::getpid());
  // mPCpus(std::stod(pidStatus[3]));
  // mPMems(std::stod(pidStatus[4]));
  double whole_run_rate = mTotalNumberObjectsPublished / mTimerTotalDurationActivity.getTime();
  mCollector->send({ mTotalNumberObjectsPublished, "QC_task_Total_objects_published_whole_run" });
  mCollector->send({ mTimerTotalDurationActivity.getTime(), "QC_task_Total_duration_activity_whole_run" });
  mCollector->send({ whole_run_rate, "QC_task_Rate_objects_published_per_second_whole_run" });
  //    mCollector->send({std::stod(pidStatus[3]), "QC_task_Mean_pcpu_whole_run"});
  //  mCollector->send({ ba::mean(mPMems), "QC_task_Mean_pmem_whole_run" });

  mCycleNumber++;
  mCycleOn = false;

  if (mTaskConfig.maxNumberCycles == mCycleNumber) {
    LOG(INFO) << "The maximum number of cycles (" << mTaskConfig.maxNumberCycles << ") has been reached."
              << " The task will not do anything from now on.";
  }
}

unsigned long TaskRunner::publish(DataAllocator& outputs)
{
  auto concreteOutput = framework::DataSpecUtils::asConcreteDataMatcher(mMonitorObjectsSpec);
  outputs.adopt(
    Output{ concreteOutput.origin,
            concreteOutput.description,
            concreteOutput.subSpec,
            mMonitorObjectsSpec.lifetime },
    dynamic_cast<TObject*>(mObjectsManager->getNonOwningArray()));

  return 1;
}

} // namespace o2::quality_control::core
