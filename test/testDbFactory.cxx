///
/// \file   Publisher_test.cpp
/// \author Barthelemy von Haller
///

#include "../include/QualityControl/DatabaseFactory.h"
#ifdef _WITH_MYSQL
#include "QualityControl/MySqlDatabase.h"
#endif
#define BOOST_TEST_MODULE Quality test
#define BOOST_TEST_MAIN
#define BOOST_TEST_DYN_LINK
#include <boost/test/unit_test.hpp>
#include <cassert>
#include <iostream>

using namespace std;

namespace AliceO2 {
namespace QualityControl {
namespace Repository {

bool do_nothing( Common::FatalException const& ex ) { return true; }

BOOST_AUTO_TEST_CASE(db_factory_test)
{
#ifdef _WITH_MYSQL
  DatabaseInterface *database = DatabaseFactory::create("MySql");
  BOOST_CHECK(database);
  BOOST_CHECK(dynamic_cast<MySqlDatabase*>(database));
#endif

  DatabaseInterface *database2 = nullptr;
  BOOST_CHECK_EXCEPTION(database2 = DatabaseFactory::create("asf"), Common::FatalException, do_nothing );
  BOOST_CHECK(!database2);
}

} /* namespace Repository */
} /* namespace QualityControl */
} /* namespace AliceO2 */
