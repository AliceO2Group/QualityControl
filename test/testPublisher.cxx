///
/// \file   Publisher_test.cpp
/// \author Barthelemy von Haller
///

#include "QualityControl/ObjectsManager.h"

#define BOOST_TEST_MODULE Publisher test
#define BOOST_TEST_MAIN
#define BOOST_TEST_DYN_LINK
#include <boost/test/unit_test.hpp>
#include "../include/Common/Exceptions.h"
#include <iostream>
#include <TObjString.h>

using namespace std;
using namespace AliceO2::QualityControl::Core;
using namespace AliceO2::Common;

namespace AliceO2 {
namespace QualityControl {
namespace Core {

BOOST_AUTO_TEST_CASE(publisher_test)
{
  ObjectsManager objectsManager;
  TObjString s("content");
  objectsManager.startPublishing("test", &s);
  TObjString *s2 = (TObjString*)(objectsManager.getObject("test"));
  BOOST_CHECK_EQUAL(s.GetString(), s2->GetString());
  BOOST_CHECK_EQUAL(Quality::Null, objectsManager.getQuality("test"));
  objectsManager.setQuality("test", Quality::Medium);
  BOOST_CHECK_EQUAL(Quality::Medium, objectsManager.getQuality("test"));
  BOOST_CHECK_THROW(objectsManager.getQuality("test2"), ObjectNotFoundError);

  // that is just for me to see how it looks like
  try {
    objectsManager.getQuality("test2");
  } catch (ObjectNotFoundError &e) {
    std::cout << e.what() << std::endl;
    if ( std::string const * extra  = boost::get_error_info<errinfo_object_name>(e) ) {
      std::cout << "object name : " << *extra << std::endl;
    }
  }
}

} /* namespace ObjectsManager */
} /* namespace QualityControl */
} /* namespace AliceO2 */
