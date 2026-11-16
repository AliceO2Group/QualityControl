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
/// \file    testPostProcessingInterface.cxx
/// \author  Piotr Konopka
///

#include "getTestDataDirectory.h"
#include "QualityControl/PostProcessingInterface.h"
#include "QualityControl/PostProcessingFactory.h"
#include <Configuration/ConfigurationFactory.h>
#include <Framework/ServiceRegistry.h>
#include <catch_amalgamated.hpp>

using namespace o2::quality_control::postprocessing;
namespace o2::quality_control_modules::test
{

// anonymous namespace to keep this class symbols visible only to this file (instead of all files combined into o2-qc-test-core)
namespace
{
class TestTask : public PostProcessingInterface
{
 public:
  TestTask() : test(0){};
  ~TestTask() override = default;

  void configure(const boost::property_tree::ptree&) override
  {
    test = 1;
  }

  // user gets to know what triggered the init
  void initialize(quality_control::postprocessing::Trigger, framework::ServiceRegistryRef) override
  {
    test = 2;
  }
  // user gets to know what triggered the processing
  void update(quality_control::postprocessing::Trigger, framework::ServiceRegistryRef) override
  {
    test = 3;
  }
  // user gets to know what triggered the end
  void finalize(quality_control::postprocessing::Trigger, framework::ServiceRegistryRef) override
  {
    test = 4;
  }

  int test;
};

} // namespace
} /* namespace o2::quality_control_modules::test */

using namespace o2::configuration;

TEST_CASE("PostProcessingInterface_test_factory")
{
  std::string configFilePath = std::string("json://") + getTestDataDirectory() + "testSharedConfig.json";
  auto configFile = ConfigurationFactory::getConfiguration(configFilePath);

  o2::quality_control_modules::test::TestTask task;
  CHECK(task.test == 0);

  task.setName("asfd");
  CHECK(task.getName() == "asfd");

  task.configure(configFile->getRecursive());
  CHECK(task.test == 1);

  o2::framework::ServiceRegistry services;

  task.initialize({ TriggerType::No }, services);
  CHECK(task.test == 2);
  task.update({ TriggerType::No }, services);
  CHECK(task.test == 3);
  task.finalize({ TriggerType::No }, services);
  CHECK(task.test == 4);
}