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
using namespace o2::quality_control::core;
using namespace AliceO2::Common;

namespace o2 {
namespace quality_control {
namespace core {

BOOST_AUTO_TEST_CASE(publisher_test)
{
  TaskConfig config;
  config.taskName = "test";
  ObjectsManager objectsManager(config);
  TObjString s("content");
  objectsManager.startPublishing(&s, "test");
  TObjString *s2 = (TObjString*)(objectsManager.getObject("test"));
  BOOST_CHECK_EQUAL(s.GetString(), s2->GetString());
  BOOST_CHECK_EQUAL(Quality::Null, objectsManager.getQuality("test"));
  auto mo = objectsManager.getMonitorObject("test");
  BOOST_CHECK_THROW(mo->setQualityForCheck("test", Quality::Medium), AliceO2::Common::ObjectNotFoundError);
  CheckDefinition check;
  check.name = "test";
  check.libraryName = "test";
  check.className = "test";
  check.result = Quality::Medium;
  mo->addOrReplaceCheck("test", check);
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

} /* namespace core */
} /* namespace quality_control */
} /* namespace o2 */
