
///
/// \file   testTRD.cxx
/// \author My Name
///

#include "QualityControl/TaskFactory.h"

#define BOOST_TEST_MODULE Publisher test
#define BOOST_TEST_MAIN
#define BOOST_TEST_DYN_LINK

#include <boost/test/unit_test.hpp>

namespace o2::quality_control_modules::trd
{
BOOST_AUTO_TEST_CASE(instantiate_task) { BOOST_CHECK(true); }

} // namespace o2::quality_control_modules::trd
