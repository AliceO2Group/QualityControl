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
/// \file    testTaskInterface.cxx
/// \author  Barthelemy von Haller
///

#include "QualityControl/TaskFactory.h"
#include "QualityControl/TaskInterface.h"
#include "QualityControl/QcInfoLogger.h"
#include <Framework/ConfigParamRegistry.h>
#include "EMCALCalib/BadChannelMap.h"
#include "CCDB/CcdbApi.h"

#if (__has_include(<Framework/ConfigParamStore.h>))
#include <Framework/ConfigParamStore.h>
o2::framework::ConfigParamRegistry createDummyRegistry()
{
  using namespace o2::framework;
  std::vector<ConfigParamSpec> specs;
  std::vector<std::unique_ptr<ParamRetriever>> retrievers;

  auto store = std::make_unique<ConfigParamStore>(specs, std::move(retrievers));
  store->preload();
  store->activate();
  ConfigParamRegistry registry(std::move(store));

  return registry;
}
#else
o2::framework::ConfigParamRegistry createDummyRegistry()
{
  using namespace o2::framework;
  std::unique_ptr<ParamRetriever> retriever;
  ConfigParamRegistry registry(move(retriever));

  return registry;
}
#endif

#define BOOST_TEST_MODULE TaskInterface test
#define BOOST_TEST_MAIN
#define BOOST_TEST_DYN_LINK

#include <Framework/InitContext.h>
#include <Framework/ConfigParamRegistry.h>
#include <Framework/ServiceRegistry.h>
#include <boost/test/unit_test.hpp>

using namespace o2::quality_control;
using namespace std;
using namespace o2::framework;

namespace o2::quality_control
{

using namespace core;

namespace test
{
class TestTask : public TaskInterface
{
 public:
  TestTask(ObjectsManager* objectsManager) : TaskInterface(objectsManager) { test = 0; }

  ~TestTask() override {}

  // Definition of the methods for the template method pattern
  void initialize(o2::framework::InitContext& /*ctx*/) override
  {
    ILOG(Info, Support) << "initialize" << ENDM;
    test = 1;
  }

  void startOfActivity(Activity& /*activity*/) override
  {
    ILOG(Info, Support) << "startOfActivity" << ENDM;
    test = 2;
  }

  void startOfCycle() override
  {
    ILOG(Info, Support) << "startOfCycle" << ENDM;
    test = 3;
  }

  void monitorData(o2::framework::ProcessingContext& /*ctx*/) override
  {
    ILOG(Info, Support) << "monitorData" << ENDM;
    test = 4;
  }

  void endOfCycle() override
  {
    ILOG(Info, Support) << "endOfCycle" << ENDM;
    test = 5;
  }

  void endOfActivity(Activity& /*activity*/) override
  {
    ILOG(Info, Support) << "endOfActivity" << ENDM;
    test = 6;
  }

  void reset() override
  {
    ILOG(Info, Support) << "reset" << ENDM;
    test = 7;
  }

  o2::emcal::BadChannelMap* testRetrieveCondition()
  {
    ILOG(Info, Support) << "testRetrieveCondition" << ENDM;
    test = 8;

    return retrieveConditionAny<o2::emcal::BadChannelMap>("qc/TST/conditions");
  }

  int test;
};

} /* namespace test */
} /* namespace o2::quality_control */

BOOST_AUTO_TEST_CASE(test_invoke_all_methods)
{
  // This is maximum that we can do until we are able to test the DPL algorithms in isolation.
  TaskConfig taskConfig;
  ObjectsManager* objectsManager = new ObjectsManager(taskConfig.taskName, taskConfig.detectorName, taskConfig.consulUrl, 0, true);

  test::TestTask testTask(objectsManager);
  BOOST_CHECK_EQUAL(testTask.test, 0);

  auto options = createDummyRegistry();
  ServiceRegistry services;
  InitContext ctx(options, services);
  testTask.initialize(ctx);
  BOOST_CHECK_EQUAL(testTask.test, 1);

  Activity act;
  testTask.startOfActivity(act);
  BOOST_CHECK_EQUAL(testTask.test, 2);

  testTask.startOfCycle();
  BOOST_CHECK_EQUAL(testTask.test, 3);

  // creating a valid ProcessingContext is almost impossible outside of the framework
  // testTask.monitorData(pctx);
  // BOOST_CHECK_EQUAL(testTask.test, 4);

  testTask.endOfCycle();
  BOOST_CHECK_EQUAL(testTask.test, 5);

  testTask.endOfActivity(act);
  BOOST_CHECK_EQUAL(testTask.test, 6);

  testTask.reset();
  BOOST_CHECK_EQUAL(testTask.test, 7);
}

BOOST_AUTO_TEST_CASE(test_task_factory)
{
  TaskConfig config{
    "skeletonTask",
    "QcSkeleton",
    "o2::quality_control_modules::skeleton::SkeletonTask",
    10,
    -1,
    "http://consul-test.cern.ch:8500"
  };

  auto objectsManager = make_shared<ObjectsManager>(config.taskName, config.detectorName, config.consulUrl);

  TaskFactory taskFactory;
  auto task = taskFactory.create(config, objectsManager);

  BOOST_REQUIRE(task != nullptr);
  delete task;
}

BOOST_AUTO_TEST_CASE(retrieveCondition)
{
  // first store a condition
  o2::emcal::BadChannelMap bad;
  bad.addBadChannel(1, o2::emcal::BadChannelMap::MaskType_t::GOOD_CELL);
  bad.addBadChannel(2, o2::emcal::BadChannelMap::MaskType_t::BAD_CELL);
  bad.addBadChannel(3, o2::emcal::BadChannelMap::MaskType_t::DEAD_CELL);
  std::map<std::string, std::string> meta;
  o2::ccdb::CcdbApi api;
  api.init("ccdb-test.cern.ch:8080");
  api.storeAsTFileAny<o2::emcal::BadChannelMap>(&bad, "qc/TST/conditions", meta);

  // retrieve it
  TaskConfig taskConfig;
  ObjectsManager* objectsManager = new ObjectsManager(taskConfig.taskName, taskConfig.detectorName, taskConfig.consulUrl, 0, true);
  test::TestTask testTask(objectsManager);
  testTask.loadCcdb("ccdb-test.cern.ch:8080");
  o2::emcal::BadChannelMap* bcm = testTask.testRetrieveCondition();
  BOOST_CHECK_EQUAL(bcm->getChannelStatus(1), o2::emcal::BadChannelMap::MaskType_t::GOOD_CELL);
  BOOST_CHECK_EQUAL(bcm->getChannelStatus(3), o2::emcal::BadChannelMap::MaskType_t::DEAD_CELL);
}
