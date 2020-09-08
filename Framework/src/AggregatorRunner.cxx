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
// QC
#include "QualityControl/DatabaseFactory.h"
#include "QualityControl/QcInfoLogger.h"

using namespace AliceO2::Common;
using namespace AliceO2::InfoLogger;
using namespace o2::framework;
using namespace o2::configuration;
using namespace o2::quality_control::core;
using namespace o2::quality_control::repository;
using namespace std;

const auto current_diagnostic = boost::current_exception_diagnostic_information;

namespace o2::quality_control::checker
{

// TODO duplicated from CheckRunner
std::size_t AggregatorRunner::hash(std::string inputString)
{
  // BSD aggregatorsum
  const int mode = 16;
  std::size_t aggregatorsum = 0;

  const std::size_t mask = (1 << (mode + 1)) - 1;
  for (char c : inputString) {
    // Rotate the sum
    aggregatorsum = (aggregatorsum >> 1) + ((aggregatorsum & 1) << (mode - 1));
    aggregatorsum = (aggregatorsum + (std::size_t)c) & mask;
  }
  return aggregatorsum;
}

AggregatorRunner::AggregatorRunner(std::string configurationSource, const vector<framework::OutputSpec> checkerRunnerOutputs)
  : mDeviceName(createAggregatorRunnerName()),
    mOutput({ "mo" }, createAggregatorRunnerDataDescription(mDeviceName), 0)
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
  for(const auto& spec: checkerRunnerOutputs) {
    auto input = DataSpecUtils::matchingInput(spec);
    input.binding = "checkerOutput" + to_string(i); // TODO check if that name is fine
    mInputs.emplace_back(input);
  }
}

AggregatorRunner::~AggregatorRunner()
{
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

void AggregatorRunner::init(framework::InitContext&)
{
  try {
    initDatabase();
  } catch (...) {
    // catch the exceptions and print it (the ultimate caller might not know how to display it)
    ILOG(Fatal) << "Unexpected exception during initialization:\n"
                << current_diagnostic(true) << ENDM;
    throw;
  }
}

void AggregatorRunner::run(framework::ProcessingContext& ctx)
{
  auto qualityObjects = aggregate();

  store(qualityObjects);

  send(qualityObjects, ctx.outputs());
}


QualityObjectsType AggregatorRunner::aggregate()
{
//  ILOG(Info) << "Trying " << mAggregators.size() << " aggregators for " << moMap.size() << " monitor objects"
//          << ENDM;

  QualityObjectsType allQOs;
//  for (auto& aggregator : mAggregators) {
//    if (aggregator.isReady(mMonitorObjectRevision)) {
//      auto newQOs = aggregator.aggregator(moMap);
//      mTotalNumberAggregatorExecuted += newQOs.size();
//
//      allQOs.insert(allQOs.end(), std::make_move_iterator(newQOs.begin()), std::make_move_iterator(newQOs.end()));
//      newQOs.clear();
//
//      // Was aggregatored, update latest revision
//      aggregator.updateRevision(mGlobalRevision);
//    } else {
//      ILOG(Info) << "Monitor Objects for the aggregator '" << aggregator.getName() << "' are not ready, ignoring" << ENDM;
//    }
//  }
  return allQOs;
}

void AggregatorRunner::store(QualityObjectsType& qualityObjects)
{
  ILOG(Info) << "Storing " << qualityObjects.size() << " QualityObjects" << ENDM;
  try {
    for (auto& qo : qualityObjects) {
      mDatabase->storeQO(qo);
//      mTotalNumberQOStored++;
    }
  } catch (boost::exception& e) {
    ILOG(Info) << "Unable to " << diagnostic_information(e) << ENDM;
  }
}

void AggregatorRunner::send(QualityObjectsType& qualityObjects, framework::DataAllocator& allocator)
{
  // Note that we might send multiple QOs in one output, as separate parts.
  // This should be fine if they are retrieved on the other side with InputRecordWalker.

  ILOG(Info) << "Sending " << qualityObjects.size() << " quality objects" << ENDM;
  for (const auto& qo : qualityObjects) {

//    const auto& correspondingAggregator = std::find_if(mAggregators.begin(), mAggregators.end(), [aggregatorName = qo->getAggregatorName()](const auto& aggregator) {
//      return aggregator.getName() == aggregatorName;
//    });

//    auto outputSpec = correspondingAggregator->getOutputSpec();
//    auto concreteOutput = framework::DataSpecUtils::asConcreteDataMatcher(outputSpec);
//    allocator.snapshot(
//      framework::Output{ concreteOutput.origin, concreteOutput.description, concreteOutput.subSpec, outputSpec.lifetime }, *qo);
  }
}

void AggregatorRunner::initDatabase()
{
  mDatabase = DatabaseFactory::create(mConfigFile->get<std::string>("qc.config.database.implementation"));
  mDatabase->connect(mConfigFile->getRecursiveMap("qc.config.database"));
  LOG(INFO) << "Database that is going to be used : ";
  LOG(INFO) << ">> Implementation : " << mConfigFile->get<std::string>("qc.config.database.implementation");
  LOG(INFO) << ">> Host : " << mConfigFile->get<std::string>("qc.config.database.host");
}
std::string AggregatorRunner::createAggregatorRunnerName()
{
  return "QUALITY-AGGREGATOR";
}

} // namespace o2::quality_control::aggregatorer
