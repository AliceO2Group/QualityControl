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
  for (const auto& spec : checkRunnerOutputs) {
    auto input = DataSpecUtils::matchingInput(spec);
    input.binding = spec.binding.value;
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
  for (auto& aggregator : mAggregatorsMap) {
    ILOG(Info, Devel) << "Processing aggregator: " << aggregator.first << ENDM;

    string name = aggregator.second->getName();
    if (updatePolicyManager.isReady(name)) {
      ILOG(Info, Devel) << "   Quality Objects for the aggregator '" << name << "' are  ready, aggregating" << ENDM;
      auto newQOs = aggregator.second->aggregate(mQualityObjects); // we give the whole list
      mTotalNumberObjectsProduced += newQOs.size();
      mTotalNumberAggregatorExecuted++;
      // we consider the output of the aggregators the same way we do the output of a check
      for (const auto& qo : newQOs) {
        mQualityObjects[qo->getName()] = qo;
        updatePolicyManager.updateObjectRevision(qo->getName());
      }

      allQOs.insert(allQOs.end(), std::make_move_iterator(newQOs.begin()), std::make_move_iterator(newQOs.end()));
      newQOs.clear();

      updatePolicyManager.updateActorRevision(name); // Was aggregated, update latest revision
    } else {
      ILOG(Info, Devel) << "   Quality Objects for the aggregator '" << name << "' are not ready, ignoring" << ENDM;
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

void AggregatorRunner::initAggregators()
{
  ILOG(Info, Devel) << "Initialization of the aggregators" << ENDM;

  // Build aggregators based on the configurationd
  if (mConfigFile->getRecursive("qc").count("aggregators")) {

    // For every aggregator definition, create an Aggregator
    for (const auto& [aggregatorName, aggregatorConfig] : mConfigFile->getRecursive("qc.aggregators")) {

      ILOG(Info, Devel) << ">> Aggregator name : " << aggregatorName << ENDM;
      if (aggregatorConfig.get<bool>("active", true)) {
        // create Aggregator and store it.
        auto aggregator = make_shared<Aggregator>(aggregatorName, aggregatorConfig);
        aggregator->init();
        updatePolicyManager.addPolicy(aggregator->getName(), aggregator->getPolicyName(), aggregator->getObjectsNames(), aggregator->getAllObjectsOption(), false);
        mAggregatorsMap[aggregatorName] = aggregator;
      }
    }
  }
}

void AggregatorRunner::sendPeriodicMonitoring()
{
  if (mTimer.isTimeout()) {
    mTimer.reset(1000000); // 10 s.
    mCollector->send({ mTotalNumberObjectsReceived, "qc_objects_received" }, DerivedMetricMode::RATE);
  }
}

} // namespace o2::quality_control::checker
