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

#include "QualityControl/DatabaseFactory.h"
#include <unordered_map>
#include "QualityControl/CcdbDatabase.h"
#include "QualityControl/QcInfoLogger.h"
#include "QualityControl/Version.h"

#define BOOST_TEST_MODULE CcdbDatabase test
#define BOOST_TEST_MAIN
#define BOOST_TEST_DYN_LINK

#include <boost/test/unit_test.hpp>
#include <TH1F.h>

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
    ILOG(Info) << "*** " << boost::unit_test::framework::current_test_case().p_name << " ***" << ENDM;
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

  CcdbDatabase* ccdb = static_cast<CcdbDatabase*>(f.backend.get());
  ILOG(Info) << "getListing()" << ENDM;
  auto tasks = ccdb->getListing();
  for (auto& task : tasks) {
    ILOG(Info) << "getPublishedObjectNames of task " << task << ENDM;
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
  shared_ptr<MonitorObject> mo1 = make_shared<MonitorObject>(h1, "my/task", "TST");

  shared_ptr<QualityObject> qo1 = make_shared<QualityObject>("test-ccdb-check", vector{ string("input1"), string("input2") }, "TST");
  qo1->setQuality(Quality::Bad);

  oldTimestamp = CcdbDatabase::getCurrentTimestamp();
  f.backend->storeMO(mo1);
  f.backend->storeQO(qo1);
}

BOOST_AUTO_TEST_CASE(ccdb_store_for_future_tests)
{
  // this test is storing a version of the objects in a different directory.
  // The goal is to keep old versions of the objects, in old formats, for future backward compatibility testing.
  test_fixture f;

  TH1F* h1 = new TH1F("to_be_kept", "asdf", 100, 0, 99);
  h1->FillRandom("gaus", 12345);
  shared_ptr<MonitorObject> mo1 = make_shared<MonitorObject>(h1, "task", "TST_KEEP");
  mo1->addMetadata("Run", o2::quality_control::core::Version::GetQcVersion().getString());
  shared_ptr<QualityObject> qo1 = make_shared<QualityObject>("check", vector{ string("input1"), string("input2") }, "TST_KEEP");
  qo1->setQuality(Quality::Bad);
  qo1->addMetadata("Run", o2::quality_control::core::Version::GetQcVersion().getString());

  f.backend->storeMO(mo1);
  f.backend->storeQO(qo1);
}

BOOST_AUTO_TEST_CASE(ccdb_retrieve, *utf::depends_on("ccdb_store"))
{
  test_fixture f;

  std::shared_ptr<TObject> obj = f.backend->retrieveTObject("qc/TST/my/task/asdf/asdf");
  auto mo = dynamic_pointer_cast<MonitorObject>(obj);
  auto mo2 = f.backend->retrieveMO("qc/TST/my/task", "asdf/asdf");
  BOOST_CHECK_NE(mo, nullptr);
  BOOST_CHECK_EQUAL(mo->getName(), mo2->getName());
  TH1F* h1 = dynamic_cast<TH1F*>(mo->getObject());
  BOOST_CHECK_NE(h1, nullptr);
  BOOST_CHECK_EQUAL(h1->GetEntries(), 10000);

  std::shared_ptr<TObject> obj2 = f.backend->retrieveTObject("qc/checks/TST/test-ccdb-check");
  auto qo = dynamic_pointer_cast<QualityObject>(obj2);
  BOOST_CHECK_NE(qo, nullptr);
  BOOST_CHECK_EQUAL(qo->getQuality(), Quality::Bad);
}

BOOST_AUTO_TEST_CASE(ccdb_retrieve_mo, *utf::depends_on("ccdb_store"))
{
  test_fixture f;
  std::shared_ptr<MonitorObject> mo = f.backend->retrieveMO("qc/TST/my/task", "asdf/asdf");
  BOOST_REQUIRE_NE(mo, nullptr);
  TH1F* h1 = dynamic_cast<TH1F*>(mo->getObject());
  BOOST_REQUIRE_NE(h1, nullptr);
  BOOST_CHECK_EQUAL(h1->GetEntries(), 10000);

  std::shared_ptr<QualityObject> qo = f.backend->retrieveQO("qc/checks/TST/test-ccdb-check");
  BOOST_REQUIRE_NE(qo, nullptr);
  BOOST_CHECK_EQUAL(qo->getName(), "test-ccdb-check");
  BOOST_REQUIRE_EQUAL(qo->getInputs().size(), 2);
  BOOST_CHECK_EQUAL(qo->getInputs()[0], "input1");
  BOOST_CHECK_EQUAL(qo->getInputs()[1], "input2");
  BOOST_CHECK_EQUAL(qo->getDetectorName(), "TST");
}

BOOST_AUTO_TEST_CASE(ccdb_retrieve_qo, *utf::depends_on("ccdb_store"))
{
  test_fixture f;
  std::shared_ptr<QualityObject> qo = f.backend->retrieveQO("qc/checks/TST/test-ccdb-check");
  BOOST_CHECK_NE(qo, nullptr);
  Quality q = qo->getQuality();
  BOOST_CHECK_EQUAL(q.getLevel(), 3);
}

BOOST_AUTO_TEST_CASE(ccdb_retrieve_json, *utf::depends_on("ccdb_store"))
{
  test_fixture f;

  std::string task = "qc/TST/my/task";
  std::string object = "asdf/asdf";
  std::cout << "[json retrieve]: " << task << "/" << object << std::endl;
  auto json = f.backend->retrieveJson(task + "/" + object);
  auto json2 = f.backend->retrieveMOJson(task, object);

  cout << "mo json : " << json << endl;

  BOOST_CHECK(!json.empty());
  BOOST_CHECK_EQUAL(json, json2);

  string qualityPath = "qc/checks/TST/test-ccdb-check";
  std::cout << "[json retrieve]: " << qualityPath << std::endl;
  auto json3 = f.backend->retrieveJson(qualityPath);
  auto json4 = f.backend->retrieveQOJson(qualityPath);
  cout << "qo json : " << json3 << endl;
  BOOST_CHECK(!json3.empty());
  BOOST_CHECK_EQUAL(json3, json4);
}

BOOST_AUTO_TEST_CASE(ccdb_retrieve_mo_json, *utf::depends_on("ccdb_store"))
{
  test_fixture f;
  std::string task = "qc/TST/my/task";
  std::string object = "asdf/asdf";
  std::cout << "[json retrieve]: " << task << "/" << object << std::endl;
  auto jsonMO = f.backend->retrieveMOJson(task, object);

  BOOST_CHECK(!jsonMO.empty());

  std::string qoPath = "qc/checks/TST/test-ccdb-check";
  std::shared_ptr<QualityObject> qo = f.backend->retrieveQO(qoPath);
  std::cout << "[json retrieve]: " << qoPath << std::endl;
  auto jsonQO = f.backend->retrieveQOJson(qoPath);

  BOOST_CHECK(!jsonQO.empty());
}

} // namespace
} // namespace o2::quality_control::core
