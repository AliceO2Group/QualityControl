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
/// \file   CheckRunner.h
/// \author Barthelemy von Haller
/// \author Piotr Konopka
///

#ifndef QC_CHECKER_CHECKRUNNER_H
#define QC_CHECKER_CHECKRUNNER_H

// std & boost
#include <chrono>
#include <memory>
#include <string>
#include <map>
#include <vector>
#include <unordered_set>
// O2
#include <Common/Timer.h>
#include <Framework/Task.h>
#include <Framework/DataProcessorSpec.h>
// QC
#include "QualityControl/Activity.h"
#include "QualityControl/CheckRunnerConfig.h"
#include "QualityControl/Check.h"
#include "QualityControl/MonitorObject.h"
#include "QualityControl/QualityObject.h"
#include "QualityControl/UpdatePolicyManager.h"

namespace o2::quality_control::core
{
class ServiceDiscovery;
}

namespace o2::quality_control::repository
{
class DatabaseInterface;
}

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

class TClass;

// todo: do not expose other namespaces in headers
using namespace o2::quality_control::core;

namespace o2::quality_control::checker
{

/// \brief The class in charge of running the checks on a MonitorObject.
///
/// A CheckRunner is in charge of loading/instantiating the proper checks for a given MonitorObject, to configure them
/// and to run them on the MonitorObjects in order to generate a quality.
/// At the moment, a checker also stores quality in the repository.
///
/// TODO Evaluate whether we should have a dedicated device to store in the database.
///
/// \author Barthélémy von Haller
class CheckRunner : public framework::Task
{
 public:
  /// Constructor
  /**
   * \brief CheckRunner constructor
   *
   * Create CheckRunner device that will perform check operation with defineds checks.
   * Depending on the constructor, it can be a single check device or a group check device.
   * Group check assumes that the input of the checks is the same!
   *
   * @param checkRunnerConfig configuration of CheckRunner
   * @param checkConfigs configuration of all Checks that should run in this data processor
   */
  CheckRunner(CheckRunnerConfig, const std::vector<CheckConfig>& checkConfigs);

  /**
   * \brief CheckRunner constructor
   *
   * Create a sink for the Input. It is expected to receive Monitor Object to store.
   * It will not run any checks on a given input.
   *
   * @param checkRunnerConfig configuration of CheckRunner
   * @param input Monitor Object input spec.
   */
  CheckRunner(CheckRunnerConfig, o2::framework::InputSpec input);

  /// Destructor
  ~CheckRunner() override;

  /// \brief CheckRunner init callback
  void init(framework::InitContext& ctx) override;

  /// \brief CheckRunner process callback
  void run(framework::ProcessingContext& ctx) override;

  /// \brief Callback for CallbackService::Id::EndOfStream
  void endOfStream(framework::EndOfStreamContext& eosContext) override;

  framework::Inputs getInputs() { return mInputs; };
  framework::Outputs getOutputs() { return mOutputs; };

  void setTaskStoreSet(std::unordered_set<std::string> storeSet) { mInputStoreSet = storeSet; }
  std::string getDeviceName() { return mDeviceName; };

  static framework::DataProcessorLabel getCheckRunnerLabel() { return { "qc-check" }; }
  static std::string createCheckRunnerIdString() { return "qc-check"; };
  static std::string createCheckRunnerName(const std::vector<CheckConfig>& checks);
  static std::string createSinkCheckRunnerName(o2::framework::InputSpec input);
  static std::string createCheckRunnerFacility(std::string deviceName);

  /// \brief Compute the detector name to be used for this checkrunner.
  /// Compute the detector name to be used for this checkrunner.
  /// If all checks belong to the same detector we use it, otherwise we use "MANY"
  static std::string getDetectorName(const std::vector<CheckConfig> checks);

 private:
  /**
   * \brief Evaluate the quality of a MonitorObject.
   *
   * The Check's associated with this MonitorObject are run and a global quality is built by
   * taking the worse quality encountered. The MonitorObject is modified by setting its quality
   * and by calling the "beautifying" methods of the Check's.
   *
   * @param mo The MonitorObject to evaluate and whose quality will be set according
   *        to the worse quality encountered while running the Check's.
   */
  QualityObjectsType check();

