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

#include <utility>
#include <TSystem.h>

// QC
#include "QualityControl/DatabaseFactory.h"
#include "QualityControl/QcInfoLogger.h"
#include "QualityControl/ServiceDiscovery.h"
#include "QualityControl/Aggregator.h"
#include "QualityControl/runnerUtils.h"
#include "QualityControl/InfrastructureSpecReader.h"
#include "QualityControl/AggregatorRunnerFactory.h"
#include "QualityControl/RootClassFactory.h"
#include "QualityControl/ConfigParamGlo.h"

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

AggregatorRunner::AggregatorRunner(AggregatorRunnerConfig arc, const std::vector<AggregatorConfig>& acs) //, const o2::quality_control::core::InfrastructureSpec& infrastructureSpec)
  : mDeviceName(createAggregatorRunnerName()),
    mRunnerConfig(std::move(arc)),
    mAggregatorsConfig(std::move(acs)),
    mTotalNumberObjectsReceived(0),
    mTotalNumberAggregatorExecuted(0),
    mTotalNumberObjectsProduced(0)
{
  prepareInputs();
}

AggregatorRunner::~AggregatorRunner()
{
  ILOG(Debug, Trace) << "AggregatorRunner destructor (" << this << ")" << ENDM;
  if (mServiceDiscovery != nullptr) {
    mServiceDiscovery->deregister();
  }
}

void AggregatorRunner::refreshConfig(InitContext& iCtx)
{
  try {
    auto updatedTree = iCtx.options().get<boost::property_tree::ptree>("qcConfiguration");

    if (updatedTree.empty()) {
      ILOG(Warning, Devel) << "Templated config tree is empty, we continue with the original one" << ENDM;
    } else {
      if (gSystem->Getenv("O2_QC_DEBUG_CONFIG_TREE")) { // until we are sure it works, keep a backdoor
        ILOG(Debug, Devel) << "We print the tree we got from the ECS via DPL : " << ENDM;
        printTree(updatedTree);
      }

      // read the config, prepare spec
      auto infrastructureSpec = InfrastructureSpecReader::readInfrastructureSpec(updatedTree);

      // replace the runner config
      mRunnerConfig = AggregatorRunnerFactory::extractRunnerConfig(infrastructureSpec.common);

      // replace the aggregators configs
      mAggregatorsConfig.clear();
      mAggregatorsConfig = AggregatorRunnerFactory::extractAggregatorsConfig(infrastructureSpec.common, infrastructureSpec.aggregators);

      // replace the inputs
      mInputs.clear();
      prepareInputs();

      ILOG(Debug, Devel) << "Configuration refreshed" << ENDM;
    }
  } catch (std::invalid_argument& error) {
    // ignore the error, we just skip the update of the config file. It can be legit, e.g. in command line mode
    ILOG(Warning, Devel) << "Could not get updated config tree in TaskRunner::init() - `qcConfiguration` could not be retrieved" << ENDM;
  } catch (...) {
    // we catch here because we don't know where it will get lost in DPL, and also we don't care if this part has failed.
    ILOG(Warning, Devel) << "Error caught in refreshConfig(): "
                         << current_diagnostic(true) << ENDM;
  }
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

void AggregatorRunner::init(framework::InitContext& iCtx)
{
  initInfoLogger(iCtx);

  refreshConfig(iCtx);
  QcInfoLogger::setDetector(AggregatorRunner::getDetectorName(mAggregators));

  try {
    initLibraries(); // we have to load libraries before we load ConfigurableParams, otherwise the corresponding ROOT dictionaries won't be found
    // load config params
    if (!ConfigParamGlo::keyValues.empty()) {
      conf::ConfigurableParam::updateFromString(ConfigParamGlo::keyValues);
    }
    initDatabase();
    initMonitoring();
    initServiceDiscovery();
    initAggregators();
  } catch (...) {
    ILOG(Fatal) << "Unexpected exception during initialization: "
                << current_diagnostic(true) << ENDM;
    throw;
  }

  try {
    // registering state machine callbacks
    // FIXME: this is a workaround until we get some O2 PR in.
#if __has_include(<Framework/Features.h>)
    iCtx.services().get<CallbackService>().set(CallbackService::Id::Start, [this, services = iCtx.services()]() mutable { start(services); });
#else
    iCtx.services().get<CallbackService>().set(CallbackService::Id::Start, [this, &services = iCtx.services()]() { start(services); });
#endif

    iCtx.services().get<CallbackService>().set(CallbackService::Id::Reset, [this]() { reset(); });
    iCtx.services().get<CallbackService>().set(CallbackService::Id::Stop, [this]() { stop(); });
  } catch (o2::framework::RuntimeErrorRef& ref) {
    ILOG(Error) << "Error during initialization: " << o2::framework::error_from_ref(ref).what << ENDM;
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

  sendPeriodicMonitoring();
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
      qo->setActivity(mActivity);
      mDatabase->storeQO(qo);
    }
  } catch (boost::exception& e) {
    ILOG(Info, Devel) << "Unable to " << diagnostic_information(e) << ENDM;
  }
}

