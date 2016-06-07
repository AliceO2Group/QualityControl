///
/// \file   Publisher_test.cpp
/// \author Barthelemy von Haller
///

#include "../include/QualityControl/DatabaseFactory.h"
#include "../include/QualityControl/MySqlDatabase.h"

#define BOOST_TEST_MODULE Quality test
#define BOOST_TEST_MAIN
#define BOOST_TEST_DYN_LINK
#include <boost/test/unit_test.hpp>
#include <assert.h>
#include <iostream>

using namespace std;

namespace AliceO2 {
namespace QualityControl {
namespace Repository {

bool do_nothing( Common::FatalException const& ex ) { return true; }

BOOST_AUTO_TEST_CASE(db_factory_test)
{
  DatabaseInterface *database = DatabaseFactory::create("MySql");
  BOOST_CHECK(database);
  BOOST_CHECK(dynamic_cast<MySqlDatabase*>(database));

  database = nullptr;
  BOOST_CHECK_EXCEPTION(database = DatabaseFactory::create("asf"), Common::FatalException, do_nothing );
  BOOST_CHECK(!database);
}

} /* namespace Repository */
} /* namespace QualityControl */
} /* namespace AliceO2 */
