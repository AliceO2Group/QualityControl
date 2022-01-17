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
/// \file    testAggregatorRunner.cxx
/// \author  Barthelemy von Haller
///

#include "getTestDataDirectory.h"
#include "QualityControl/AggregatorRunnerFactory.h"
#include "QualityControl/AggregatorRunner.h"
#include "QualityControl/AggregatorRunnerConfig.h"
#include "QualityControl/AggregatorConfig.h"
#include "QualityControl/Aggregator.h"
#include "QualityControl/MonitorObject.h"
#include "QualityControl/InfrastructureSpecReader.h"
#include <Configuration/ConfigurationFactory.h>
#include <Framework/InitContext.h>
#include <Framework/ConfigParamRegistry.h>
#include <Framework/ConfigParamStore.h>

#define BOOST_TEST_MODULE CheckRunner test
#define BOOST_TEST_MAIN
#define BOOST_TEST_DYN_LINK

#include <boost/test/unit_test.hpp>

using namespace o2::quality_control::checker;
using namespace std;
using namespace o2::framework;
using namespace o2::configuration;
using namespace o2::header;
using namespace o2::quality_control::core;

std::pair<AggregatorRunnerConfig, std::vector<AggregatorConfig>> getAggregatorConfigs(const std::string& configFilePath)
{
  auto config = ConfigurationFactory::getConfiguration(configFilePath);
  auto infrastructureSpec = InfrastructureSpecReader::readInfrastructureSpec(config->getRecursive());
  std::vector<AggregatorConfig> aggregatorConfigs;
  for (const auto& aggregatorSpec : infrastructureSpec.aggregators) {
    if (aggregatorSpec.active) {
      aggregatorConfigs.emplace_back(Aggregator::extractConfig(infrastructureSpec.common, aggregatorSpec));
    }
  }
  auto aggregatorRunnerConfig = AggregatorRunnerFactory::extractConfig(infrastructureSpec.common);

  return { aggregatorRunnerConfig, aggregatorConfigs };
}

BOOST_AUTO_TEST_CASE(test_aggregator_runner_static)
{
  BOOST_CHECK(AggregatorRunner::createAggregatorRunnerDataDescription("qwertyuiop") == DataDescription("qwertyuiop"));
  BOOST_CHECK(AggregatorRunner::createAggregatorRunnerDataDescription("012345678901234567890") == DataDescription("0123456789012345"));
  BOOST_CHECK_THROW(AggregatorRunner::createAggregatorRunnerDataDescription(""), AliceO2::Common::FatalException);
}

BOOST_AUTO_TEST_CASE(test_aggregator_runner)
{
  std::string configFilePath = std::string("json://") + getTestDataDirectory() + "testSharedConfig.json";
  auto [aggregatorRunnerConfig, aggregatorConfigs] = getAggregatorConfigs(configFilePath);
  auto config = ConfigurationFactory::getConfiguration(configFilePath);
  auto infrastructureSpec = InfrastructureSpecReader::readInfrastructureSpec(config->getRecursive());
  AggregatorRunner aggregatorRunner{ aggregatorRunnerConfig, infrastructureSpec };

  Options options{
    { "runNumber", VariantType::String, { "Run number" } },
    { "qcConfiguration", VariantType::Dict, emptyDict(), { "Some dictionary configuration" } }
  };
  std::vector<std::unique_ptr<ParamRetriever>> retr;
  std::unique_ptr<ConfigParamStore> store = make_unique<ConfigParamStore>(move(options), move(retr));
  ConfigParamRegistry cfReg(std::move(store));
  ServiceRegistry sReg;
  InitContext initContext{ cfReg, sReg };
  aggregatorRunner.init(initContext);

  BOOST_CHECK_EQUAL(aggregatorRunner.getDeviceName(), "QC-AGGREGATOR-RUNNER");

  // check the reordering
  const std::vector<std::shared_ptr<Aggregator>> aggregators = aggregatorRunner.getAggregators();
  BOOST_CHECK(aggregators.at(0)->getName() == "MyAggregatorC" || aggregators.at(0)->getName() == "MyAggregatorB");
  BOOST_CHECK(aggregators.at(1)->getName() == "MyAggregatorC" || aggregators.at(1)->getName() == "MyAggregatorB");
  BOOST_CHECK(aggregators.at(2)->getName() == "MyAggregatorA");
  BOOST_CHECK(aggregators.at(3)->getName() == "MyAggregatorD");
}

