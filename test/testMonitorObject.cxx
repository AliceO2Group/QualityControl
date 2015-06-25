///
/// \file   Publisher_test.cpp
/// \author Barthelemy von Haller
///

#include "Core/MonitorObject.h"

#define BOOST_TEST_MODULE MO test
#define BOOST_TEST_MAIN
#define BOOST_TEST_DYN_LINK
#include <boost/test/unit_test.hpp>
#include <assert.h>

namespace AliceO2 {
namespace QualityControl {
namespace Core {

BOOST_AUTO_TEST_CASE(arithmetic_test)
{
  int a = 1;
  int b = 2;
  BOOST_CHECK_NE(a, b);
  b = a;
  BOOST_CHECK_EQUAL(a, b);
  AliceO2::QualityControl::Core::MonitorObject obj;
}



} /* namespace Publisher */
} /* namespace QualityControl */
} /* namespace AliceO2 */
