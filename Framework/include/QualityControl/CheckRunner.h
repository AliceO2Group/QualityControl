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
/// \file   CheckRunner.h
/// \author Barthelemy von Haller
/// \author Piotr Konopka
///

#ifndef QC_CHECKER_CHECKER_H
#define QC_CHECKER_CHECKER_H

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
  CheckRunner(Check check, std::string configurationSource);
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

  /// \brief Unified DataDescription naming scheme for all checkers
  static o2::header::DataDescription createCheckRunnerDataDescription(const std::string taskName);
  static o2::framework::Inputs createInputSpec(const std::string checkName, const std::string configSource);

  std::string getDeviceName() { return mDeviceName; };
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
  std::vector<Check*> check(std::map<std::string, std::shared_ptr<MonitorObject>> moMap);

  /**
   * \brief Store the MonitorObject in the database.
   *
   * @param mo The MonitorObject to be stored in the database.
   */
  void store(std::vector<Check*>& checks);

  /**
   * \brief Send the MonitorObject on FairMQ to whoever is listening.
   */
  void send(std::vector<Check*>& checks, framework::DataAllocator& allocator);

  /**
   * \brief Update cached monitor object with new one.
   *
   * \param mo The MonitorObject to be updated
   */
  void update(std::shared_ptr<MonitorObject> mo);

  /**
   * \brief Collect input specs from Checks
   *
   * \param checks List of all checks
   */
  static o2::framework::Outputs collectOutputs(const std::vector<Check>& checks);

  inline void initDatabase();
  inline void initMonitoring();

  /**
   * \brief Increase the revision number for the Monitor Object.
   *
   * The revision number is an timeslot id for the monitor object.
   * It is assigned to an MO on receiving and is stored in mMonitorObjectRevision.
   * This function function should be called at the end of the receiving MOs.
   */
  void updateRevision();

  /**
   * \brief BSD checksum algorithm.
   *
   * \param input_string String intended to be hashed
   */
  static std::size_t hash(std::string input_string);

  // General state
  std::string mDeviceName;
  std::vector<Check> mChecks;
  o2::quality_control::core::QcInfoLogger& mLogger;
  std::shared_ptr<o2::quality_control::repository::DatabaseInterface> mDatabase;
  std::map<std::string, unsigned int> mMonitorObjectRevision;
  unsigned int mGlobalRevision = 1;
  std::unordered_set<std::string> mInputStoreSet;
  std::vector<std::shared_ptr<MonitorObject>> mMonitorObjectStoreVector;
  std::shared_ptr<o2::configuration::ConfigurationInterface> mConfigFile;

  // DPL
  o2::framework::Inputs mInputs;
  o2::framework::Outputs mOutputs;

  // Checks cache
  std::map<std::string, std::shared_ptr<MonitorObject>> mMonitorObjects;

  // monitoring
  std::shared_ptr<o2::monitoring::Monitoring> mCollector;
  int mTotalNumberObjectsReceived;
  int mTotalNumberCheckExecuted;
  int mTotalNumberQOStored;
  int mTotalNumberMOStored;
  AliceO2::Common::Timer timer;
};

} // namespace o2::quality_control::checker

#endif // QC_CHECKER_CHECKER_H