Quality getQualityForCheck(QualityObjectsType qos, string checkName)
{
  auto it = find_if(qos.begin(), qos.end(), [&checkName](const shared_ptr<QualityObject> obj) { return obj->getCheckName() == checkName; });
  if (it != qos.end()) {
    return (*it)->getQuality();
  } else {
    return Quality::Null;
  }
}

BOOST_AUTO_TEST_CASE(test_aggregator_quality_filter)
{
  std::string configFilePath = std::string("json://") + getTestDataDirectory() + "testSharedConfig.json";
  auto [aggregatorRunnerConfig, aggregatorConfigs] = getAggregatorConfigs(configFilePath);
  auto myAggregatorBConfig = std::find_if(aggregatorConfigs.begin(), aggregatorConfigs.end(), [](const auto& cfg) { return cfg.name == "MyAggregatorB"; });
  BOOST_REQUIRE(myAggregatorBConfig != aggregatorConfigs.end());
  auto aggregator = make_shared<Aggregator>(*myAggregatorBConfig);
  aggregator->init();

  // empty list -> Good
  QualityObjectsMapType qoMap;
  QualityObjectsType result = aggregator->aggregate(qoMap);
  BOOST_CHECK_EQUAL(getQualityForCheck(result, "MyAggregatorB/newQuality"), Quality::Good);

  // Add dataSizeCheck1/q1=good and dataSizeCheck1/q2=medium -> return medium
  qoMap["dataSizeCheck1/q1"] = make_shared<QualityObject>(Quality::Good, "dataSizeCheck1/q1");
  qoMap["dataSizeCheck1/q2"] = make_shared<QualityObject>(Quality::Medium, "dataSizeCheck1/q2");
  result = aggregator->aggregate(qoMap);
  BOOST_CHECK_EQUAL(getQualityForCheck(result, "MyAggregatorB/newQuality"), Quality::Medium);

  // Add whatever/q1=bad -> return medium because it is filtered out (not in config file)
  qoMap["whatever/q1"] = make_shared<QualityObject>(Quality::Bad, "whatever/q1");
  result = aggregator->aggregate(qoMap);
  BOOST_CHECK_EQUAL(getQualityForCheck(result, "MyAggregatorB/newQuality"), Quality::Medium);

  // Add someNumbersCheck/example=bad return bad
  qoMap["dataSizeCheck2/someNumbersTask/example"] = make_shared<QualityObject>(Quality::Bad, "dataSizeCheck2/someNumbersTask/example");
  result = aggregator->aggregate(qoMap);
  BOOST_CHECK_EQUAL(getQualityForCheck(result, "MyAggregatorB/newQuality"), Quality::Bad);

  // reset and add dataSizeCheck/q1=good and dataSizeCheck/q2=medium and someNumbersCheck/example=medium and someNumbersCheck/whatever=bad
  // Return medium because someNumbersTask/whatever is filtered out
  qoMap.clear();
  qoMap["dataSizeCheck1/q1"] = make_shared<QualityObject>(Quality::Good, "dataSizeCheck1/q1");
  qoMap["dataSizeCheck1/q2"] = make_shared<QualityObject>(Quality::Medium, "dataSizeCheck1/q2");
  qoMap["dataSizeCheck2/someNumbersTask/example"] = make_shared<QualityObject>(Quality::Medium, "dataSizeCheck2/someNumbersTask/example");
  qoMap["dataSizeCheck2/someNumbersTask/example2"] = make_shared<QualityObject>(Quality::Bad, "dataSizeCheck2/someNumbersTask/example2");
  result = aggregator->aggregate(qoMap);
  BOOST_CHECK_EQUAL(getQualityForCheck(result, "MyAggregatorB/newQuality"), Quality::Medium);
}

BOOST_AUTO_TEST_CASE(test_getDetector)
{
  AggregatorConfig config;
  config.detectorName = "TST";

  std::vector<std::shared_ptr<Aggregator>> aggregators;
  BOOST_CHECK_EQUAL(AggregatorRunner::getDetectorName(aggregators), "");
  auto checkTST = std::make_shared<Aggregator>(config);
  aggregators.push_back(checkTST);
  BOOST_CHECK_EQUAL(AggregatorRunner::getDetectorName(aggregators), "TST");
  auto checkTST2 = std::make_shared<Aggregator>(config);
  aggregators.push_back(checkTST2);
  BOOST_CHECK_EQUAL(AggregatorRunner::getDetectorName(aggregators), "TST");
  config.detectorName = "EMC";
  auto checkEMC = std::make_shared<Aggregator>(config);
  aggregators.push_back(checkEMC);
  BOOST_CHECK_EQUAL(AggregatorRunner::getDetectorName(aggregators), "MANY");
}