///
/// \file   Publisher_test.cpp
/// \author Barthelemy von Haller
///

#include "QualityControl/ObjectsManager.h"

#define BOOST_TEST_MODULE Publisher test
#define BOOST_TEST_MAIN
#define BOOST_TEST_DYN_LINK
#include "../include/Common/Exceptions.h"
#include <TObjString.h>
#include <boost/test/unit_test.hpp>
#include <iostream>

using namespace std;
using namespace o2::quality_control::core;
using namespace AliceO2::Common;

namespace o2::quality_control::core
{

BOOST_AUTO_TEST_CASE(duplicate_object_test)
{
  TaskConfig config;
  config.taskName = "test";
  ObjectsManager objectsManager(config);
  TObjString s("content");
  objectsManager.startPublishing(&s);
  BOOST_CHECK_THROW(objectsManager.startPublishing(&s), o2::quality_control::core::DuplicateObjectError);
}

} // namespace o2::quality_control::core
