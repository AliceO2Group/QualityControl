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
/// \file   AggregatorRunner.cxx
/// \author Barthelemy von Haller
///

#include "QualityControl/AggregatorRunner.h"

// O2
#include <Common/Exceptions.h>
#include <Monitoring/MonitoringFactory.h>
#include <Monitoring/Monitoring.h>
#include <Framework/InputRecordWalker.h>
#include <CommonUtils/ConfigurableParam.h>
#include <Framework/DataProcessorSpec.h>
#include <Framework/InitContext.h>
#include <Framework/ConfigParamRegistry.h>

#include <numeric>
#include <utility>
#include <TSystem.h>

// QC
#include "QualityControl/DatabaseFactory.h"
#include "QualityControl/QcInfoLogger.h"
#include "QualityControl/Aggregator.h"
#include "QualityControl/runnerUtils.h"
#include "QualityControl/InfrastructureSpecReader.h"
#include "QualityControl/AggregatorRunnerFactory.h"
#include "QualityControl/RootClassFactory.h"
#include "QualityControl/ConfigParamGlo.h"
#include "QualityControl/Bookkeeping.h"
#include "QualityControl/WorkflowType.h"
#include "QualityControl/DataHeaderHelpers.h"

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

AggregatorRunner::AggregatorRunner(AggregatorRunnerConfig arc, const std::vector<AggregatorConfig>& acs)
  : mDeviceName(createAggregatorRunnerName()),
    mRunnerConfig(std::move(arc)),
    mAggregatorsConfig(acs),
    mTotalNumberObjectsReceived(0),
    mTotalNumberAggregatorExecuted(0),
    mTotalNumberObjectsProduced(0)
{
  prepareInputs();
  prepareOutputs();
}

AggregatorRunner::~AggregatorRunner()
{
  ILOG(Debug, Trace) << "AggregatorRunner destructor (" << this << ")" << ENDM;
}

void AggregatorRunner::prepareInputs()
{
  std::set<std::string> alreadySeen;
  int i = 0;
  for (const auto& aggConfig : mAggregatorsConfig) {
    for (auto input : aggConfig.inputSpecs) {
      if (alreadySeen.count(input.binding) == 0) {
        alreadySeen.insert(input.binding);
        input.binding = "checkerOutput" + to_string(i++);
        mInputs.emplace_back(input);
      }
    }
  }
}

void AggregatorRunner::prepareOutputs()
{
  for (const auto& aggConfig : mAggregatorsConfig) {
    mOutputs.emplace_back(aggConfig.qoSpec);
  }
}

header::DataDescription AggregatorRunner::createAggregatorRunnerDataDescription(const std::string& aggregatorName)
{
  if (aggregatorName.empty()) {
    BOOST_THROW_EXCEPTION(FatalException() << errinfo_details("Empty taskName for task's data description"));
  }
  return quality_control::core::createDataDescription(aggregatorName, AggregatorRunner::descriptionHashLength);
}

std::string AggregatorRunner::createAggregatorRunnerName()
{
  return AggregatorRunner::createAggregatorRunnerIdString(); // there is only one thus we can just take the idString
}

void AggregatorRunner::init(framework::InitContext& iCtx)
{
  core::initInfologger(iCtx, mRunnerConfig.infologgerDiscardParameters, "aggregator");
  QcInfoLogger::setDetector(AggregatorRunner::getDetectorName(mAggregators));
  Bookkeeping::getInstance().init(mRunnerConfig.bookkeepingUrl);

  try {
    initLibraries(); // we have to load libraries before we load ConfigurableParams, otherwise the corresponding ROOT dictionaries won't be found
    // load config params
    if (!ConfigParamGlo::keyValues.empty()) {
      conf::ConfigurableParam::updateFromString(ConfigParamGlo::keyValues);
    }
    initDatabase();
    initMonitoring();
    initAggregators();
  } catch (...) {
    ILOG(Fatal) << "Unexpected exception during initialization: "
                << current_diagnostic(true) << ENDM;
    throw;
  }

  try {
    // registering state machine callbacks
    // FIXME: this is a workaround until we get some O2 PR in.
    iCtx.services().get<CallbackService>().set<CallbackService::Id::Start>([this, services = iCtx.services()]() mutable { start(services); });
    iCtx.services().get<CallbackService>().set<CallbackService::Id::Reset>([this]() { reset(); });
    iCtx.services().get<CallbackService>().set<CallbackService::Id::Stop>([this]() { stop(); });
  } catch (o2::framework::RuntimeErrorRef& ref) {
    ILOG(Error) << "Error during initialization: " << o2::framework::error_from_ref(ref).what << ENDM;
  }
}

