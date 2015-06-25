///
/// \file   Publisher_test.cpp
/// \author Barthelemy von Haller
///

#include "../libs/Core/Quality.h"

#define BOOST_TEST_MODULE hello test
#define BOOST_TEST_MAIN
#define BOOST_TEST_DYN_LINK
#include <boost/test/unit_test.hpp>
#include <assert.h>
#include <iostream>

using namespace std;

namespace AliceO2 {
namespace QualityControl {
namespace Core {

BOOST_AUTO_TEST_CASE(hello_test)
{
  Quality asdf(123, "asdf");
  Quality myQuality = Quality::Bad;
//  cout << "Quality is : " << myQuality.getLevel() << " (" << myQuality.getName() << ")" << endl;
//  BOOST_CHECK_EQUAL(myQuality, Quality::Bad);
//  BOOST_CHECK_EQUAL(myQuality.getLevel(), 3);
}


} /* namespace Publisher */
} /* namespace QualityControl */
} /* namespace AliceO2 */
