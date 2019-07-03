// Copyright CERN and copyright holders of ALICE O2. This software is
// distributed under the terms of the GNU General Public License v3 (GPL
// Version 3), copied verbatim in the file "COPYING".
//
// See http://alice-o2.web.cern.ch/license for full licensing information.
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.

///
/// \file   testCcdbDatabase.cxx
/// \author Adam Wegrzynek
/// \author Bartheley von Haller
///

#include <QualityControl/DatabaseFactory.h>
#include <unordered_map>
#include <QualityControl/CcdbDatabase.h>

#define BOOST_TEST_MODULE CcdbDatabaseExtra test
#define BOOST_TEST_MAIN
#define BOOST_TEST_DYN_LINK

#include <boost/test/unit_test.hpp>
#include <TH1F.h>
#include <TFile.h>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>

namespace utf = boost::unit_test;

namespace o2::quality_control::core
{

namespace
{

using namespace o2::quality_control::core;
using namespace o2::quality_control::repository;
using namespace std;

const std::string CCDB_ENDPOINT = "ccdb-test.cern.ch:8080";
std::unordered_map<std::string, std::string> Objects;

/**
 * Fixture for the tests, i.e. code is ran in every test that uses it, i.e. it is like a setup and teardown for tests.
 */
struct test_fixture {
  test_fixture()
  {
    backend = DatabaseFactory::create("CCDB");
    backend->connect(CCDB_ENDPOINT, "", "", "");
    std::cout << "*** " << boost::unit_test::framework::current_test_case().p_name << " ***" << std::endl;
  }

  ~test_fixture() = default;

  std::unique_ptr<DatabaseInterface> backend;
  map<string, string> metadata;
};

// These tests should not be executed automatically, too much error prone.
// It depends on what is in the database.

BOOST_AUTO_TEST_CASE(ccdb_retrieve_all)
{
  test_fixture f;
  for (auto const& [task, object] : Objects) {
    std::cout << "[RETRIEVE]: " << task << object << std::endl;
    auto mo = f.backend->retrieve(task, object);
    if (mo == nullptr) {
      std::cout << "No object found (" << task << object << ")" << std::endl;
      continue;
    }
    cout << "name of encapsulated object : " << mo->getObject()->GetName() << endl; // just to test it
  }
}

// TODO this should not be executed automatically, too much error prone.
// It depends on what is in the database.
BOOST_AUTO_TEST_CASE(ccdb_retrieve_all_json)
{
  test_fixture f;
  for (auto const& [task, object] : Objects) {
    std::cout << "[JSON RETRIEVE]: " << task << "/" << object << std::endl;
    auto json = f.backend->retrieveJson(task, object);
    if (json.empty()) {
      std::cout << "skipping empty object..." << std::endl;
      continue;
    }
    std::stringstream ss;
    ss << json;
    boost::property_tree::ptree pt;
    boost::property_tree::read_json(ss, pt);
  }
}
} // namespace
} // namespace o2::quality_control::core
