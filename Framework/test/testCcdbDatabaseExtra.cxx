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
/// \file   testCcdbDatabaseExtra.cxx
/// \author Adam Wegrzynek
/// \author Barthelemy von Haller
///

#include "QualityControl/DatabaseFactory.h"
#include <unordered_map>
#include "QualityControl/CcdbDatabase.h"
#include "QualityControl/QcInfoLogger.h"

#define BOOST_TEST_MODULE CcdbDatabaseExtra test
#define BOOST_TEST_MAIN
#define BOOST_TEST_DYN_LINK

#include <boost/test/unit_test.hpp>
#include <TH1F.h>
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
    ILOG(Info, Support) << "*** " << boost::unit_test::framework::current_test_case().p_name << " ***" << ENDM;
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
    ILOG(Info, Support) << "[RETRIEVE]: " << task << object << ENDM;
    auto mo = f.backend->retrieveMO(task, object);
    if (mo == nullptr) {
      ILOG(Info, Support) << "No object found (" << task << object << ")" << ENDM;
      continue;
    }
    ILOG(Info, Support) << "name of encapsulated object : " << mo->getObject()->GetName() << ENDM; // just to test it
  }
}

// It depends on what is in the database.
//BOOST_AUTO_TEST_CASE(ccdb_retrieve_all_json)
//{
//  test_fixture f;
//  for (auto const& [task, object] : Objects) {
//    ILOG(Info, Support) << "[JSON RETRIEVE]: " << task << "/" << object << ENDM;
//    auto json = f.backend->retrieveMOJson(task, object);
//    if (json.empty()) {
//      ILOG(Info, Support) << "skipping empty object..." << ENDM;
//      continue;
//    }
//    std::stringstream ss;
//    ss << json;
//    boost::property_tree::ptree pt;
//    boost::property_tree::read_json(ss, pt);
//  }
//}

long oldTimestamp;

BOOST_AUTO_TEST_CASE(ccdb_store)
{
  test_fixture f;

  TH1F* h1 = new TH1F("asdf/asdf", "asdf", 100, 0, 99);
  h1->FillRandom("gaus", 10000);
  shared_ptr<MonitorObject> mo1 = make_shared<MonitorObject>(h1, "my/task", "TST");
  oldTimestamp = CcdbDatabase::getCurrentTimestamp();
  f.backend->storeMO(mo1);

  shared_ptr<QualityObject> qo = make_shared<QualityObject>(Quality::Bad, "checkName", "TST", "OnAll");
  f.backend->storeQO(qo);
}

BOOST_AUTO_TEST_CASE(ccdb_retrieve_former_versions, *utf::depends_on("ccdb_store"))
{
  // store a new object
  test_fixture f;
  TH1F* h1 = new TH1F("asdf/asdf", "asdf", 100, 0, 99);
  h1->FillRandom("gaus", 10001);
  shared_ptr<MonitorObject> mo1 = make_shared<MonitorObject>(h1, "my/task", "TST");
  f.backend->storeMO(mo1);

  // Retrieve old object stored at timestampStorage
  std::shared_ptr<MonitorObject> mo = f.backend->retrieveMO("qc/TST/MO/my/task", "asdf/asdf", oldTimestamp);
  BOOST_CHECK(mo);
  TH1F* old = dynamic_cast<TH1F*>(mo->getObject());
  BOOST_CHECK_NE(old, nullptr);
  BOOST_CHECK_EQUAL(old->GetEntries(), 10000);

  // Retrieve latest object with timestamp
  std::shared_ptr<MonitorObject> mo2 = f.backend->retrieveMO("qc/TST/MO/my/task", "asdf/asdf", CcdbDatabase::getCurrentTimestamp());
  BOOST_CHECK(mo2);
  TH1F* latest = dynamic_cast<TH1F*>(mo2->getObject());
  BOOST_CHECK_NE(latest, nullptr);
  BOOST_CHECK_EQUAL(latest->GetEntries(), 10001);

  // Retrieve latest object without timetsamp
  std::shared_ptr<MonitorObject> mo3 = f.backend->retrieveMO("qc/TST/MO/my/task", "asdf/asdf");
  BOOST_CHECK(mo3);
  TH1F* latest2 = dynamic_cast<TH1F*>(mo3->getObject());
  BOOST_CHECK_NE(latest2, nullptr);
  BOOST_CHECK_EQUAL(latest2->GetEntries(), 10001);
}

BOOST_AUTO_TEST_CASE(ccdb_getobjects_name)
{
  test_fixture f;

  CcdbDatabase* ccdb = static_cast<CcdbDatabase*>(f.backend.get());
  ILOG(Info, Support) << "getListing()" << ENDM;
  auto tasks = ccdb->getListing("/qc");
  BOOST_CHECK_GT(tasks.size(), 0); // we know that there are a few
  // print but only for TST
  auto objects = f.backend->getPublishedObjectNames("/qc/TST");
  BOOST_CHECK_GT(objects.size(), 0);
}
} // namespace
} // namespace o2::quality_control::core
