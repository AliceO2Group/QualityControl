///
/// \file   Publisher_test.cpp
/// \author Barthelemy von Haller
///

#include "../libs/Publisher/Publisher.h"

#define BOOST_TEST_MODULE hello test
#define BOOST_TEST_MAIN
#define BOOST_TEST_DYN_LINK
#include <boost/test/unit_test.hpp>
#include <assert.h>

namespace AliceO2 {
namespace QualityControl {
namespace Publisher {

BOOST_AUTO_TEST_CASE(arithmetic_test)
{
  int a = 1;
  int b = 2;
  BOOST_CHECK_NE(a, b);
  b = a;
  BOOST_CHECK_EQUAL(a, b);
}

BOOST_AUTO_TEST_CASE(hello_test)
{
}


} /* namespace Publisher */
} /* namespace QualityControl */
} /* namespace AliceO2 */
