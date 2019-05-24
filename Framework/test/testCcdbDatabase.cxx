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
///

#include <QualityControl/DatabaseFactory.h>
#include <unordered_map>

#define BOOST_TEST_MODULE CcdbDatabase test
#define BOOST_TEST_MAIN
#define BOOST_TEST_DYN_LINK

#include <boost/test/unit_test.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>

namespace o2::quality_control::core
{

namespace
{

using namespace o2::quality_control::core;
using namespace o2::quality_control::repository;

const std::string CCDB_ENDPOINT = "ccdb-test.cern.ch:8080";
std::unique_ptr<DatabaseInterface> BackendInstance;
std::unordered_map<std::string, std::string> Objects;

BOOST_AUTO_TEST_SUITE(optionalTest, *boost::unit_test::disabled())

BOOST_AUTO_TEST_CASE(ccdb_create)
{
  BackendInstance = DatabaseFactory::create("CCDB");
  BackendInstance->connect(CCDB_ENDPOINT, "", "", "");
}

BOOST_AUTO_TEST_CASE(ccdb_getobjects)
{
  auto tasks = BackendInstance->getListOfTasksWithPublications();
  for (auto& task : tasks) {
    auto objects = BackendInstance->getPublishedObjectNames(task);
    for (auto& object : objects) {
      Objects.insert({task, object});
    }
  }
}

BOOST_AUTO_TEST_CASE(ccdb_retrieve)
{
  for (auto const& [task, object] : Objects) {
    std::cout << "[RETRIEVE]: " << task << "/" << object << std::endl;
    BackendInstance->retrieve(task, object);
  }
}

BOOST_AUTO_TEST_CASE(ccdb_retrievejson)
{
  for (auto const& [task, object] : Objects) {
    std::cout << "[JSON RETRIEVE]: " << task << "/" << object << std::endl;
    auto json = BackendInstance->retrieveJson(task, object);
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

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_CASE(Dummy)
{
  BOOST_CHECK(true);
}

} // namespace anonymous
} // namespace o2::quality_control::core
