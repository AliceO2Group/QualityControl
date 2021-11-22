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
/// \file    testCheck.cxx
/// \author  Rafal Pacholek
///

#include "QualityControl/CheckRunnerFactory.h"
#include "QualityControl/MonitorObject.h"
#include "QualityControl/InfrastructureSpecReader.h"
#include "getTestDataDirectory.h"
#include <DataSampling/DataSampling.h>
#include <Common/Exceptions.h>
#include <TH1F.h>
#include <Configuration/ConfigurationFactory.h>
#include <Configuration/ConfigurationInterface.h>

#define BOOST_TEST_MODULE Check test
#define BOOST_TEST_MAIN
#define BOOST_TEST_DYN_LINK

#include <boost/test/unit_test.hpp>

using namespace o2::quality_control::checker;
using namespace o2::quality_control::core;
using namespace std;
using namespace o2::framework;
using namespace o2::utilities;
using namespace o2::header;
using namespace o2::configuration;
using namespace AliceO2::Common;

CheckConfig getCheckConfig(const std::string& configFilePath, const std::string& checkName)
{
  auto config = ConfigurationFactory::getConfiguration(configFilePath);
  auto infrastructureSpec = InfrastructureSpecReader::readInfrastructureSpec(config->getRecursive());

  auto checkSpec = std::find_if(infrastructureSpec.checks.begin(), infrastructureSpec.checks.end(), [&checkName](const auto& checkSpec) {
    return checkSpec.checkName == checkName;
  });
  if (checkSpec != infrastructureSpec.checks.end()) {
    return Check::extractConfig(infrastructureSpec.common, *checkSpec);
  } else {
    throw std::runtime_error("check " + checkName + " not found in the config file");
  }
}

BOOST_AUTO_TEST_CASE(test_check_specs)
{
  std::string configFilePath = std::string("json://") + getTestDataDirectory() + "testSharedConfig.json";

  Check check(getCheckConfig(configFilePath, "singleCheck"));

  BOOST_REQUIRE_EQUAL(check.getInputs().size(), 1);
  BOOST_CHECK_EQUAL(check.getInputs()[0], (InputSpec{ { "mo" }, "QC", "skeletonTask-mo", 0, Lifetime::Sporadic }));

  BOOST_CHECK_EQUAL(check.getOutputSpec(), (OutputSpec{ "QC", "singleCheck-chk", 0, Lifetime::Sporadic }));
}

/*
 * Test check and beautify
 */
class TestCheck : public CheckInterface
{
 public:
  TestCheck() {}
  void configure(std::string name)
  {
    (void)name;
  }
  Quality check(std::map<std::string, std::shared_ptr<MonitorObject>>* moMap)
  {
    if (moMap->size()) {
      mCheck = true;
    }
    return Quality();
  }
  void beautify(std::shared_ptr<MonitorObject> mo, Quality checkResult = Quality::Null)
  {
    (void)checkResult;
    if (mo) {
      mBeautify = true;
    }
  }
  std::string getAcceptedType()
  {
    return "any";
  }

  bool mCheck = false;
  bool mBeautify = false;
};

BOOST_AUTO_TEST_CASE(test_check_invoke_check_beautify)
{
  std::string configFilePath = std::string("json://") + getTestDataDirectory() + "testSharedConfig.json";

  Check check(getCheckConfig(configFilePath, "singleCheck"));
  check.init();

  TestCheck testCheck;
  check.setCheckInterface(dynamic_cast<CheckInterface*>(&testCheck));

  std::map<std::string, std::shared_ptr<MonitorObject>> moMap = { { "skeletonTask/example", std::shared_ptr<MonitorObject>(new MonitorObject()) } };

  check.check(moMap);
  // Check should run
  BOOST_CHECK(testCheck.mCheck);
  // Beautify should run - single MO declared
  BOOST_CHECK(testCheck.mBeautify);
}

BOOST_AUTO_TEST_CASE(test_check_dont_invoke_beautify)
{
  std::string configFilePath = std::string("json://") + getTestDataDirectory() + "testSharedConfig.json";

  Check check(getCheckConfig(configFilePath, "checkAny"));
  check.init();

  TestCheck testCheck;
  check.setCheckInterface(dynamic_cast<CheckInterface*>(&testCheck));

  std::map<std::string, std::shared_ptr<MonitorObject>> moMap = { { "abcTask/test1", std::shared_ptr<MonitorObject>() } };

  check.check(moMap);
  // Check should run
  BOOST_CHECK(testCheck.mCheck);
  // Beautify should not run - more than one MO declared
  BOOST_CHECK(!testCheck.mBeautify);
}

BOOST_AUTO_TEST_CASE(test_check_postprocessing)
{
  std::string configFilePath = std::string("json://") + getTestDataDirectory() + "testSharedConfig.json";

  Check check(getCheckConfig(configFilePath, "checkAnyPP"));
  check.init();

  TestCheck testCheck;
  check.setCheckInterface(dynamic_cast<CheckInterface*>(&testCheck));

  std::map<std::string, std::shared_ptr<MonitorObject>> moMap = { { "SkeletonPostProcessing/example", std::shared_ptr<MonitorObject>(new MonitorObject()) } };

  check.check(moMap);
  // Check should run
  BOOST_CHECK(testCheck.mCheck);
  // Beautify should run - single MO declared
  BOOST_CHECK(testCheck.mBeautify);
}