void AggregatorRunner::run(framework::ProcessingContext& ctx)
{
  framework::InputRecord& inputs = ctx.inputs();
  for (auto const& ref : InputRecordWalker(inputs)) { // InputRecordWalker because the output of CheckRunner can be multi-part
    ILOG(Debug, Trace) << "AggregatorRunner received data" << ENDM;
    shared_ptr<const QualityObject> const qo = inputs.get<QualityObject*>(ref);
    if (qo != nullptr) {
      ILOG(Debug, Trace) << "   It is a qo: " << qo->getName() << ENDM;
      mQualityObjects[qo->getName()] = qo;
      mTotalNumberObjectsReceived++;
      mUpdatePolicyManager.updateObjectRevision(qo->getName());
    }
  }

  auto qualityObjects = aggregate();
  store(qualityObjects);
  send(qualityObjects, ctx.outputs());

  mUpdatePolicyManager.updateGlobalRevision();

  sendPeriodicMonitoring();
}

AggregatorRunner::QualityObjectsWithAggregatorNameVector AggregatorRunner::aggregate()
{
  ILOG(Debug, Trace) << "Aggregate called in AggregatorRunner, QOs in cache: " << mQualityObjects.size() << ENDM;

  QualityObjectsWithAggregatorNameVector allQOs;
  for (auto const& aggregator : mAggregators) {
    string aggregatorName = aggregator->getName();
    ILOG(Info, Devel) << "Processing aggregator: " << aggregatorName << ENDM;

    if (mUpdatePolicyManager.isReady(aggregatorName)) {
      ILOG(Info, Devel) << "   Quality Objects for the aggregator '" << aggregatorName << "' are ready, aggregating" << ENDM;
      auto newQOs = aggregator->aggregate(mQualityObjects, *mActivity); // we give the whole list
      mTotalNumberObjectsProduced += newQOs.size();
      mTotalNumberAggregatorExecuted++;
      // we consider the output of the aggregators the same way we do the output of a check
      for (const auto& qo : newQOs) {
        mQualityObjects[qo->getName()] = qo;
        mUpdatePolicyManager.updateObjectRevision(qo->getName());
      }

      allQOs.emplace_back(aggregatorName, newQOs);

      newQOs.clear();

      mUpdatePolicyManager.updateActorRevision(aggregatorName); // Was aggregated, update latest revision
    } else {
      ILOG(Info, Devel) << "   Quality Objects for the aggregator '" << aggregatorName << "' are not ready, ignoring" << ENDM;
    }
  }
  return allQOs;
}

void AggregatorRunner::store(QualityObjectsWithAggregatorNameVector& qualityObjectsWithAggregatorNames)
{
  const auto objectCount = std::accumulate(qualityObjectsWithAggregatorNames.begin(), qualityObjectsWithAggregatorNames.end(), 0, [](size_t count, const auto& namedQualityObject) {
    return namedQualityObject.second.size() + count;
  });

  ILOG(Info, Devel) << "Storing " << objectCount << " QualityObjects" << ENDM;

  auto validFrom = getCurrentTimestamp();
  try {
    for (const auto& [_, qualityObjects] : qualityObjectsWithAggregatorNames) {
      for (const auto& qo : qualityObjects) {
        mDatabase->storeQO(qo);
      }
    }

    if (!qualityObjectsWithAggregatorNames.empty() && !qualityObjectsWithAggregatorNames.front().second.empty()) {
      const auto& qo = qualityObjectsWithAggregatorNames.front().second.front();
      ILOG(Debug, Devel) << "Validity of QO '" << qo->GetName() << "' is (" << qo->getValidity().getMin() << ", " << qo->getValidity().getMax() << ")" << ENDM;
    }

  } catch (boost::exception& e) {
    ILOG(Info, Devel) << "Unable to " << diagnostic_information(e) << ENDM;
  }
}

void AggregatorRunner::send(const QualityObjectsWithAggregatorNameVector& qualityObjectsWithAggregatorNames, framework::DataAllocator& allocator)
{
  for (const auto& [aggregatorName, qualityObjects] : qualityObjectsWithAggregatorNames) {
    const auto concreteOutput = framework::DataSpecUtils::asConcreteDataMatcher(mAggregatorsMap.at(aggregatorName)->getConfig().qoSpec);
    for (const auto& qualityObject : qualityObjects) {
      allocator.snapshot(framework::Output{ concreteOutput.origin, concreteOutput.description, concreteOutput.subSpec }, *qualityObject);
    }
  }
}

void AggregatorRunner::initDatabase()
{
  mDatabase = DatabaseFactory::create(mRunnerConfig.database.at("implementation"));
  mDatabase->connect(mRunnerConfig.database);
  ILOG(Info, Devel) << "Database that is going to be used > Implementation : " << mRunnerConfig.database.at("implementation") << " / Host : " << mRunnerConfig.database.at("host") << ENDM;
}

