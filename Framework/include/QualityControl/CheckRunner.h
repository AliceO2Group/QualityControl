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
#include <Headers/DataHeader.h>
#include <Monitoring/MonitoringFactory.h>
#include <Configuration/ConfigurationInterface.h>
#include <Framework/DataProcessorSpec.h>
// QC
#include "QualityControl/CheckInterface.h"
#include "QualityControl/DatabaseInterface.h"
#include "QualityControl/MonitorObject.h"
#include "QualityControl/QcInfoLogger.h"
#include "QualityControl/Check.h"
#include "QualityControl/UpdatePolicyManager.h"

namespace o2::quality_control::core
{
class ServiceDiscovery;
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

namespace o2::quality_control::checker
{

/// \brief The class in charge of running the checks on a MonitorObject.
///
/// A CheckRunner is in charge of loading/instantiating the proper checks for a given MonitorObject, to configure them
/// and to run them on the MonitorObject in order to generate a quality.
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
   * @param checkName Check name from the configuration
   * @param checkNames List of check names, that operate on the same inputs.
   * @param configurationSource Path to configuration
   */
  CheckRunner(std::vector<Check> checks, std::string configurationSource);

  /**
   * \brief CheckRunner constructor
   *
   * Create a sink for the Input. It is expected to receive Monitor Object to store.
   * It will not run any checks on a given input.
   *
   * @param input Monitor Object input spec.
   * @param configSource Path to configuration
   */
  CheckRunner(o2::framework::InputSpec input, std::string configurationSource);

  /// Destructor
  ~CheckRunner() override;

  /// \brief CheckRunner init callback
  void init(framework::InitContext& ctx) override;

  /// \brief CheckRunner process callback
  void run(framework::ProcessingContext& ctx) override;

  framework::Inputs getInputs() { return mInputs; };
  framework::Outputs getOutputs() { return mOutputs; };

  void setTaskStoreSet(std::unordered_set<std::string> storeSet) { mInputStoreSet = storeSet; }
  std::string getDeviceName() { return mDeviceName; };

  /// \brief Unified DataDescription naming scheme for all checkers
  static o2::header::DataDescription createCheckRunnerDataDescription(const std::string taskName);
  static o2::framework::Inputs createInputSpec(const std::string checkName, const std::string configSource);

  static std::string createCheckRunnerIdString() { return "QC-CHECK-RUNNER"; };
  static std::string createCheckRunnerName(std::vector<Check> checks);
  static std::string createSinkCheckRunnerName(o2::framework::InputSpec input);

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
  void store(QualityObjectsType& qualityObjects);

  /**
   * \brief Store the MonitorObjects in the database.
   *
   * @param monitorObjects MOs to be stored in DB.
   */
  void store(std::vector<std::shared_ptr<MonitorObject>>& monitorObjects);

  /**
   * \brief Send the QualityObjects on the DataProcessor output channel.
   */
  void send(QualityObjectsType& qualityObjects, framework::DataAllocator& allocator);

  /**
   * \brief Collect input specs from Checks
   *
   * \param checks List of all checks
   */
  static o2::framework::Outputs collectOutputs(const std::vector<Check>& checks);

  inline void initDatabase();
  inline void initMonitoring();
  inline void initServiceDiscovery();

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
  static std::size_t hash(std::string input_string);

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
  void start(const framework::ServiceRegistry& services);
  /// \brief Callback for CallbackService::Id::Stop (DPL) a.k.a. STOP transition (FairMQ)
  void stop();
  /// \brief Callback for CallbackService::Id::Reset (DPL) a.k.a. RESET DEVICE transition (FairMQ)
  void reset();

  // General state
  std::string mDeviceName;
  std::vector<Check> mChecks;
  int mRunNumber;
  o2::quality_control::core::QcInfoLogger& mLogger;
  std::shared_ptr<o2::quality_control::repository::DatabaseInterface> mDatabase;
  std::unordered_set<std::string> mInputStoreSet;
  std::vector<std::shared_ptr<MonitorObject>> mMonitorObjectStoreVector;
  std::shared_ptr<o2::configuration::ConfigurationInterface> mConfigFile;
  UpdatePolicyManager updatePolicyManager;

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
  AliceO2::Common::Timer mTimer;
};

} // namespace o2::quality_control::checker

#endif // QC_CHECKER_CHECKRUNNER_H
