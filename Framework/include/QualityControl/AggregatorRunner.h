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
/// \file   AggregatorRunner.h
/// \author Barthelemy von Haller
///

#ifndef QC_CHECKER_AGGREGATORRUNNER_H
#define QC_CHECKER_AGGREGATORRUNNER_H

// O2
#include <Framework/Task.h>
#include <Framework/DataProcessorSpec.h>
#include <Common/Timer.h>
// QC
#include "QualityControl/QualityObject.h"
#include "QualityControl/UpdatePolicyManager.h"
#include "QualityControl/Activity.h"

namespace o2::framework
{
struct InputSpec;
struct OutputSpec;
class DataAllocator;
} // namespace o2::framework

namespace o2::monitoring
{
class Monitoring;
}

namespace o2::configuration
{
class ConfigurationInterface;
}

namespace AliceO2::Common
{
class Timer;
}

namespace o2::quality_control
{
namespace core
{
class ServiceDiscovery;
}
namespace checker
{
class Aggregator;
}
namespace repository
{
class DatabaseInterface;
}
} // namespace o2::quality_control

class TClass;

namespace o2::quality_control::checker
{
struct AggregatorSource;

/// \brief The class in charge of running the aggregators on the QualityObjects.
///
/// An AggregatorRunner is the device in charge of receiving data, handling the Aggregators and
/// calling them when the data is ready to be processed. It also initializes a few services such
/// as the monitoring.
/// At the moment, the aggregatorRunner also stores these new qualities in the repository.
/// At the moment, it is also a unique process although it could easily be updated to be able to run
/// in parallel.
///
/// \author Barthélémy von Haller
class AggregatorRunner : public framework::Task
{
 public:
  /// Constructor
  /**
   * \brief AggregatorRunner constructor
   * Create the AggregatorRunner device.
   *
   * @param configurationSource Path to configuration
   * @param checkRunnerOutputs List of checkRunners' output that it will take as inputs.
   */
  AggregatorRunner(const std::string& configurationSource, const vector<framework::OutputSpec> checkRunnerOutputs);

  /// Destructor
  ~AggregatorRunner() override;

  /// \brief AggregatorRunner init callback
  void init(framework::InitContext& ctx) override;

  /// \brief AggregatorRunner process callback
  void run(framework::ProcessingContext& ctx) override;

  framework::Inputs getInputs() { return mInputs; }
  std::string getDeviceName() { return mDeviceName; }
  const std::vector<std::shared_ptr<Aggregator>>& getAggregators() const { return mAggregators; }

  static std::string createAggregatorRunnerIdString() { return "QC-AGGREGATOR-RUNNER"; };
  static std::string createAggregatorRunnerName();
  static header::DataDescription createAggregatorRunnerDataDescription(const std::string& aggregatorName);

 private:
  /**
   * \brief For each aggregator, check if the data is ready and, if so, call its own aggregation method.
   *
   * For each aggregator, evaluate if data is ready (i.e. if its policy is fulfilled) and, if so,
   * call its `aggregate()` method.
   * This method is usually called upon reception of fresh inputs data.
   */
  core::QualityObjectsType aggregate();

  /**
   * \brief Store the QualityObjects in the database.
   *
   * @param qualityObjects QOs to be stored in DB.
   */
  void store(core::QualityObjectsType& qualityObjects);

  inline void initDatabase();
  inline void initMonitoring();
  inline void initServiceDiscovery();
  inline void initAggregators();

  /**
   * Reorder the aggregators stored in mAggregators.
   */
  void reorderAggregators();

  /**
   * Checks whether all sources provided are already in the aggregators vector.
   * The match is done by name.
   * @param sources
   * @param aggregators
   * @return true if all sources are found, by name, in the vector of aggregators.
   */
  bool areSourcesIn(const std::vector<AggregatorSource>& sources,
                    const std::vector<std::shared_ptr<Aggregator>>& aggregators);

  /**
   * Send metrics to the monitoring system if the time has come.
   */
  void sendPeriodicMonitoring();

  /// \brief Callback for CallbackService::Id::Start (DPL) a.k.a. RUN transition (FairMQ)
  void start(const framework::ServiceRegistry& services);
  /// \brief Callback for CallbackService::Id::Stop (DPL) a.k.a. STOP transition (FairMQ)
  void stop();
  /// \brief Callback for CallbackService::Id::Reset (DPL) a.k.a. RESET DEVICE transition (FairMQ)
  void reset();

  // General state
  std::string mDeviceName;
  core::Activity mActivity;
  std::vector<std::shared_ptr<Aggregator>> mAggregators;
  std::shared_ptr<o2::quality_control::repository::DatabaseInterface> mDatabase;
  std::shared_ptr<o2::configuration::ConfigurationInterface> mConfigFile;
  core::QualityObjectsMapType mQualityObjects; // where we cache the incoming quality objects and the output of the aggregators
  UpdatePolicyManager updatePolicyManager;

  // DPL
  o2::framework::Inputs mInputs;

  // monitoring
  std::shared_ptr<o2::monitoring::Monitoring> mCollector;
  AliceO2::Common::Timer mTimer;
  int mTotalNumberObjectsReceived;
  int mTotalNumberAggregatorExecuted;
  int mTotalNumberObjectsProduced;

  // Service discovery
  std::shared_ptr<core::ServiceDiscovery> mServiceDiscovery;
};

} // namespace o2::quality_control::checker

#endif // QC_CHECKER_AGGREGATORRUNNER_H
