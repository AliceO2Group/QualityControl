///
/// \file    TestQCTask.cxx
/// \author  Barthelemy von Haller
///

#include "../include/QualityControl/TaskFactory.h"
#include "../include/QualityControl/TaskInterface.h"

#define BOOST_TEST_MODULE QC test
#define BOOST_TEST_MAIN
#define BOOST_TEST_DYN_LINK

#include <boost/test/output_test_stream.hpp>
#include <boost/test/unit_test.hpp>
#include <cassert>
#include <iostream>

//#include "Framework/DataRefUtils.h"
//#include "Framework/AlgorithmSpec.h"
//#include "Framework/ServiceRegistry.h"
//#include "Framework/runDataProcessing.h"
//#include <Monitoring/Monitoring.h>
//#include "FairMQLogger.h"

using boost::test_tools::output_test_stream;

using namespace o2::quality_control;
using namespace std;
using namespace o2::framework;

namespace o2
{
namespace quality_control
{

using namespace core;

namespace Test
{
class TestTask : public TaskInterface
{
 public:
  TestTask(ObjectsManager* objectsManager) : TaskInterface(objectsManager) { test = 0; }

  ~TestTask() override {}

  // Definition of the methods for the template method pattern
  void initialize(o2::framework::InitContext& ctx) override
  {
    cout << "initialize" << endl;
    test = 1;
  }

  void startOfActivity(Activity& activity) override
  {
    cout << "startOfActivity" << endl;
    test = 2;
  }

  void startOfCycle() override { cout << "startOfCycle" << endl; }

  virtual void monitorData(o2::framework::ProcessingContext& ctx) { cout << "monitorData" << endl; }

  void endOfCycle() override { cout << "endOfCycle" << endl; }

  void endOfActivity(Activity& activity) override { cout << "endOfActivity" << endl; }

  void reset() override { cout << "reset" << endl; }

  int test;
};

} /* namespace Test */
} /* namespace quality_control */
} /* namespace o2 */

// BOOST_AUTO_TEST_CASE(TestInstantiate)
//{
////  o2::framework::InitContext;
//  TaskConfig taskConfig;
//  ObjectsManager objectsManager(taskConfig);
//  Test::TestTask tt(&objectsManager);
//  BOOST_CHECK_EQUAL(tt.test, 0);
//
//  // TODO is there a way to test a DPL task ??
//  std::unique_ptr<ParamRetriever> retriever;
//  ConfigParamRegistry options(move(retriever));
//  ServiceRegistry services;
//  InitContext ctx(options, services);
//  tt.initialize(ctx); // tt is the TestTask
//  BOOST_CHECK_EQUAL(tt.test, 1);
//  Activity act;
//  tt.startOfActivity(act);
//  BOOST_CHECK_EQUAL(tt.test, 2);
//}
//}