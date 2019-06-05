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
#include <TH1F.h>
#include <TFile.h>

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

BOOST_AUTO_TEST_CASE(ccdb_create)
{
  test_fixture f;
}

BOOST_AUTO_TEST_CASE(ccdb_getobjects_name)
{
  test_fixture f;

  auto tasks = f.backend->getListOfTasksWithPublications();
  for (auto& task : tasks) {
    auto objects = f.backend->getPublishedObjectNames(task);
    for (auto& object : objects) {
      Objects.insert({ task, object });
    }
  }
}

BOOST_AUTO_TEST_CASE(ccdb_store) {
  test_fixture f;
  TH1F *h1 = new TH1F("asdf/asdf", "asdf", 100, 0, 99);
  h1->FillRandom("gaus", 10000);
  shared_ptr<MonitorObject> mo1 = make_shared<MonitorObject>(h1, "my/task");
  f.backend->store(mo1);
}

BOOST_AUTO_TEST_CASE(ccdb_retrieve)
{
  test_fixture f;
  MonitorObject* mo = f.backend->retrieve("my/task", "asdf/asdf");
  BOOST_CHECK_NE(mo, nullptr);
  TH1F *h1 = dynamic_cast<TH1F*>(mo->getObject());
  BOOST_CHECK_NE(h1, nullptr);
  BOOST_CHECK_EQUAL(h1->GetEntries(), 10000);
}

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

BOOST_AUTO_TEST_CASE(ccdb_retrievejson)
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
