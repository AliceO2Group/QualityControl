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

#include "QualityControl/CheckInterface.h"
#include "QualityControl/CheckRunnerFactory.h"
#include "QualityControl/CommonSpec.h"
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

  auto checkSpec = std::find_if(infrastructureSpec.checks.begin(), infrastructureSpec.checks.end(),
                                [&checkName](const auto& checkSpec) {
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
  BOOST_CHECK_EQUAL(check.getInputs()[0], (InputSpec{ { "mo" }, "QTST", "skeletonTask", 0, Lifetime::Sporadic }));

  BOOST_CHECK_EQUAL(check.getOutputSpec(), (OutputSpec{ "QC", "singleCheck-chk", 0, Lifetime::Sporadic }));
}

std::shared_ptr<MonitorObject> dummyMO(const std::string& objName)
{
  auto obj = std::make_shared<MonitorObject>(new TH1F(objName.c_str(), objName.c_str(), 100, 0, 10), "test", "test", "TST");
  obj->setIsOwner(true);
  return obj;
}

BOOST_AUTO_TEST_CASE(test_check_empty_mo)
{
  std::string configFilePath = std::string("json://") + getTestDataDirectory() + "testSharedConfig.json";

  Check check(getCheckConfig(configFilePath, "singleCheck"));
  check.init();
  check.setActivity(std::make_shared<Activity>());

  {
    std::map<std::string, std::shared_ptr<MonitorObject>> moMap{
      { "skeletonTask/example", nullptr }
    };

    auto qos = check.check(moMap);
    BOOST_CHECK_EQUAL(qos.size(), 0);
  }

  {
    std::map<std::string, std::shared_ptr<MonitorObject>> moMap{
      { "skeletonTask/example", std::make_shared<MonitorObject>() }
    };

    auto qos = check.check(moMap);
    BOOST_CHECK_EQUAL(qos.size(), 0);
  }
}

BOOST_AUTO_TEST_CASE(test_check_invoke_check)
{
  std::string configFilePath = std::string("json://") + getTestDataDirectory() + "testSharedConfig.json";

  Check check(getCheckConfig(configFilePath, "singleCheck"));
  check.init();
  check.setActivity(std::make_shared<Activity>());

  std::map<std::string, std::shared_ptr<MonitorObject>> moMap{
    { "skeletonTask/example", dummyMO("example") }
  };

  auto qos = check.check(moMap);
  BOOST_CHECK_EQUAL(qos.size(), 1);
}

BOOST_AUTO_TEST_CASE(test_check_postprocessing)
{
  std::string configFilePath = std::string("json://") + getTestDataDirectory() + "testSharedConfig.json";

  Check check(getCheckConfig(configFilePath, "checkAnyPP"));
  check.init();
  check.setActivity(std::make_shared<Activity>());

  std::map<std::string, std::shared_ptr<MonitorObject>> moMap{
    { "SkeletonPostProcessing/example", dummyMO("example") }
  };

  auto qos = check.check(moMap);
  BOOST_CHECK_EQUAL(qos.size(), 1);
}

BOOST_AUTO_TEST_CASE(test_check_activity)
{
  Check check({ "test",
                "QcSkeleton",
                "o2::quality_control_modules::skeleton::SkeletonCheck",
                "TST",
                {},
                UpdatePolicyType::OnAny,
                {},
                true });

  std::map<std::string, std::shared_ptr<MonitorObject>> moMap{
    { "abcTask/test1", dummyMO("test1") },
    { "abcTask/test2", dummyMO("test2") }
  };

  moMap["abcTask/test1"]->setActivity({ 300000, 1, "LHC22a", "spass", "qc", { 1, 10 }, "pp" });
  moMap["abcTask/test2"]->setActivity({ 300000, 1, "LHC22a", "spass", "qc", { 5, 15 }, "pp" });

  check.init();
  check.setActivity(std::make_shared<Activity>());
  auto qos = check.check(moMap);

  BOOST_REQUIRE_EQUAL(qos.size(), 1);
  ValidityInterval correctValidity{ 1, 15 };
  BOOST_CHECK(qos[0]->getActivity().mValidity == correctValidity);
}