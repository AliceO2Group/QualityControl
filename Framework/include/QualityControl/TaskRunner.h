///
/// \file   TaskDataProcessor.h
/// \author Piotr Konopka
/// \author Barthelemy von Haller
///

#ifndef QC_CORE_TASKRUNNER_H
#define QC_CORE_TASKRUNNER_H

#include <mutex>
#include <thread>
// boost (should be first but then it makes errors in fairmq)
#include <boost/accumulators/accumulators.hpp>
#include <boost/accumulators/statistics.hpp>
#include <boost/asio.hpp>
#include <boost/serialization/array_wrapper.hpp>
// O2
#include "Common/Timer.h"
#include "Configuration/ConfigurationInterface.h"
#include "Framework/DataProcessorSpec.h"
#include "Monitoring/MonitoringFactory.h"
// QC
#include "QualityControl/TaskConfig.h"
#include "QualityControl/TaskInterface.h"

namespace ba = boost::accumulators;

namespace o2
{
namespace quality_control
{
namespace core
{

using namespace o2::framework;
using namespace std::chrono;

/// \brief A class driving the execution of a QC task inside DPL.
///
/// It is responsible for retrieving details about the task via the Configuration system and the Task (indirectly).
/// It then steers the execution of the task and provides it with O2 Data Model data, provided by framework.
/// It finally publishes the MonitorObjects owned and filled by the QC task and managed by the ObjectsManager.
/// Usage:
/// \code{.cxx}
/// auto qcTask = std::make_shared<TaskDataProcessor>(taskName, configurationSource);
/// DataProcessorSpec newTask{
///   taskName,
///   qcTask->getInputsSpecs(),
///   Outputs{ qcTask->getOutputSpec() },
///   AlgorithmSpec{
///     (AlgorithmSpec::InitCallback) [qcTask = std::move(qcTask)](InitContext& initContext) {
///
///       qcTask->initCallback(initContext);
///
///       return (AlgorithmSpec::ProcessCallback) [qcTask = std::move(qcTask)] (ProcessingContext &processingContext) {
///         qcTask->processCallback(processingContext);
///       };
///     }
///   }
/// };
/// \endcode
///
/// \author Piotr Konopka
/// \author Barthelemy von Haller
class TaskRunner
{
 public:
  /// \brief Constructor
  ///
  /// \param taskName - name of the task, which exists in tasks list in the configuration file
  /// \param configurationSource - absolute path to configuration file, preceded with backend (f.e. "json://")
  /// \param id - subSpecification for taskRunner's OutputSpec, useful to avoid outputs collisions one more complex topologies
  TaskRunner(std::string taskName, std::string configurationSource, size_t id = 0);
  ~TaskRunner();

  /// \brief To be invoked during initialization of Data Processor
  void initCallback(InitContext& iCtx);
  /// \brief To be invoked inside Data Processor's main ProcessCallback
  void processCallback(ProcessingContext& pCtx);
  /// \brief To be invoked inside Data Processor's TimerCallback
  void timerCallback(ProcessingContext& pCtx);

  const Inputs& getInputsSpecs() { return mInputSpecs; };
  const OutputSpec getOutputSpec() { return mMonitorObjectsSpec; };

  void setResetAfterPublish(bool);

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

  void populateConfig(std::string taskName);
  void startOfActivity();
  void endOfActivity();
  void finishCycle(DataAllocator& outputs);
  unsigned long publish(DataAllocator& outputs);
  static void CustomCleanupTMessage(void* data, void* object);

 private:
  std::string mTaskName;
  TaskConfig mTaskConfig;
  std::shared_ptr<configuration::ConfigurationInterface> mConfigFile; // used in init only
  std::shared_ptr<monitoring::Monitoring> mCollector;
  std::unique_ptr<TaskInterface> mTask;
  bool mResetAfterPublish;
  std::shared_ptr<ObjectsManager> mObjectsManager;
  std::recursive_mutex mTaskMutex; // \todo should be plain mutex, when timer callback is implemented in dpl

  // consider moving these two to TaskConfig
  Inputs mInputSpecs;
  OutputSpec mMonitorObjectsSpec;

  int mNumberBlocks;
  int mLastNumberObjects;
  bool mCycleOn;
  int mCycleNumber;

  // stats
  AliceO2::Common::Timer mStatsTimer;
  int mTotalNumberObjectsPublished;
  AliceO2::Common::Timer mTimerTotalDurationActivity;
  ba::accumulator_set<double, ba::features<ba::tag::mean, ba::tag::variance>> mPCpus;
  ba::accumulator_set<double, ba::features<ba::tag::mean, ba::tag::variance>> mPMems;
};

} // namespace core
} // namespace quality_control
} // namespace o2

#endif // QC_CORE_TASKRUNNER_H
