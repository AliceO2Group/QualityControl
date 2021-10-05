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
#include <Configuration/ConfigurationFactory.h>
#include <Framework/DataSpecUtils.h>
#include <Monitoring/MonitoringFactory.h>
#include <Monitoring/Monitoring.h>
#include <Framework/InputRecordWalker.h>

#include <utility>
// QC
#include "QualityControl/DatabaseFactory.h"
#include "QualityControl/QcInfoLogger.h"
#include "QualityControl/ServiceDiscovery.h"
#include "QualityControl/Aggregator.h"
#include "QualityControl/runnerUtils.h"

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
    mAggregatorsConfigs(acs),
    mTotalNumberObjectsReceived(0)
{
  // prepare list of all inputs
  // we cannot use the binding of the output because it is empty.
  int i = 0;
  for (const auto& aggConfig : mAggregatorsConfigs) {
    for (auto input : aggConfig.inputSpecs) {
      input.binding = "checkerOutput" + to_string(i++);
      mInputs.emplace_back(input);
    }
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

void AggregatorRunner::init(framework::InitContext& iCtx)
{
  InfoLoggerContext* ilContext = nullptr;
  try {
    ilContext = &iCtx.services().get<AliceO2::InfoLogger::InfoLoggerContext>();
  } catch (const RuntimeErrorRef& err) {
    ILOG(Error) << "Could not find the DPL InfoLogger Context." << ENDM;
  }

  try {
    ILOG_INST.init("aggregator", mRunnerConfig.infologgerFilterDiscardDebug, mRunnerConfig.infologgerDiscardLevel, ilContext);
    initDatabase();
    initMonitoring();
    initServiceDiscovery();
    initAggregators();
  } catch (...) {
    ILOG(Fatal) << "Unexpected exception during initialization:\n"
                << current_diagnostic(true) << ENDM;
    throw;
  }

  try {
    // registering state machine callbacks
    iCtx.services().get<CallbackService>().set(CallbackService::Id::Start, [this, &services = iCtx.services()]() { start(services); });
    iCtx.services().get<CallbackService>().set(CallbackService::Id::Stop, [this]() { stop(); });
    iCtx.services().get<CallbackService>().set(CallbackService::Id::Reset, [this]() { reset(); });
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
  ILOG(Info, Devel) << "Database that is going to be used : ";
  ILOG(Info, Support) << ">> Implementation : " << mRunnerConfig.database.at("implementation") << ENDM;
  ILOG(Info, Support) << ">> Host : " << mRunnerConfig.database.at("host") << ENDM;
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
    ILOG(Warning, Ops) << "Service Discovery disabled" << ENDM;
    return;
  }
  std::string url = ServiceDiscovery::GetDefaultUrl(ServiceDiscovery::DefaultHealthPort + 2); // we try to avoid colliding with the CheckRunner
  mServiceDiscovery = std::make_shared<ServiceDiscovery>(consulUrl, mDeviceName, mDeviceName, url);
  ILOG(Info, Devel) << "ServiceDiscovery initialized";
}

void AggregatorRunner::initAggregators()
{
  ILOG(Info, Devel) << "Initialization of the aggregators" << ENDM;

  // For every aggregator definition, create an Aggregator
  for (const auto& aggregatorConfig : mAggregatorsConfigs) {
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
    mCollector->send({ mTotalNumberObjectsReceived, "qc_objects_received" }, DerivedMetricMode::RATE);
  }
}

void AggregatorRunner::start(const ServiceRegistry& services)
{
  mActivity.mId = computeRunNumber(services, mRunnerConfig.fallbackRunNumber);
  mActivity.mPeriodName = computePeriodName(services, mRunnerConfig.fallbackPeriodName);
  mActivity.mPassName = computePassName(mRunnerConfig.fallbackPassName);
  mActivity.mProvenance = computeProvenance(mRunnerConfig.fallbackProvenance);
  ILOG(Info, Ops) << "Starting run " << mActivity.mId << ":"
                  << "\n   - period: " << mActivity.mPeriodName << "\n   - pass type: " << mActivity.mPassName << "\n   - provenance: " << mActivity.mProvenance << ENDM;
}


void AggregatorRunner::stop()
{
  ILOG(Info, Ops) << "Stopping run " << mActivity.mId << ENDM;
}

void AggregatorRunner::reset()
{
  ILOG(Info, Ops) << "Reset" << ENDM;

  try {
    mCollector.reset();
    mActivity = Activity();
  } catch (...) {
    // we catch here because we don't know where it will go in DPL's CallbackService
    ILOG(Error, Support) << "Error caught in reset() :\n"
                         << current_diagnostic(true) << ENDM;
    throw;
  }
}

} // namespace o2::quality_control::checker
