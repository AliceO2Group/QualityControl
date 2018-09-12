///
/// \file   TaskDataProcessor.h
/// \author Piotr Konopka
///

#ifndef QC_CORE_TASKDATAPROCESSOR_H
#define QC_CORE_TASKDATAPROCESSOR_H

#include <thread>
#include <mutex>
// boost (should be first but then it makes errors in fairmq)
#include <boost/serialization/array_wrapper.hpp>
#include <boost/accumulators/accumulators.hpp>
#include <boost/accumulators/statistics.hpp>
#include <boost/asio.hpp>
// O2
#include "Common/Timer.h"
#include "Configuration/ConfigurationInterface.h"
#include "Framework/DataProcessorSpec.h"
#include "Monitoring/MonitoringFactory.h"
// QC
#include "QualityControl/TaskInterfaceDPL.h"
#include "QualityControl/TaskConfig.h"

namespace ba = boost::accumulators;

namespace o2 {
namespace quality_control {
namespace core {

using namespace o2::framework;
using namespace std::chrono;

/// \brief A class driving the execution of a QC task inside DPL.
///
/// TaskDataProcessor is a port of TaskDevice class, adapted for usage inside Data Processing Layer.
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
class TaskDataProcessor {
 public:
  TaskDataProcessor(std::string taskName, std::string configurationSource);
  ~TaskDataProcessor();

  /// \brief To be invoked during initialization of Data Processor
  void initCallback(InitContext& iCtx);
  /// \brief To be invoked inside Data Processor's main ProcessCallback
  void processCallback(ProcessingContext& pCtx);
  /// \brief To be invoked inside Data Processor's TimerCallback
  void timerCallback(ProcessingContext& pCtx);

  const Inputs& getInputsSpecs() { return mInputSpecs; };
  const OutputSpec getOutputSpec() { return mMonitorObjectsSpec; };

  void setResetAfterPublish(bool);

  /// \brief Unified DataDescription naming scheme for all tasks
  static o2::header::DataDescription taskDataDescription(const std::string taskName);

 private:
  void populateConfig(std::string taskName);
  void startOfActivity();
  void endOfActivity();
  void finishCycle(DataAllocator& outputs);
  unsigned long publish(DataAllocator& outputs);
  static void CustomCleanupTMessage(void* data, void* object);

 private:
  std::string mTaskName;
  TaskConfig mTaskConfig;
  std::shared_ptr<o2::configuration::ConfigurationInterface> mConfigFile; // used in init only
  std::shared_ptr<o2::monitoring::Monitoring> mCollector;
  TaskInterfaceDPL* mTask;
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

#endif // QC_CORE_TASKDATAPROCESSOR_H
