///
/// \file   Publisher_test.cpp
/// \author Barthelemy von Haller
///

#include "../include/QualityControl/MonitorObject.h"

#define BOOST_TEST_MODULE MO test
#define BOOST_TEST_MAIN
#define BOOST_TEST_DYN_LINK
#include <boost/test/unit_test.hpp>
#include <cassert>

namespace o2 {
namespace quality_control {
namespace core {

BOOST_AUTO_TEST_CASE(mo)
{
  o2::quality_control::core::MonitorObject obj;
  BOOST_CHECK_EQUAL(obj.getName(), "");
  obj.setName("test");
  BOOST_CHECK_EQUAL(obj.getName(), "test");

  obj.addCheck("first", "class1", "lib1");
  obj.addCheck("second", "class1", "lib1");
  obj.addCheck("third", "class2", "lib1");
  obj.addCheck("first", "class2", "lib1");
  auto checkers1 = obj.getChecks();
  BOOST_CHECK_EQUAL(checkers1["first"].name, "first");
  BOOST_CHECK_EQUAL(checkers1["first"].className, "class2");
  BOOST_CHECK_EQUAL(checkers1["first"].libraryName, "lib1");
  BOOST_CHECK_EQUAL(obj.getCheck("second").name, "second");
  BOOST_CHECK_EQUAL(obj.getCheck("second").className, "class1");
  BOOST_CHECK_EQUAL(obj.getCheck("second").libraryName, "lib1");

  BOOST_CHECK_EQUAL(obj.getQuality(), Quality::Null);
  obj.setQualityForCheck("first", Quality::Good);
  BOOST_CHECK_EQUAL(obj.getQuality(), Quality::Good);
  obj.setQualityForCheck("second", Quality::Bad);
  BOOST_CHECK_EQUAL(obj.getQuality(), Quality::Bad);
  obj.setQualityForCheck("second", Quality::Medium);
  BOOST_CHECK_EQUAL(obj.getQuality(), Quality::Medium);

}
BOOST_AUTO_TEST_CASE(mo_check)
{
  o2::quality_control::core::MonitorObject obj;
  BOOST_CHECK_EQUAL(obj.getQuality(), Quality::Null);
  CheckDefinition check;
  check.name = "test";
  check.libraryName = "test";
  check.className = "test";
  obj.addOrReplaceCheck("test", check);
  BOOST_CHECK_EQUAL(obj.getQuality(), Quality::Null);

}


} /* namespace ObjectsManager */
} /* namespace quality_control */
} /* namespace o2 */