  /**
   * \brief Store the QualityObjects in the database.
   *
   * @param qualityObjects QOs to be stored in DB.
   */
  void store(QualityObjectsType& qualityObjects, long validFrom);

  /**
   * \brief Store the MonitorObjects in the database.
   *
   * @param monitorObjects MOs to be stored in DB.
   */
  void store(std::vector<std::shared_ptr<MonitorObject>>& monitorObjects, long validFrom);

  /**
   * \brief Send the QualityObjects on the DataProcessor output channel.
   */
  void send(QualityObjectsType& qualityObjects, framework::DataAllocator& allocator);

  /**
   * \brief Collect input specs from Checks
   *
   * \param checks List of all checks
   */
  static o2::framework::Outputs collectOutputs(const std::vector<CheckConfig>& checks);

  void initDatabase();
  void initMonitoring();
  void initServiceDiscovery();
  void initInfologger(framework::InitContext& iCtx);
  void initLibraries();

  /**
   * Update the list of objects this TaskRunner is sending out.
   * @param qualityObjects
   */
  void updateServiceDiscovery(const QualityObjectsType& qualityObjects);

  /**
   * \brief BSD checksum algorithm.
   *
   * \param input_string String intended to be hashed
   */
  static std::size_t hash(const std::string& inputString);

  /**
   * \brief Massage/Prepare data from the Context and store it in the cache.
   * When data is received it can be 1. a TObjArray filled with MonitorObjects,
   * 2. a TObjArray filled with TObjects or 3. a TObject. The two latter happen
   * in case an external device is sending the data.
   * This method first transform the data in order to have a TObjArray of MonitorObjects.
   * It then stores these objects in the cache.
   * @param ctx
   */
  void prepareCacheData(framework::InputRecord& inputRecord);
  /**
   * Send metrics to the monitoring system if the time has come.
   */
  void sendPeriodicMonitoring();

  /// \brief Callback for CallbackService::Id::Start (DPL) a.k.a. RUN transition (FairMQ)
  void start(framework::ServiceRegistryRef services);
  /// \brief Callback for CallbackService::Id::Stop (DPL) a.k.a. STOP transition (FairMQ)
  void stop() override;
  /// \brief Callback for CallbackService::Id::Reset (DPL) a.k.a. RESET DEVICE transition (FairMQ)
  void reset();

  /// Refresh the configuration using the payload found in the fairmq options (if available)
  void refreshConfig(framework::InitContext& iCtx);

  // General state
  std::string mDeviceName;
  std::map<std::string, Check> mChecks;
  std::string mDetectorName;
  std::shared_ptr<Activity> mActivity; // shareable with the Checks
  CheckRunnerConfig mConfig;
  std::shared_ptr<o2::quality_control::repository::DatabaseInterface> mDatabase;
  std::unordered_set<std::string> mInputStoreSet;
  std::vector<std::shared_ptr<MonitorObject>> mMonitorObjectStoreVector;
  UpdatePolicyManager updatePolicyManager;
  bool mReceivedEOS = false;

  // DPL
  o2::framework::Inputs mInputs;
  o2::framework::Outputs mOutputs;

  // Checks cache
  std::map<std::string, std::shared_ptr<MonitorObject>> mMonitorObjects;

  // Service discovery
  std::shared_ptr<ServiceDiscovery> mServiceDiscovery;
  std::unordered_set<std::string> mListAllQOPaths; // store the names of all the QOs the Checks have generated so far

  // monitoring
  std::shared_ptr<o2::monitoring::Monitoring> mCollector;
  int mTotalNumberObjectsReceived;
  int mTotalNumberCheckExecuted;
  int mTotalNumberQOStored;
  int mTotalNumberMOStored;
  int mTotalQOSent;
  int mNumberQOStored = 0; // since the last publication of the monitoring data
  int mNumberMOStored = 0; // since the last publication of the monitoring data
  AliceO2::Common::Timer mTimer;
  AliceO2::Common::Timer mTimerTotalDurationActivity;
};

} // namespace o2::quality_control::checker

#endif // QC_CHECKER_CHECKRUNNER_H
