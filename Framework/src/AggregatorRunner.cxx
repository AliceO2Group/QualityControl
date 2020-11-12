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

#include <fairlogger/Logger.h>
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

AggregatorRunner::AggregatorRunner(const std::string& configurationSource, const vector<framework::OutputSpec> checkerRunnerOutputs)
  : mDeviceName(createAggregatorRunnerName()),
    mOutput({ "qo" }, createAggregatorRunnerDataDescription(mDeviceName), 0),
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
  int i = 0;
  for (const auto& spec : checkerRunnerOutputs) {
    auto input = DataSpecUtils::matchingInput(spec);
    input.binding = "checkerOutput" + to_string(i); // TODO check if that name is fine
    mInputs.emplace_back(input);
    i++;
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
    cout << "hello" << endl;
    initDatabase();
    initMonitoring();
    cout << "*1" << endl;
    initServiceDiscovery();
    cout << "*2" << endl;
    initAggregators();
    cout << "*3" << endl;
  } catch (...) {
    // catch the exceptions and print it (the ultimate caller might not know how to display it)
    ILOG(Fatal) << "Unexpected exception during initialization:\n"
                << current_diagnostic(true) << ENDM;
    throw;
  }
}

void AggregatorRunner::run(framework::ProcessingContext& ctx)
{
  cout << "run" << endl;
  // get data
  framework::InputRecord& inputs = ctx.inputs();
  for (auto const& ref : InputRecordWalker(inputs)) { // InputRecordWalker because the output of CheckRunner can be multi-part
    if (ref.header != nullptr) {
      ILOG(Info) << "Received data !" << ENDM;
      shared_ptr<const QualityObject> qo = inputs.get<QualityObject*>(ref);
      if (qo != nullptr) {
        ILOG(Info) << "it is a qo: " << qo->getName() << ENDM;
        mQualityObjects[qo->getName()] = qo;
        mTotalNumberObjectsReceived++;
        updatePolicyManager.updateObjectRevision(qo->getName());
      }
    }
  }

  auto qualityObjects = aggregate();

  store(qualityObjects);

  send(qualityObjects, ctx.outputs());

  updatePolicyManager.updateGlobalRevision();

}

QualityObjectsType AggregatorRunner::aggregate()
{
  ILOG(Info, Devel) << "Aggregate called in AggregatorRunner, QOs in cache: " << mQualityObjects.size() << ENDM;

  QualityObjectsType allQOs;
  for (auto& aggregator : mAggregatorsMap) {
    ILOG(Info, Devel) << "Processing aggregator: " << aggregator.first << ENDM;

    string name = aggregator.second->getName();
    if (updatePolicyManager.isReady(name)) {
      auto newQOs = aggregator.second->aggregate(mQualityObjects);
      mTotalNumberObjectsProduced += newQOs.size();
      mTotalNumberAggregatorExecuted++;
      // we consider the output of the aggregators the same way we do the output of a check
      for(const auto& qo : newQOs) {
        mQualityObjects[qo->getName()] = qo;
        updatePolicyManager.updateObjectRevision(qo->getName());
      }

      allQOs.insert(allQOs.end(), std::make_move_iterator(newQOs.begin()), std::make_move_iterator(newQOs.end()));
        newQOs.clear();

      updatePolicyManager.updateActorRevision(name); // Was aggregated, update latest revision
    } else {
      ILOG(Info) << "Quality Objects for the aggregator '" << name << "' are not ready, ignoring" << ENDM;
    }
  }
  return allQOs;
}

void AggregatorRunner::store(QualityObjectsType& qualityObjects)
{
  ILOG(Info) << "Storing " << qualityObjects.size() << " QualityObjects" << ENDM;
  try {
    for (auto& qo : qualityObjects) {
      mDatabase->storeQO(qo);
    }
  } catch (boost::exception& e) {
    ILOG(Info) << "Unable to " << diagnostic_information(e) << ENDM;
  }
}

void AggregatorRunner::send(QualityObjectsType& qualityObjects, framework::DataAllocator& /*allocator*/)
{
  // Note that we might send multiple QOs in one output, as separate parts.
  // This should be fine if they are retrieved on the other side with InputRecordWalker.

  ILOG(Info) << "Sending " << qualityObjects.size() << " quality objects" << ENDM;
//  for (const auto& qo : qualityObjects) {

    //    const auto& correspondingAggregator = std::find_if(mAggregators.begin(), mAggregators.end(), [aggregatorName = qo->getAggregatorName()](const auto& aggregator) {
    //      return aggregator.getName() == aggregatorName;
    //    });

    //    auto outputSpec = correspondingAggregator->getOutputSpec();
    //    auto concreteOutput = framework::DataSpecUtils::asConcreteDataMatcher(outputSpec);
    //    allocator.snapshot(
    //      framework::Output{ concreteOutput.origin, concreteOutput.description, concreteOutput.subSpec, outputSpec.lifetime }, *qo);
//  }
}

void AggregatorRunner::initDatabase()
{
  mDatabase = DatabaseFactory::create(mConfigFile->get<std::string>("qc.config.database.implementation"));
  mDatabase->connect(mConfigFile->getRecursiveMap("qc.config.database"));
  LOG(INFO) << "Database that is going to be used : ";
  LOG(INFO) << ">> Implementation : " << mConfigFile->get<std::string>("qc.config.database.implementation");
  LOG(INFO) << ">> Host : " << mConfigFile->get<std::string>("qc.config.database.host");
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
  LOG(INFO) << "ServiceDiscovery initialized";
}

void AggregatorRunner::initAggregators()
{
  cout << "init aggreg" << endl;
  // Build aggregators based on the configurationd
  if (mConfigFile->getRecursive("qc").count("aggregators")) {
    cout << "a" << endl;
    // For every aggregator definition, create an Aggregator
    for (const auto& [aggregatorName, aggregatorConfig] : mConfigFile->getRecursive("qc.aggregators")) {
      ILOG(Info) << ">> Aggregator name : " << aggregatorName << ENDM;
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
