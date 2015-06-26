///
/// \file    TestQCTask.cxx
/// \author  Barthelemy von Haller
///

#include "../libs/Core/QCTask.h"

#define BOOST_TEST_MODULE QC test
#define BOOST_TEST_MAIN
#define BOOST_TEST_DYN_LINK
#include <boost/test/unit_test.hpp>
#include <assert.h>
#include <iostream>
#include <boost/test/output_test_stream.hpp>
using boost::test_tools::output_test_stream;

using namespace AliceO2::QualityControl;
using namespace std;

namespace AliceO2 {
namespace QualityControl {

using namespace Core;

namespace Test {
class TestTask: public QCTask
{
  public:
    TestTask()
    {
test = 0;
    }
    virtual ~TestTask()
    {

    }

    // Definition of the methods for the template method pattern
    virtual void initialize()
    {
      cout << "initialize" << endl;
      test= 1;
    }
    virtual void startOfActivity(Activity &activity)
    {
      cout << "startOfActivity" << endl;
      test = 2;
    }
    virtual void startOfCycle()
    {
      cout << "startOfCycle" << endl;
    }
    virtual void monitorDataBlock(DataBlock &block)
    {
      cout << "monitorDataBlock" << endl;
    }
    virtual void endOfCycle()
    {
      cout << "endOfCycle" << endl;
    }
    virtual void endOfActivity(Activity &activity)
    {
      cout << "endOfActivity" << endl;
    }
    virtual void Reset()
    {
      cout << "Reset" << endl;
    }
    int test;
};

} /* namespace Test */
} /* namespace QualityControl */
} /* namespace AliceO2 */

BOOST_AUTO_TEST_CASE(TestInstantiate)
{
  Test::TestTask tt;
  BOOST_CHECK_EQUAL(tt.test, 0);
  tt.initialize();
  BOOST_CHECK_EQUAL(tt.test, 1);
  Activity act;
  tt.startOfActivity(act);
  BOOST_CHECK_EQUAL(tt.test, 2);
}

