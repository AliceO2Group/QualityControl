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
/// \file   AggregatorRunner.cxx
/// \author Barthelemy von Haller
///

#include "QualityControl/AggregatorRunner.h"

// O2
#include <Common/Exceptions.h>
#include <Configuration/ConfigurationFactory.h>
#include <Framework/DataSpecUtils.h>
#include <Monitoring/MonitoringFactory.h>
#include <Monitoring/Monitoring.h>
#include <Framework/InputRecordWalker.h>
// QC
#include "QualityControl/DatabaseFactory.h"
#include "QualityControl/QcInfoLogger.h"
#include "QualityControl/ServiceDiscovery.h"
#include "QualityControl/Aggregator.h"

using namespace AliceO2::Common;
using namespace AliceO2::InfoLogger;
using namespace o2::framework;
using namespace o2::configuration;
using namespace o2::quality_control::core;
using namespace o2::quality_control::repository;
using namespace std;
using namespace o2::monitoring;

const auto current_diagnostic = boost::current_exception_diagnostic_information;

namespace o2::quality_control::checker
{

AggregatorRunner::AggregatorRunner(const std::string& configurationSource, const vector<framework::OutputSpec> checkRunnerOutputs)
  : mDeviceName(createAggregatorRunnerName()),
    mTotalNumberObjectsReceived(0)
{
  try {
    mConfigFile = ConfigurationFactory::getConfiguration(configurationSource);
  } catch (...) {
    // catch the exceptions and print it (the ultimate caller might not know how to display it)
    ILOG(Fatal) << "Unexpected exception during initialization:\n"
                << boost::current_exception_diagnostic_information(true) << ENDM;
    throw;
  }

  // prepare list of all inputs
  // we cannot use the binding of the output because it is empty.
  int i = 0;
  for (const auto& spec : checkRunnerOutputs) {
    auto input = DataSpecUtils::matchingInput(spec);
    input.binding = "checkerOutput" + to_string(i++);
    mInputs.emplace_back(input);
  }
}

AggregatorRunner::~AggregatorRunner()
{
  if (mServiceDiscovery != nullptr) {
    mServiceDiscovery->deregister();
  }
}

header::DataDescription AggregatorRunner::createAggregatorRunnerDataDescription(const std::string& aggregatorName)
{
  if (aggregatorName.empty()) {
    BOOST_THROW_EXCEPTION(FatalException() << errinfo_details("Empty taskName for task's data description"));
  }
  o2::header::DataDescription description;
  description.runtimeInit(std::string(aggregatorName.substr(0, header::DataDescription::size)).c_str());
  return description;
}

std::string AggregatorRunner::createAggregatorRunnerName()
{
  return AggregatorRunner::createAggregatorRunnerIdString(); // there is only one thus we can just take the idString
}

void AggregatorRunner::init(framework::InitContext&)
{
  try {
    initInfologger();
    initDatabase();
    initMonitoring();
    initServiceDiscovery();
    initAggregators();
  } catch (...) {
    // catch the exceptions and print it (the ultimate caller might not know how to display it)
    ILOG(Fatal) << "Unexpected exception during initialization:\n"
                << current_diagnostic(true) << ENDM;
    throw;
  }
}

void AggregatorRunner::run(framework::ProcessingContext& ctx)
{
  framework::InputRecord& inputs = ctx.inputs();
  for (auto const& ref : InputRecordWalker(inputs)) { // InputRecordWalker because the output of CheckRunner can be multi-part
    ILOG(Debug, Trace) << "AggregatorRunner received data" << ENDM;
    shared_ptr<const QualityObject> qo = inputs.get<QualityObject*>(ref);
    if (qo != nullptr) {
      ILOG(Debug, Trace) << "   It is a qo: " << qo->getName() << ENDM;
      mQualityObjects[qo->getName()] = qo;
      mTotalNumberObjectsReceived++;
      updatePolicyManager.updateObjectRevision(qo->getName());
    }
  }

  auto qualityObjects = aggregate();
  store(qualityObjects);

  updatePolicyManager.updateGlobalRevision();
}

QualityObjectsType AggregatorRunner::aggregate()
{
  ILOG(Debug, Trace) << "Aggregate called in AggregatorRunner, QOs in cache: " << mQualityObjects.size() << ENDM;

  QualityObjectsType allQOs;
  for (auto const& aggregator : mAggregators) {
    string aggregatorName = aggregator->getName();
    ILOG(Info, Devel) << "Processing aggregator: " << aggregatorName << ENDM;

    if (updatePolicyManager.isReady(aggregatorName)) {
      ILOG(Info, Devel) << "   Quality Objects for the aggregator '" << aggregatorName << "' are  ready, aggregating" << ENDM;
      auto newQOs = aggregator->aggregate(mQualityObjects); // we give the whole list
      mTotalNumberObjectsProduced += newQOs.size();
      mTotalNumberAggregatorExecuted++;
      // we consider the output of the aggregators the same way we do the output of a check
      for (const auto& qo : newQOs) {
        mQualityObjects[qo->getName()] = qo;
        updatePolicyManager.updateObjectRevision(qo->getName());
      }

      allQOs.insert(allQOs.end(), std::make_move_iterator(newQOs.begin()), std::make_move_iterator(newQOs.end()));
      newQOs.clear();

      updatePolicyManager.updateActorRevision(aggregatorName); // Was aggregated, update latest revision
    } else {
      ILOG(Info, Devel) << "   Quality Objects for the aggregator '" << aggregatorName << "' are not ready, ignoring" << ENDM;
    }
  }
  return allQOs;
}

void AggregatorRunner::store(QualityObjectsType& qualityObjects)
{
  ILOG(Info, Devel) << "Storing " << qualityObjects.size() << " QualityObjects" << ENDM;
  try {
    for (auto& qo : qualityObjects) {
      mDatabase->storeQO(qo);
    }
  } catch (boost::exception& e) {
    ILOG(Info, Devel) << "Unable to " << diagnostic_information(e) << ENDM;
  }
}

void AggregatorRunner::initDatabase()
{
  mDatabase = DatabaseFactory::create(mConfigFile->get<std::string>("qc.config.database.implementation"));
  mDatabase->connect(mConfigFile->getRecursiveMap("qc.config.database"));
  ILOG(Info, Devel) << "Database that is going to be used : ";
  ILOG(Info, Devel) << ">> Implementation : " << mConfigFile->get<std::string>("qc.config.database.implementation");
  ILOG(Info, Devel) << ">> Host : " << mConfigFile->get<std::string>("qc.config.database.host");
}

void AggregatorRunner::initMonitoring()
{
  auto monitoringUrl = mConfigFile->get<std::string>("qc.config.monitoring.url", "infologger:///debug?qc");
  mCollector = MonitoringFactory::Get(monitoringUrl);
  mCollector->enableProcessMonitoring();
  mCollector->addGlobalTag(tags::Key::Subsystem, tags::Value::QC);
  mCollector->addGlobalTag("AggregatorRunnerName", mDeviceName);
  mTimer.reset(1000000); // 10 s.
}

void AggregatorRunner::initServiceDiscovery()
{
  auto consulUrl = mConfigFile->get<std::string>("qc.config.consul.url", "http://consul-test.cern.ch:8500");
  std::string url = ServiceDiscovery::GetDefaultUrl(ServiceDiscovery::DefaultHealthPort + 1); // we try to avoid colliding with the TaskRunner
  mServiceDiscovery = std::make_shared<ServiceDiscovery>(consulUrl, mDeviceName, mDeviceName, url);
  ILOG(Info, Devel) << "ServiceDiscovery initialized";
}

void AggregatorRunner::initInfologger()
{
  bool discardDebug = mConfigFile->get<bool>("qc.config.infologger.filterDiscardDebug", false);
  int discardLevel = mConfigFile->get<int>("qc.config.infologger.filterDiscardLevel", -1);
  ILOG_INST.init("Check", discardDebug, discardLevel );
}

void AggregatorRunner::initAggregators()
{
  ILOG(Info, Devel) << "Initialization of the aggregators" << ENDM;

  // Build aggregators based on the configurationd
  if (mConfigFile->getRecursive("qc").count("aggregators")) {

    // For every aggregator definition, create an Aggregator
    for (const auto& [aggregatorName, aggregatorConfig] : mConfigFile->getRecursive("qc.aggregators")) {
      ILOG(Info, Devel) << ">> Aggregator name : " << aggregatorName << ENDM;

      if (aggregatorConfig.get<bool>("active", true)) {
        try {
          auto aggregator = make_shared<Aggregator>(aggregatorName, aggregatorConfig);
          aggregator->init();
          updatePolicyManager.addPolicy(aggregator->getName(),
                                        aggregator->getPolicyName(),
                                        aggregator->getObjectsNames(),
                                        aggregator->getAllObjectsOption(),
                                        false);
          mAggregators.push_back(aggregator);
        } catch (...) {
          // catch the configuration exception and print it to avoid losing it
          ILOG(Error, Ops) << "Error creating aggregator '" << aggregatorName << "'"
                           << current_diagnostic(true) << ENDM;
          continue; // skip this aggregator, it might still fail fatally later if another aggregator depended on it
        }
      }
    }
  }

  reorderAggregators();
}

bool AggregatorRunner::areSourcesIn(const std::vector<AggregatorSource>& sources,
                                    const std::vector<std::shared_ptr<Aggregator>>& aggregators)
{
  for (auto source : sources) {
    auto it = find_if(aggregators.begin(), aggregators.end(),
                      [&](const std::shared_ptr<Aggregator>& agg) { return (agg->getName() == source.name); });
    if (it == aggregators.end()) {
      return false;
    }
  }

  return true;
}

void AggregatorRunner::reorderAggregators()
{
  // Implementation
  // This is a simple, light-weight, but sub-optimal implementation.
  // One could build a proper tree (e.g. with boost Graph) and then apply a complex algorithm to order
  // the nodes and find cycles.
  // Instead this implementation goes through the aggregators and for each checks whether
  // there are no dependencies or if they are all fulfilled. If it is the case the aggregator
  // is moved at the end of the resulting vector.
  // In case we have looped through all the remaining aggregators and nothing has been done,
  // ie. no aggregator has its dependencies fulfilled, we stop and raise an error.
  // This means that there is a cycle or that one of the dependencies does not exist.
  // Note that by "fulfilled" we mean that all the sources of an aggregator are already
  // in the result vector.

  std::vector<std::shared_ptr<Aggregator>> originals = mAggregators;
  std::vector<std::shared_ptr<Aggregator>> results;
  bool modificationLastIteration = true;
  // As long as there are items in original and we did some modifications in the last iteration
  while (!originals.empty() && modificationLastIteration) {
    modificationLastIteration = false;
    std::vector<std::shared_ptr<Aggregator>> toBeMoved; // we need it because we cannot modify the vectors while iterating over them
    // Loop over remaining items in the original list
    for (const auto& orig : originals) {
      // if no Aggregator dependencies or Aggregator sources are all already in result
      auto sources = orig->getSources(aggregator);
      if (sources.empty() || areSourcesIn(sources, results)) {
        // move from original to result
        toBeMoved.push_back(orig);
        modificationLastIteration = true;
      }
    }
    // move the items from one vector to the other
    for (const auto& item : toBeMoved) {
      results.push_back(item);
      originals.erase(std::remove(originals.begin(), originals.end(), item), originals.end());
    }
  }

  if (!originals.empty()) {
    string msg =
      "Error in the aggregators definition : either there is a cycle "
      "or an aggregator depends on an aggregator that does not exist.";
    ILOG(Error, Ops) << msg << ENDM;
    BOOST_THROW_EXCEPTION(FatalException() << errinfo_details(msg));
  }
  assert(results.size() != mAggregators.size());
  mAggregators = results;
}

void AggregatorRunner::sendPeriodicMonitoring()
{
  if (mTimer.isTimeout()) {
    mTimer.reset(1000000); // 10 s.
    mCollector->send({ mTotalNumberObjectsReceived, "qc_objects_received" }, DerivedMetricMode::RATE);
  }
}

} // namespace o2::quality_control::checker
