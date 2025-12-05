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
/// \file   TaskRunner.h
/// \author Piotr Konopka
/// \author Barthelemy von Haller
///

#ifndef QC_CORE_TASKRUNNER_H
#define QC_CORE_TASKRUNNER_H

// O2
#include <Common/Timer.h>
#include <Framework/Task.h>
#include <Framework/DataProcessorSpec.h>
#include <Framework/CompletionPolicy.h>
#include <Framework/DataTakingContext.h>
#include <Headers/DataHeader.h>
#include <Framework/ServiceRegistryRef.h>
// QC
#include "QualityControl/TaskRunnerConfig.h"
#include "QualityControl/ActorTraits.h"

namespace o2::configuration
{
class ConfigurationInterface;
}

namespace o2::monitoring
{
class Monitoring;
}

namespace o2::framework
{
class EndOfStreamContext;
class InputSpan;
class InitContext;
class ProcessingContext;
} // namespace o2::framework

namespace o2::quality_control::core
{

class Timekeeper;
class TaskInterface;
class ObjectsManager;

/// \brief A class driving the execution of a QC task inside DPL.
///
/// It is responsible for retrieving details about the task via the Configuration system and the Task (indirectly).
/// It then steers the execution of the task and provides it with O2 Data Model data, provided by framework.
/// It finally publishes the MonitorObjects owned and filled by the QC task and managed by the ObjectsManager.
/// Usage:
/// \code{.cxx}
/// TaskRunner qcTask{taskName, configurationSource, id};
/// DataProcessorSpec newTask{
///   taskName,
///   qcTask.getInputsSpecs(),
///   Outputs{ qcTask.getOutputSpec() },
///   AlgorithmSpec{},
///   qcTask.getOptions()
/// };
/// // this needs to be at the end
/// newTask.algorithm = adaptFromTask<TaskRunner>(std::move(qcTask));
/// \endcode
///
/// \author Piotr Konopka
/// \author Barthelemy von Haller
class TaskRunner : public framework::Task
{
  /// \brief Number of bytes in data description used for hashing of Task names. See HashDataDescription.h for details
  static constexpr size_t taskDescriptionHashLength = 4;
  static_assert(taskDescriptionHashLength <= o2::header::DataDescription::size);

 public:
  /// \brief Constructor
  ///
  /// \param taskName - name of the task, which exists in tasks list in the configuration file
  /// \param configurationSource - absolute path to configuration file, preceded with backend (f.e. "json://")
  /// \param id - subSpecification for taskRunner's OutputSpec, useful to avoid outputs collisions one more complex topologies
  TaskRunner(const TaskRunnerConfig& config);
  ~TaskRunner() override;

  /// \brief TaskRunner's init callback
  void init(framework::InitContext& iCtx) override;
  /// \brief TaskRunner's process callback
  void run(framework::ProcessingContext& pCtx) override;
  /// \brief TaskRunner's finaliseCCDB callback
  void finaliseCCDB(framework::ConcreteDataMatcher& matcher, void* obj) override;

  /// \brief TaskRunner's completion policy callback
  static framework::CompletionPolicy::CompletionOp completionPolicyCallback(o2::framework::InputSpan const& inputs, std::vector<framework::InputSpec> const&, framework::ServiceRegistryRef&);

  std::string getDeviceName() const { return mTaskConfig.deviceName; };
  const framework::Inputs& getInputsSpecs() const { return mTaskConfig.inputSpecs; };
  const framework::OutputSpec& getOutputSpec() const { return mTaskConfig.moSpec; };
  const framework::Options& getOptions() const { return mTaskConfig.options; };

  /// \brief Data Processor Label to identify all Task Runners
  static framework::DataProcessorLabel getTaskRunnerLabel() { return { "qc-task" }; }
  /// \brief ID string for all TaskRunner devices
  static std::string createTaskRunnerIdString();
  /// \brief Unified DataOrigin for Quality Control tasks
  static header::DataOrigin createTaskDataOrigin(const std::string& detectorCode, bool movingWindows = false);
  /// \brief Unified DataDescription naming scheme for all tasks
  static header::DataDescription createTaskDataDescription(const std::string& taskName);
  /// \brief Unified DataDescription naming scheme for all timers
  static header::DataDescription createTimerDataDescription(const std::string& taskName);

  /// \brief Callback for CallbackService::Id::EndOfStream
  void endOfStream(framework::EndOfStreamContext& eosContext) override;

 private:
  /// \brief Callback for CallbackService::Id::Start (DPL) a.k.a. RUN transition (FairMQ)
  void start(framework::ServiceRegistryRef services);
  /// \brief Callback for CallbackService::Id::Stop (DPL) a.k.a. STOP transition (FairMQ)
  void stop(framework::ServiceRegistryRef services);
  /// \brief Callback for CallbackService::Id::Reset (DPL) a.k.a. RESET DEVICE transition (FairMQ)
  void reset();

  /// \brief Checks if all the expected data inputs are present in the provided InputRecord
  static bool isDataReady(const framework::InputRecord& inputs);
  void printTaskConfig() const;
  void startOfActivity();
  void endOfActivity();
  void startCycle();
  void finishCycle(framework::DataAllocator& outputs);
  int publish(framework::DataAllocator& outputs);
  void publishCycleStats();
  void saveToFile();

 private:
  TaskRunnerConfig mTaskConfig;
  std::shared_ptr<monitoring::Monitoring> mCollector;
  std::shared_ptr<TaskInterface> mTask;
  std::shared_ptr<ObjectsManager> mObjectsManager;
  std::shared_ptr<Timekeeper> mTimekeeper;
  Activity mActivity;

  void updateMonitoringStats(framework::ProcessingContext& pCtx);
  void registerToBookkeeping();

  bool mCycleOn = false;
  bool mNoMoreCycles = false;
  int mCycleNumber = 0;
  framework::DeploymentMode mDeploymentMode = framework::DeploymentMode::Local;

  // stats
  int mNumberMessagesReceivedInCycle = 0;
  int mNumberObjectsPublishedInCycle = 0;
  int mTotalNumberObjectsPublished = 0; // over a run
  double mLastPublicationDuration = 0;
  uint64_t mDataReceivedInCycle = 0;
  AliceO2::Common::Timer mTimerTotalDurationActivity;
  AliceO2::Common::Timer mTimerDurationCycle;
};

template <>
struct ActorTraits<TaskRunner>
{
  constexpr static auto sActorTypeShort = "task";
  constexpr static auto sActorTypeKebabCase = "qc-task";
  constexpr static auto sActorTypeUpperCamelCase = "TaskRunner";
  constexpr static size_t sDataDescriptionHashLength = 4;

  constexpr static std::array<Service, 3> sRequiredServices = {Service::InfoLogger, Service::Monitoring, Service::Bookkeeping};
  constexpr static bkp::DplProcessType sDplProcessType{bkp::DplProcessType::QC_TASK};
  constexpr static std::array<DataSourceType, 2> sConsumedDataSources = {DataSourceType::DataSamplingPolicy, DataSourceType::Direct};
  constexpr static std::array<DataSourceType, 1> sPublishedDataSources = {DataSourceType::Task};

  constexpr static UserCodeInstanceCardinality sUserCodeInstanceCardinality = UserCodeInstanceCardinality::One;
  constexpr static bool sDetectorSpecific = true;
  constexpr static Criticality sCriticality = Criticality::UserDefined;
};

static_assert(ValidActorTraits<ActorTraits<TaskRunner>>);


} // namespace o2::quality_control::core

#endif // QC_CORE_TASKRUNNER_H
