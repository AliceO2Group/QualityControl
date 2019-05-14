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
/// \file   TaskRunner.h
/// \author Piotr Konopka
/// \author Barthelemy von Haller
///

#ifndef QC_CORE_TASKRUNNER_H
#define QC_CORE_TASKRUNNER_H

// boost (should be first but then it makes errors in fairmq)
#include <boost/accumulators/accumulators.hpp>
#include <boost/accumulators/statistics.hpp>
#include <boost/asio.hpp>
#include <boost/serialization/array_wrapper.hpp>
// O2
#include <Common/Timer.h>
#include <Framework/Task.h>
#include <Framework/DataProcessorSpec.h>
#include <Framework/CompletionPolicy.h>
#include <Monitoring/MonitoringFactory.h>
#include <Configuration/ConfigurationInterface.h>
// QC
#include "QualityControl/TaskConfig.h"
#include "QualityControl/TaskInterface.h"

namespace ba = boost::accumulators;

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
/// // this needs to be moved at the end
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
  ~TaskRunner() override;

  /// \brief TaskRunner's init callback
  void init(framework::InitContext& iCtx) override;
  /// \brief TaskRunner's process callback
  void run(framework::ProcessingContext& pCtx) override;

  /// \brief TaskRunner's completion policy callback
  static framework::CompletionPolicy::CompletionOp completionPolicyCallback(gsl::span<framework::PartRef const> const& inputs);

  std::string getDeviceName() { return mDeviceName; };
  const framework::Inputs& getInputsSpecs() { return mInputSpecs; };
  const framework::OutputSpec getOutputSpec() { return mMonitorObjectsSpec; };
  const framework::Options getOptions() { return mOptions; };

  void setResetAfterPublish(bool);

  static std::string createTaskRunnerIdString();
  /// \brief Unified DataOrigin for Quality Control tasks
  static header::DataOrigin createTaskDataOrigin();
  /// \brief Unified DataDescription naming scheme for all tasks
  static header::DataDescription createTaskDataDescription(const std::string& taskName);

 private:
  /// \brief Callback for CallbackService::Id::Start (DPL) a.k.a. RUN transition (FairMQ)
  void start();
  /// \brief Callback for CallbackService::Id::Stop (DPL) a.k.a. STOP transition (FairMQ)
  void stop();
  /// \brief Callback for CallbackService::Id::Reset (DPL) a.k.a. RESET DEVICE transition (FairMQ)
  void reset();

  std::tuple<bool /*data ready*/, bool /*timer ready*/> validateInputs(const framework::InputRecord&);
  void populateConfig(std::string taskName);
  void startOfActivity();
  void endOfActivity();
  void startCycle();
  void finishCycle(framework::DataAllocator& outputs);
  unsigned long publish(framework::DataAllocator& outputs);

 private:
  std::string mDeviceName;
  TaskConfig mTaskConfig;
  std::shared_ptr<configuration::ConfigurationInterface> mConfigFile; // used in init only
  std::shared_ptr<monitoring::Monitoring> mCollector;
  std::shared_ptr<TaskInterface> mTask;
  bool mResetAfterPublish;
  std::shared_ptr<ObjectsManager> mObjectsManager;

  // consider moving these to TaskConfig
  framework::Inputs mInputSpecs;
  framework::OutputSpec mMonitorObjectsSpec;
  framework::Options mOptions;

  int mNumberBlocks;
  int mLastNumberObjects;
  bool mCycleOn;
  int mCycleNumber;
  std::chrono::steady_clock::time_point mCycleStartTime;

  // stats
  AliceO2::Common::Timer mStatsTimer;
  int mTotalNumberObjectsPublished;
  AliceO2::Common::Timer mTimerTotalDurationActivity;
  ba::accumulator_set<double, ba::features<ba::tag::mean, ba::tag::variance>> mPCpus;
  ba::accumulator_set<double, ba::features<ba::tag::mean, ba::tag::variance>> mPMems;
};

} // namespace o2::quality_control::core

#endif // QC_CORE_TASKRUNNER_H
