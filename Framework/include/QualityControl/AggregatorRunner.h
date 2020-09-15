// Copyright CERN and copyright holders of ALICE O2. This software is
// distributed under the terms of the GNU General Public License v3 (GPL
// Version 3), copied verbatim in the file "COPYING".
//
// See http://alice-o2.web.cern.ch/license for full licensing information.
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.

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
/// \file   AggregatorRunner.h
/// \author Barthelemy von Haller
///

#ifndef QC_CHECKER_AGGREGATORRUNNER_H
#define QC_CHECKER_AGGREGATORRUNNER_H

// O2
#include <Framework/Task.h>
#include <Framework/DataProcessorSpec.h>
#include <Configuration/ConfigurationInterface.h>
#include <Common/Timer.h>
// QC
#include "QualityControl/AggregatorInterface.h"
#include "QualityControl/DatabaseInterface.h"
#include "QualityControl/QcInfoLogger.h"
#include "QualityControl/Aggregator.h"

namespace o2::framework
{
struct InputSpec;
struct OutputSpec;
class DataAllocator;
}

namespace o2::monitoring
{
class Monitoring;
}

namespace o2::quality_control::core
{
class ServiceDiscovery;
}

class TClass;

namespace o2::quality_control::checker
{

/// \brief The class in charge of running the aggregators on the QualityObjects.
///
/// A AggregatorRunner is in charge of loading/instantiating the proper aggregators, to configure it/them
/// and to run them on the QualityObjects in order to generate new quality(ies).
/// At the moment, the aggregatorRunner also stores these new qualities in the repository.
///
/// TODO Evaluate whether we should have a dedicated device to store in the database.
///
/// \author Barthélémy von Haller
class AggregatorRunner : public framework::Task
{
 public:
  /// Constructor
  /**
   * \brief AggregatorRunner constructor
   *
   * Create the AggregatorRunner device that will perform aggregations.
   *
   * @param aggregatorName Aggregator name from the configuration
   * @param aggregatorNames List of aggregator names, that operate on the same inputs.
   * @param configurationSource Path to configuration
   */
  AggregatorRunner(const std::string& configurationSource, const vector<framework::OutputSpec> checkerRunnerOutputs);

  /// Destructor
  ~AggregatorRunner() override;

  /// \brief AggregatorRunner init callback
  void init(framework::InitContext& ctx) override;

  /// \brief AggregatorRunner process callback
  void run(framework::ProcessingContext& ctx) override;

  framework::Inputs getInputs() { return mInputs; };
  framework::OutputSpec getOutput() { return mOutput; };
  std::string getDeviceName() { return mDeviceName; };

  static std::string createAggregatorRunnerIdString() { return "QC-AGGREGATOR-RUNNER"; };
  static std::string createAggregatorRunnerName();
  static header::DataDescription createAggregatorRunnerDataDescription(const std::string& aggregatorName);

 private:
  /**
   * \brief Evaluate the quality of a MonitorObject.
   *
   * The Aggregator's associated with this MonitorObject are run and a global quality is built by
   * taking the worse quality encountered. The MonitorObject is modified by setting its quality
   * and by calling the "beautifying" methods of the Aggregator's.
   *
   * @param mo The MonitorObject to evaluate and whose quality will be set according
   *        to the worse quality encountered while running the Aggregator's.
   */
  QualityObjectsType aggregate();

  /**
   * \brief Store the QualityObjects in the database.
   *
   * @param qualityObjects QOs to be stored in DB.
   */
  void store(QualityObjectsType& qualityObjects);

  /**
   * \brief Send the QualityObjects on the DataProcessor output channel.
   */
  void send(QualityObjectsType& qualityObjects, framework::DataAllocator& allocator);

  inline void initDatabase();
  inline void initMonitoring();
  inline void initServiceDiscovery();
  inline void initAggregators();

  // General state
  std::string mDeviceName;
  std::map<std::string, Aggregator> aggregatorsMap;
  std::shared_ptr<o2::quality_control::repository::DatabaseInterface> mDatabase;
  std::shared_ptr<o2::configuration::ConfigurationInterface> mConfigFile;

  // DPL
  o2::framework::Inputs mInputs;
  framework::OutputSpec mOutput;

  // monitoring
  std::shared_ptr<o2::monitoring::Monitoring> mCollector;
  AliceO2::Common::Timer mTimer;

  // Service discovery
  std::shared_ptr<ServiceDiscovery> mServiceDiscovery;
};

} // namespace o2::quality_control::aggregatorer

#endif // QC_CHECKER_AGGREGATORRUNNER_H
