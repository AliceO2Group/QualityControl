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
#include <catch_amalgamated.hpp>

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
  auto infrastructureSpec = InfrastructureSpecReader::readInfrastructureSpec(config->getRecursive(), WorkflowType::Standalone);

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

TEST_CASE("test_check_specs")
{
  std::string configFilePath = std::string("json://") + getTestDataDirectory() + "testSharedConfig.json";

  Check check(getCheckConfig(configFilePath, "singleCheck"));

  REQUIRE(check.getInputs().size() == 1);
  CHECK(check.getInputs()[0] == (InputSpec{ { "mo" }, "QTST", "skeletonTask", 0, Lifetime::Sporadic }));

  CHECK(check.getOutputSpec() == (OutputSpec{ "CTST", "singleCheck", 0, Lifetime::Sporadic }));
}

TEST_CASE("test_check_long_description")
{
  std::string configFilePath = std::string("json://") + getTestDataDirectory() + "testSharedConfig.json";

  Check check(getCheckConfig(configFilePath, "singleCheckLongDescription"));

  REQUIRE(check.getInputs().size() == 1);
  CHECK(check.getInputs()[0] == (InputSpec{ { "mo" }, "QTST", "skeletonTask", 0, Lifetime::Sporadic }));

  CHECK(check.getOutputSpec() == (OutputSpec{ "CTST", "singleCheckL9fdb", 0, Lifetime::Sporadic }));
}

std::shared_ptr<MonitorObject> dummyMO(const std::string& objName)
{
  auto obj = std::make_shared<MonitorObject>(new TH1F(objName.c_str(), objName.c_str(), 100, 0, 10), "test", "test", "TST");
  obj->setIsOwner(true);
  return obj;
}

TEST_CASE("test_check_empty_mo")
{
  std::string configFilePath = std::string("json://") + getTestDataDirectory() + "testSharedConfig.json";

  Check check(getCheckConfig(configFilePath, "singleCheck"));
  check.init();
  check.startOfActivity(Activity());

  {
    std::map<std::string, std::shared_ptr<MonitorObject>> moMap{
      { "skeletonTask/example", nullptr }
    };

    auto qos = check.check(moMap);
    CHECK(qos.size() == 0);
  }

  {
    std::map<std::string, std::shared_ptr<MonitorObject>> moMap{
      { "skeletonTask/example", std::make_shared<MonitorObject>() }
    };

    auto qos = check.check(moMap);
    CHECK(qos.size() == 0);
  }
}

TEST_CASE("test_check_invoke_check")
{
  std::string configFilePath = std::string("json://") + getTestDataDirectory() + "testSharedConfig.json";

  Check check(getCheckConfig(configFilePath, "singleCheck"));
  check.init();
  check.startOfActivity(Activity());

  std::map<std::string, std::shared_ptr<MonitorObject>> moMap{
    { "skeletonTask/example", dummyMO("example") }
  };

  auto qos = check.check(moMap);
  CHECK(qos.size() == 1);
}

TEST_CASE("test_check_postprocessing")
{
  std::string configFilePath = std::string("json://") + getTestDataDirectory() + "testSharedConfig.json";

  Check check(getCheckConfig(configFilePath, "checkAnyPP"));
  check.init();
  check.startOfActivity(Activity());

  std::map<std::string, std::shared_ptr<MonitorObject>> moMap{
    { "SkeletonPostProcessing/example", dummyMO("example") }
  };

  auto qos = check.check(moMap);
  CHECK(qos.size() == 1);
}

TEST_CASE("test_check_activity")
{
  Check check({ "test",
                "QcSkeleton",
                "o2::quality_control_modules::skeleton::SkeletonCheck",
                "TST",
                "",
                {},
                "something",
                { { "implementation", "CCDB" }, { "host", "something" } },
                {},
                UpdatePolicyType::OnAny,
                {},
                true });

  std::map<std::string, std::shared_ptr<MonitorObject>> moMap{
    { "abcTask/test1", dummyMO("test1") },
    { "abcTask/test2", dummyMO("test2") }
  };

  moMap["abcTask/test1"]->setActivity({ 300000, "PHYSICS", "LHC22a", "spass", "qc", { 1, 10 }, "pp" });
  moMap["abcTask/test2"]->setActivity({ 300000, "PHYSICS", "LHC22a", "spass", "qc", { 5, 15 }, "pp" });

  check.init();
  check.startOfActivity(Activity());
  auto qos = check.check(moMap);

  REQUIRE(qos.size() == 1);
  ValidityInterval correctValidity{ 1, 15 };
  CHECK(qos[0]->getActivity().mValidity == correctValidity);
}