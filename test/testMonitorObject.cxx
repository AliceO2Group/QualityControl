///
/// \file   Publisher_test.cpp
/// \author Barthelemy von Haller
///

#include "../include/QualityControl/MonitorObject.h"

#define BOOST_TEST_MODULE MO test
#define BOOST_TEST_MAIN
#define BOOST_TEST_DYN_LINK
#include <boost/test/unit_test.hpp>
#include <assert.h>

namespace AliceO2 {
namespace QualityControl {
namespace Core {

BOOST_AUTO_TEST_CASE(mo)
{
  AliceO2::QualityControl::Core::MonitorObject obj;
  BOOST_CHECK_EQUAL(obj.getName(), "");
  obj.setName("test");
  BOOST_CHECK_EQUAL(obj.getName(), "test");
  obj.setQuality(Quality::Medium);
  BOOST_CHECK_EQUAL(obj.getQuality(), Quality::Medium);

  obj.addCheck("first", "class1");
  obj.addCheck("second", "class1");
  obj.addCheck("third", "class2");
  obj.addCheck("first", "class2");
  auto checkers1 = obj.getChecks();
  BOOST_CHECK_EQUAL(checkers1["first"], "class2");
  BOOST_CHECK_EQUAL(checkers1["second"], "class1");
}

} /* namespace ObjectsManager */
} /* namespace QualityControl */
} /* namespace AliceO2 */
