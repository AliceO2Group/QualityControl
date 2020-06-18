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
/// \file    testPostProcessingInterface.cxx
/// \author  Piotr Konopka
///

#include "getTestDataDirectory.h"
#include "QualityControl/PostProcessingInterface.h"
#include "QualityControl/PostProcessingFactory.h"
#include <Configuration/ConfigurationFactory.h>

#define BOOST_TEST_MODULE PostProcessingRunner test
#define BOOST_TEST_MAIN
#define BOOST_TEST_DYN_LINK

#include <boost/test/unit_test.hpp>

using namespace o2::quality_control::postprocessing;
namespace o2::quality_control_modules::test
{

class TestTask : public PostProcessingInterface
{
 public:
  TestTask() : test(0){};
  ~TestTask() override = default;

  void configure(std::string, const boost::property_tree::ptree&) override
  {
    test = 1;
  }

  // user gets to know what triggered the init
  void initialize(quality_control::postprocessing::Trigger, framework::ServiceRegistry&) override
  {
    test = 2;
  }
  // user gets to know what triggered the processing
  void update(quality_control::postprocessing::Trigger, framework::ServiceRegistry&) override
  {
    test = 3;
  }
  // user gets to know what triggered the end
  void finalize(quality_control::postprocessing::Trigger, framework::ServiceRegistry&) override
  {
    test = 4;
  }

  int test;
};

} /* namespace o2::quality_control_modules::test */

using namespace o2::configuration;

BOOST_AUTO_TEST_CASE(test_factory)
{
  std::string configFilePath = std::string("json://") + getTestDataDirectory() + "testSharedConfig.json";
  auto configFile = ConfigurationFactory::getConfiguration(configFilePath);

  o2::quality_control_modules::test::TestTask task;
  BOOST_CHECK_EQUAL(task.test, 0);

  task.setName("asfd");
  BOOST_CHECK_EQUAL(task.getName(), "asfd");

  task.configure("", configFile->getRecursive());
  BOOST_CHECK_EQUAL(task.test, 1);

  o2::framework::ServiceRegistry services;

  task.initialize(Trigger::No, services);
  BOOST_CHECK_EQUAL(task.test, 2);
  task.update(Trigger::No, services);
  BOOST_CHECK_EQUAL(task.test, 3);
  task.finalize(Trigger::No, services);
  BOOST_CHECK_EQUAL(task.test, 4);
}