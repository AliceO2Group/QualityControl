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

#define BOOST_TEST_MODULE CcdbDatabase test
#define BOOST_TEST_MAIN
#define BOOST_TEST_DYN_LINK

#include <boost/test/unit_test.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <TH1F.h>
#include <TFile.h>

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

BOOST_AUTO_TEST_CASE(ccdb_create)
{
  test_fixture f;

  f.backend->truncate("my/task", "*");
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

long oldTimestamp;

BOOST_AUTO_TEST_CASE(ccdb_store)
{
  test_fixture f;
  TH1F* h1 = new TH1F("asdf/asdf", "asdf", 100, 0, 99);
  h1->FillRandom("gaus", 10000);
  shared_ptr<MonitorObject> mo1 = make_shared<MonitorObject>(h1, "my/task");
  oldTimestamp = CcdbDatabase::getCurrentTimestamp();
  f.backend->store(mo1);
}

BOOST_AUTO_TEST_CASE(ccdb_retrieve, *utf::depends_on("ccdb_store"))
{
  test_fixture f;
  MonitorObject* mo = f.backend->retrieve("my/task", "asdf/asdf");
  BOOST_CHECK_NE(mo, nullptr);
  TH1F* h1 = dynamic_cast<TH1F*>(mo->getObject());
  BOOST_CHECK_NE(h1, nullptr);
  BOOST_CHECK_EQUAL(h1->GetEntries(), 10000);
}

BOOST_AUTO_TEST_CASE(ccdb_retrieve_former_versions, *utf::depends_on("ccdb_store"))
{
  // store a new object
  test_fixture f;
  TH1F* h1 = new TH1F("asdf/asdf", "asdf", 100, 0, 99);
  h1->FillRandom("gaus", 10001);
  shared_ptr<MonitorObject> mo1 = make_shared<MonitorObject>(h1, "my/task");
  f.backend->store(mo1);

  // Retrieve old object stored at timestampStorage
  MonitorObject* mo = f.backend->retrieve("my/task", "asdf/asdf", oldTimestamp);
  BOOST_CHECK_NE(mo, nullptr);
  TH1F* old = dynamic_cast<TH1F*>(mo->getObject());
  BOOST_CHECK_NE(old, nullptr);
  BOOST_CHECK_EQUAL(old->GetEntries(), 10000);

  // Retrieve latest object with timestamp
  MonitorObject* mo2 = f.backend->retrieve("my/task", "asdf/asdf", CcdbDatabase::getCurrentTimestamp());
  BOOST_CHECK_NE(mo2, nullptr);
  TH1F* latest = dynamic_cast<TH1F*>(mo2->getObject());
  BOOST_CHECK_NE(latest, nullptr);
  BOOST_CHECK_EQUAL(latest->GetEntries(), 10001);

  // Retrieve latest object without timetsamp
  MonitorObject* mo3 = f.backend->retrieve("my/task", "asdf/asdf");
  BOOST_CHECK_NE(mo3, nullptr);
  TH1F* latest2 = dynamic_cast<TH1F*>(mo3->getObject());
  BOOST_CHECK_NE(latest2, nullptr);
  BOOST_CHECK_EQUAL(latest2->GetEntries(), 10001);
}

} // namespace
} // namespace o2::quality_control::core
