///
/// \file   Publisher_test.cpp
/// \author Barthelemy von Haller
///

#include "../libs/Core/Quality.h"

#define BOOST_TEST_MODULE Quality test
#define BOOST_TEST_MAIN
#define BOOST_TEST_DYN_LINK
#include <boost/test/unit_test.hpp>
#include <assert.h>
#include <iostream>

using namespace std;

namespace AliceO2 {
namespace QualityControl {
namespace Core {

BOOST_AUTO_TEST_CASE(quality_test)
{
  Quality asdf(123, "asdf");
  Quality myQuality = Quality::Bad;
  BOOST_CHECK_EQUAL(myQuality.getLevel(), 3);
  BOOST_CHECK_EQUAL(myQuality.getName(), "Bad");
  myQuality = Quality::Good;
  BOOST_CHECK_EQUAL(myQuality.getLevel(), 1);
  BOOST_CHECK_EQUAL(myQuality.getName(), "Good");
  myQuality = Quality::Medium;
  BOOST_CHECK_EQUAL(myQuality.getLevel(), 2);
  BOOST_CHECK_EQUAL(myQuality.getName(), "Medium");
  myQuality = Quality::Null;
  BOOST_CHECK_EQUAL(myQuality.getLevel(), 0);
  BOOST_CHECK_EQUAL(myQuality.getName(), "Null");
}

} /* namespace Publisher */
} /* namespace QualityControl */
} /* namespace AliceO2 */
