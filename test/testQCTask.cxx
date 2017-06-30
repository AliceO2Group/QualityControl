///
/// \file    TestQCTask.cxx
/// \author  Barthelemy von Haller
///

#include "../include/QualityControl/TaskInterface.h"
#include "../include/QualityControl/TaskFactory.h"

#define BOOST_TEST_MODULE QC test
#define BOOST_TEST_MAIN
#define BOOST_TEST_DYN_LINK

#include <boost/test/unit_test.hpp>
#include <cassert>
#include <iostream>
#include <boost/test/output_test_stream.hpp>

using boost::test_tools::output_test_stream;

using namespace AliceO2::QualityControl;
using namespace std;

namespace AliceO2 {
namespace QualityControl {

using namespace Core;

namespace Test {
class TestTask : public TaskInterface
{
  public:
    TestTask(ObjectsManager *objectsManager) : TaskInterface(objectsManager)
    {
      test = 0;
    }

    ~TestTask() override
    {

    }

    // Definition of the methods for the template method pattern
    void initialize() override
    {
      cout << "initialize" << endl;
      test = 1;
    }

    void startOfActivity(Activity &activity) override
    {
      cout << "startOfActivity" << endl;
      test = 2;
    }

    void startOfCycle() override
    {
      cout << "startOfCycle" << endl;
    }

    virtual void monitorDataBlock(DataBlock &block)
    {
      cout << "monitorDataBlock" << endl;
    }

    void endOfCycle() override
    {
      cout << "endOfCycle" << endl;
    }

    void endOfActivity(Activity &activity) override
    {
      cout << "endOfActivity" << endl;
    }

    void reset() override
    {
      cout << "reset" << endl;
    }

    int test;
};

} /* namespace Test */
} /* namespace QualityControl */
} /* namespace AliceO2 */

BOOST_AUTO_TEST_CASE(TestInstantiate)
{
  TaskConfig config;
  config.publisherClassName="MockPublisher";
  config.taskName="my task name";
  ObjectsManager objectsManager(config);
//  Test::TestTask tt(&objectsManager);
//  BOOST_CHECK_EQUAL(tt.test, 0);
//  tt.initialize();
//  BOOST_CHECK_EQUAL(tt.test, 1);
//  Activity act;
//  tt.startOfActivity(act);
//  BOOST_CHECK_EQUAL(tt.test, 2);
}