void AggregatorRunner::initDatabase()
{
  mDatabase = DatabaseFactory::create(mRunnerConfig.database.at("implementation"));
  mDatabase->connect(mRunnerConfig.database);
  ILOG(Info, Support) << "Database that is going to be used > Implementation : " << mRunnerConfig.database.at("implementation") << " / Host : " << mRunnerConfig.database.at("host") << ENDM;
}

void AggregatorRunner::initMonitoring()
{
  mCollector = MonitoringFactory::Get(mRunnerConfig.monitoringUrl);
  mCollector->enableProcessMonitoring();
  mCollector->addGlobalTag(tags::Key::Subsystem, tags::Value::QC);
  mCollector->addGlobalTag("AggregatorRunnerName", mDeviceName);
  mTimer.reset(1000000); // 10 s.
}

void AggregatorRunner::initServiceDiscovery()
{
  auto consulUrl = mRunnerConfig.consulUrl;
  if (consulUrl.empty()) {
    mServiceDiscovery = nullptr;
    ILOG(Warning, Support) << "Service Discovery disabled" << ENDM;
    return;
  }
  mServiceDiscovery = std::make_shared<ServiceDiscovery>(consulUrl, mDeviceName, mDeviceName);
  ILOG(Info, Devel) << "ServiceDiscovery initialized";
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
      updatePolicyManager.addPolicy(aggregator->getName(),
                                    aggregator->getUpdatePolicyType(),
                                    aggregator->getObjectsNames(),
                                    aggregator->getAllObjectsOption(),
                                    false);
      mAggregators.push_back(aggregator);
    } catch (...) {
      // catch the configuration exception and print it to avoid losing it
      ILOG(Error, Ops) << "Error creating aggregator '" << aggregatorConfig.name << "'"
                       << current_diagnostic(true) << ENDM;
      continue; // skip this aggregator, it might still fail fatally later if another aggregator depended on it
    }
  }

  reorderAggregators();
}

void AggregatorRunner::initInfoLogger(InitContext& iCtx)
{
  // TODO : the method should be merged with the other, similar, methods in *Runners

  InfoLoggerContext* ilContext = nullptr;
  AliceO2::InfoLogger::InfoLogger* il = nullptr;
  try {
    ilContext = &iCtx.services().get<AliceO2::InfoLogger::InfoLoggerContext>();
    il = &iCtx.services().get<AliceO2::InfoLogger::InfoLogger>();
  } catch (const RuntimeErrorRef& err) {
    ILOG(Error) << "Could not find the DPL InfoLogger." << ENDM;
  }

  mRunnerConfig.infologgerDiscardParameters.discardFile = templateILDiscardFile(mRunnerConfig.infologgerDiscardParameters.discardFile, iCtx);
  QcInfoLogger::init("aggregator",
                     mRunnerConfig.infologgerDiscardParameters,
                     il,
                     ilContext);
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
  assert(results.size() != mAggregators.size());
  mAggregators = results;
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
  mActivity = computeActivity(services, mRunnerConfig.fallbackActivity);
  mTimerTotalDurationActivity.reset();
  string partitionName = computePartitionName(services);
  QcInfoLogger::setRun(mActivity.mId);
  QcInfoLogger::setPartition(partitionName);
  ILOG(Info, Support) << "Starting run " << mActivity.mId << "> period: " << mActivity.mPeriodName << " / pass type: " << mActivity.mPassName << " / provenance: " << mActivity.mProvenance << ENDM;
}

void AggregatorRunner::stop()
{
  ILOG(Info, Support) << "Stopping run " << mActivity.mId << ENDM;
}

void AggregatorRunner::reset()
{
  ILOG(Info, Devel) << "Reset" << ENDM;

  try {
    mCollector.reset();
    mActivity = Activity();
  } catch (...) {
    // we catch here because we don't know where it will go in DPL's CallbackService
    ILOG(Error, Support) << "Error caught in reset() : "
                         << current_diagnostic(true) << ENDM;
    throw;
  }
}

std::string AggregatorRunner::getDetectorName(std::vector<std::shared_ptr<Aggregator>> aggregators)
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
