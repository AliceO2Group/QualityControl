///
/// \file   Publisher_test.cpp
/// \author Barthelemy von Haller
///

#include "../libs/Core/Publisher.h"

#define BOOST_TEST_MODULE Publisher test
#define BOOST_TEST_MAIN
#define BOOST_TEST_DYN_LINK
#include <boost/test/unit_test.hpp>
#include <Core/Exceptions.h>
#include <iostream>

using namespace std;
using namespace AliceO2::QualityControl::Core;

namespace AliceO2 {
namespace QualityControl {
namespace Core {

BOOST_AUTO_TEST_CASE(publisher_test)
{
  Publisher publisher;
  string s = "";
  publisher.startPublishing("test", &s);
  string *s2 = (string*)(publisher.getObject("test"));
  BOOST_CHECK_EQUAL(s, *s2);
  BOOST_CHECK_EQUAL(Quality::Null, publisher.getQuality("test"));
  publisher.setQuality("test", Quality::Medium);
  BOOST_CHECK_EQUAL(Quality::Medium, publisher.getQuality("test"));
  BOOST_CHECK_THROW(publisher.getQuality("test2"), ObjectNotFoundError);

  // that is just for me to see how it looks like
  try {
    publisher.getQuality("test2");
  } catch (ObjectNotFoundError &e) {
    std::cout << e.what() << std::endl;
    if ( std::string const * extra  = boost::get_error_info<errinfo_object_name>(e) ) {
      std::cout << "object name : " << *extra << std::endl;
    }
  }
}

} /* namespace Publisher */
} /* namespace QualityControl */
} /* namespace AliceO2 */
