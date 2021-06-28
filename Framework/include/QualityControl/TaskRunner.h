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

#include <boost/property_tree/ptree.hpp>

// O2
#include <Common/Timer.h>
#include <Framework/Task.h>
#include <Framework/DataProcessorSpec.h>
#include <Framework/CompletionPolicy.h>
#include <Framework/EndOfStreamContext.h>
#include <Framework/InputSpan.h>
#include <Headers/DataHeader.h>
#include <Framework/InitContext.h>
// QC
#include "QualityControl/TaskConfig.h"
#include "QualityControl/TaskInterface.h"

namespace o2::configuration
{
class ConfigurationInterface;
}

namespace o2::monitoring
{
class Monitoring;
}

namespace o2::quality_control::core
{

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
 public:
  /// \brief Constructor
  ///
  /// \param taskName - name of the task, which exists in tasks list in the configuration file
  /// \param configurationSource - absolute path to configuration file, preceded with backend (f.e. "json://")
  /// \param id - subSpecification for taskRunner's OutputSpec, useful to avoid outputs collisions one more complex topologies
  TaskRunner(const std::string& taskName, const std::string& configurationSource, size_t id = 0);
  ~TaskRunner() override = default;

  /// \brief TaskRunner's init callback
  void init(framework::InitContext& iCtx) override;
  /// \brief TaskRunner's process callback
  void run(framework::ProcessingContext& pCtx) override;

  /// \brief TaskRunner's completion policy callback
  static framework::CompletionPolicy::CompletionOp completionPolicyCallback(o2::framework::InputSpan const& inputs);

  std::string getDeviceName() { return mDeviceName; };
  const framework::Inputs& getInputsSpecs() { return mInputSpecs; };
  const framework::OutputSpec getOutputSpec() { return mMonitorObjectsSpec; };
  const framework::Options getOptions() { return mOptions; };

  void setResetAfterPublish(bool);

  /// \brief ID string for all TaskRunner devices
  static std::string createTaskRunnerIdString();
  /// \brief Unified DataOrigin for Quality Control tasks
  static header::DataOrigin createTaskDataOrigin();
  /// \brief Unified DataDescription naming scheme for all tasks
  static header::DataDescription createTaskDataDescription(const std::string& taskName);

  /// \brief Callback for CallbackService::Id::EndOfStream
  void endOfStream(framework::EndOfStreamContext& eosContext) override;

 private:
  /// \brief Callback for CallbackService::Id::Start (DPL) a.k.a. RUN transition (FairMQ)
  void start(const framework::ServiceRegistry& services);
  /// \brief Callback for CallbackService::Id::Stop (DPL) a.k.a. STOP transition (FairMQ)
  void stop();
  /// \brief Callback for CallbackService::Id::Reset (DPL) a.k.a. RESET DEVICE transition (FairMQ)
  void reset();

  std::tuple<bool /*data ready*/, bool /*timer ready*/> validateInputs(const framework::InputRecord&);
  void loadTaskConfig();
  void loadTopologyConfig();
  void startOfActivity();
  void endOfActivity();
  void startCycle();
  void finishCycle(framework::DataAllocator& outputs);
  int publish(framework::DataAllocator& outputs);
  void publishCycleStats();
  void saveToFile();

 private:
  std::string mDeviceName;
  TaskConfig mTaskConfig;
  std::shared_ptr<configuration::ConfigurationInterface> mConfigFile; // used in init only
  std::shared_ptr<monitoring::Monitoring> mCollector;
  std::shared_ptr<TaskInterface> mTask;
  bool mResetAfterPublish = false;
  std::shared_ptr<ObjectsManager> mObjectsManager;
  int mRunNumber;

  std::string validateDetectorName(std::string name) const;
  boost::property_tree::ptree getTaskConfigTree() const;
  void updateMonitoringStats(framework::ProcessingContext& pCtx);
  void computeRunNumber(const framework::ServiceRegistry& services);

  // consider moving these to TaskConfig
  framework::Inputs mInputSpecs;
  framework::OutputSpec mMonitorObjectsSpec;
  framework::Options mOptions;

  bool mCycleOn = false;
  bool mNoMoreCycles = false;
  int mCycleNumber = 0;

  // stats
  int mNumberMessagesReceivedInCycle = 0;
  int mNumberObjectsPublishedInCycle = 0;
  int mTotalNumberObjectsPublished = 0; // over a run
  double mLastPublicationDuration = 0;
  uint64_t mDataReceivedInCycle = 0;
  AliceO2::Common::Timer mTimerTotalDurationActivity;
  AliceO2::Common::Timer mTimerDurationCycle;
};

} // namespace o2::quality_control::core

#endif // QC_CORE_TASKRUNNER_H
