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
/// \file    testTaskInterface.cxx
/// \author  Barthelemy von Haller
///

#include "QualityControl/TaskFactory.h"
#include "QualityControl/TaskInterface.h"

#define BOOST_TEST_MODULE TaskInterface test
#define BOOST_TEST_MAIN
#define BOOST_TEST_DYN_LINK

#include <Framework/InitContext.h>
#include <boost/test/output_test_stream.hpp>
#include <boost/test/unit_test.hpp>
#include <iostream>

using boost::test_tools::output_test_stream;

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
    cout << "initialize" << endl;
    test = 1;
  }

  void startOfActivity(Activity& /*activity*/) override
  {
    cout << "startOfActivity" << endl;
    test = 2;
  }

  void startOfCycle() override
  {
    cout << "startOfCycle" << endl;
    test = 3;
  }

  void monitorData(o2::framework::ProcessingContext& /*ctx*/) override
  {
    cout << "monitorData" << endl;
    test = 4;
  }

  void endOfCycle() override
  {
    cout << "endOfCycle" << endl;
    test = 5;
  }

  void endOfActivity(Activity& /*activity*/) override
  {
    cout << "endOfActivity" << endl;
    test = 6;
  }

  void reset() override
  {
    cout << "reset" << endl;
    test = 7;
  }

  int test;
};

} /* namespace test */
} /* namespace o2::quality_control */

BOOST_AUTO_TEST_CASE(test_invoke_all_methods)
{
  // This is maximum that we can do until we are able to test the DPL algorithms in isolation.
  TaskConfig taskConfig;
  ObjectsManager* objectsManager = new ObjectsManager(taskConfig);
  test::TestTask testTask(objectsManager);
  BOOST_CHECK_EQUAL(testTask.test, 0);

  std::unique_ptr<ParamRetriever> retriever;
  ConfigParamRegistry options(move(retriever));
  ServiceRegistry services;
  InitContext ctx(options, services);
  testTask.initialize(ctx);
  BOOST_CHECK_EQUAL(testTask.test, 1);

  Activity act;
  testTask.startOfActivity(act);
  BOOST_CHECK_EQUAL(testTask.test, 2);

  testTask.startOfCycle();
  BOOST_CHECK_EQUAL(testTask.test, 3);

  // creating a valid ProcessingContex is almost impossible outside of the framework
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

  auto objectsManager = make_shared<ObjectsManager>(config);

  TaskFactory taskFactory;
  auto task = taskFactory.create(config, objectsManager);

  BOOST_REQUIRE(task != nullptr);
  delete task;
}