void AggregatorRunner::initMonitoring()
{
  mCollector = MonitoringFactory::Get(mRunnerConfig.monitoringUrl);
  mCollector->enableProcessMonitoring();
  mCollector->addGlobalTag(tags::Key::Subsystem, tags::Value::QC);
  mCollector->addGlobalTag("AggregatorRunnerName", mDeviceName);
  mTimer.reset(1000000); // 10 s.
}

void AggregatorRunner::initAggregators()
{
  ILOG(Info, Devel) << "Initialization of the aggregators" << ENDM;

  // For every aggregator definition, create an Aggregator
  for (const auto& aggregatorConfig : mAggregatorsConfig) {
    ILOG(Info, Devel) << ">> Aggregator name : " << aggregatorConfig.name << ENDM;
    try {
      auto aggregator = make_shared<Aggregator>(aggregatorConfig);
      aggregator->init();
      mUpdatePolicyManager.addPolicy(aggregator->getName(),
                                     aggregator->getUpdatePolicyType(),
                                     aggregator->getObjectsNames(),
                                     aggregator->getAllObjectsOption(),
                                     false);
      mAggregators.push_back(aggregator);
      mAggregatorsMap.emplace(aggregator->getName(), aggregator);
    } catch (...) {
      // catch the configuration exception and print it to avoid losing it
      ILOG(Error, Ops) << "Error creating aggregator '" << aggregatorConfig.name << "'"
                       << current_diagnostic(true) << ENDM;
      continue; // skip this aggregator, it might still fail fatally later if another aggregator depended on it
    }
  }

  reorderAggregators();
}

void AggregatorRunner::initLibraries()
{
  std::set<std::string> moduleNames;
  for (const auto& config : mAggregatorsConfig) {
    moduleNames.insert(config.moduleName);
  }
  for (const auto& moduleName : moduleNames) {
    core::root_class_factory::loadLibrary(moduleName);
  }
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
      auto sources = orig->getSources(DataSourceType::Aggregator);
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
  assert(results.size() == mAggregators.size());
  mAggregators = results;
  mAggregatorsMap.clear();
  for (const auto& aggregator : mAggregators) {
    mAggregatorsMap.emplace(aggregator->getName(), aggregator);
  }
}

void AggregatorRunner::sendPeriodicMonitoring()
{
  if (mTimer.isTimeout()) {
    mTimer.reset(1000000); // 10 s.
    mCollector->send({ mTotalNumberObjectsReceived, "qc_aggregator_objects_received" });
    mCollector->send({ mTotalNumberAggregatorExecuted, "qc_aggregator_executed" });
    mCollector->send({ mTotalNumberObjectsProduced, "qc_aggregator_objects_produced" });
    mCollector->send({ mTimerTotalDurationActivity.getTime(), "qc_aggregator_duration" });
  }
}

void AggregatorRunner::start(ServiceRegistryRef services)
{
  mActivity = std::make_shared<Activity>(computeActivity(services, mRunnerConfig.fallbackActivity));
  mTimerTotalDurationActivity.reset();
  QcInfoLogger::setRun(mActivity->mId);
  QcInfoLogger::setPartition(mActivity->mPartitionName);
  ILOG(Info, Support) << "Starting run " << mActivity->mId << ENDM;
  for (auto& aggregator : mAggregators) {
    aggregator->startOfActivity(*mActivity);
  }

  // register ourselves to the BK
  if (!gSystem->Getenv("O2_QC_DONT_REGISTER_IN_BK")) { // Set this variable to disable the registration
    ILOG(Debug, Devel) << "Registering aggregator to BookKeeping" << ENDM;
    Bookkeeping::getInstance().registerProcess(mActivity->mId, mDeviceName, AggregatorRunner::getDetectorName(mAggregators), bkp::DplProcessType::QC_AGGREGATOR, "");
  }
}

void AggregatorRunner::stop()
{
  ILOG(Info, Support) << "Stopping run " << mActivity->mId << ENDM;
  for (auto& aggregator : mAggregators) {
    aggregator->endOfActivity(*mActivity);
  }
}

void AggregatorRunner::reset()
{
  ILOG(Info, Devel) << "Reset" << ENDM;

  try {
    mCollector.reset();
    mActivity = make_shared<Activity>();
  } catch (...) {
    // we catch here because we don't know where it will go in DPL's CallbackService
    ILOG(Error, Support) << "Error caught in reset() : "
                         << current_diagnostic(true) << ENDM;
    throw;
  }
}

std::string AggregatorRunner::getDetectorName(const std::vector<std::shared_ptr<Aggregator>>& aggregators)
{
  std::string detectorName;
  for (auto& aggregator : aggregators) {
    const std::string& thisDetector = aggregator->getDetector();
    if (detectorName.length() == 0) {
      detectorName = thisDetector;
    } else if (thisDetector != detectorName) {
      detectorName = "MANY";
      break;
    }
  }
  return detectorName;
}

} // namespace o2::quality_control::checker
