// Copyright 2019-2020 CERN and copyright holders of ALICE O2.
// See https://alice-o2.web.cern.ch/copyright for details of the copyright holders.
// All rights not expressly granted are reserved.
//
// This software is distributed under the terms of the GNU General Public
// License v3 (GPL Version 3), copied verbatim in the file "COPYING".
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.

///
/// \file   testCcdbDatabase.cxx
/// \author Adam Wegrzynek
/// \author Barthelemy von Haller
///

#include <unordered_map>
#include "QualityControl/CcdbDatabase.h"
#include "QualityControl/QcInfoLogger.h"
#include "QualityControl/Version.h"

#define BOOST_TEST_MODULE CcdbDatabase test
#define BOOST_TEST_MAIN
#define BOOST_TEST_DYN_LINK

#include <boost/test/unit_test.hpp>
#include <TH1F.h>
#include "QualityControl/RepoPathUtils.h"
#include "QualityControl/testUtils.h"
#include <TROOT.h>

namespace utf = boost::unit_test;

namespace o2::quality_control::core
{

namespace
{

using namespace o2::quality_control::core;
using namespace o2::quality_control::repository;
using namespace std;

const std::string CCDB_ENDPOINT = "ccdb-test.cern.ch:8080";

/**
 * Fixture for the tests, i.e. code is ran in every test that uses it, i.e. it is like a setup and teardown for tests.
 */
struct test_fixture {
  test_fixture()
  {
    backend = std::make_unique<CcdbDatabase>();
    backend->connect(CCDB_ENDPOINT, "", "", "");
    pid = std::to_string(getpid());
    taskName = "Test/pid" + pid;
    ILOG(Info, Support) << "*** " << boost::unit_test::framework::current_test_case().p_name << " (" << pid
                        << ") ***" << ENDM;
  }

  ~test_fixture() = default;

  // shorthands to get the paths to the objects and their containing folder
  std::string getQoPath(const string& checkName, const string& provenance = "qc") const
  {
    return RepoPathUtils::getQoPath(detector, taskName + "/" + checkName, "", {}, provenance);
  }
  std::string getMoPath(const string& objectName, const string& provenance = "qc") const
  {
    return RepoPathUtils::getMoPath(detector, taskName, objectName, provenance);
  }
  std::string getMoFolder(const string& objectName, const string& provenance = "qc") const
  {
    string fullMoPath = getMoPath(objectName, provenance);
    return fullMoPath.substr(0, fullMoPath.find_last_of('/'));
  }

  std::unique_ptr<CcdbDatabase> backend;
  map<string, string> metadata;
  std::string pid;
  const std::string detector = "TST";
  std::string taskName;
};

struct MyGlobalFixture {
  void teardown()
  {
    std::unique_ptr<CcdbDatabase> backend = std::make_unique<CcdbDatabase>();
    backend->connect(CCDB_ENDPOINT, "", "", "");
    // cannot use the test_fixture because we are tearing down
    backend->truncate("qc/TST/MO/Test/pid" + std::to_string(getpid()), "*");
    backend->truncate("qc/TST/QO/Test/pid" + std::to_string(getpid()), "*");
    backend->truncate("qc_hello/TST/MO/Test/pid" + std::to_string(getpid()), "*");
    backend->truncate("qc_hello/TST/QO/Test/pid" + std::to_string(getpid()), "*");
  }
};
BOOST_TEST_GLOBAL_FIXTURE(MyGlobalFixture);

long oldTimestamp;

BOOST_AUTO_TEST_CASE(ccdb_store)
{
  test_fixture f;

  TH1F* h1 = new TH1F("quarantine", "asdf", 100, 0, 99);
  h1->FillRandom("gaus", 10000);
  shared_ptr<MonitorObject> mo1 = make_shared<MonitorObject>(h1, f.taskName, "TST");
  mo1->updateActivity(1234, "LHC66", "passName1", "qc");

  TH1F* h2 = new TH1F("metadata", "asdf", 100, 0, 99);
  shared_ptr<MonitorObject> mo2 = make_shared<MonitorObject>(h2, f.taskName, "TST");
  mo2->addMetadata("my_meta", "is_good");

  TH1F* h3 = new TH1F("short", "asdf", 100, 0, 99);
  shared_ptr<MonitorObject> mo3 = make_shared<MonitorObject>(h3, f.taskName, "TST");

  TH1F* h4 = new TH1F("provenance", "asdf", 100, 0, 99);
  shared_ptr<MonitorObject> mo4 = make_shared<MonitorObject>(h4, f.taskName, "TST");
  mo4->updateActivity(1234, "LHC66", "passName1", "qc_hello");

  shared_ptr<QualityObject> qo1 = make_shared<QualityObject>(Quality::Bad, f.taskName + "/test-ccdb-check", "TST", "OnAll", vector{ string("input1"), string("input2") });
  qo1->updateActivity(1234, "LHC66", "passName1", "qc");
  shared_ptr<QualityObject> qo2 = make_shared<QualityObject>(Quality::Null, f.taskName + "/metadata", "TST", "OnAll", vector{ string("input1") });
  qo2->addMetadata("my_meta", "is_good");
  shared_ptr<QualityObject> qo3 = make_shared<QualityObject>(Quality::Good, f.taskName + "/short", "TST", "OnAll", vector{ string("input1") });
  shared_ptr<QualityObject> qo4 = make_shared<QualityObject>(Quality::Good, f.taskName + "/provenance", "TST", "OnAll", vector{ string("input1") });
  qo4->updateActivity(0, "", "", "qc_hello");

  oldTimestamp = CcdbDatabase::getCurrentTimestamp();
  f.backend->storeMO(mo1);
  f.backend->storeMO(mo2);
  f.backend->storeMO(mo4);
  f.backend->storeQO(qo1);
  f.backend->storeQO(qo2);
  f.backend->storeQO(qo4);

  // with timestamps
  f.backend->storeMO(mo3, 10000, 20000);
  f.backend->storeQO(qo3, 10000, 20000);
}

BOOST_AUTO_TEST_CASE(ccdb_store_for_future_tests)
{
  // this test is storing a version of the objects in a different directory.
  // The goal is to keep old versions of the objects, in old formats, for future backward compatibility testing.
  test_fixture f;

  TH1F* h1 = new TH1F("to_be_kept", "asdf", 100, 0, 99);
  h1->FillRandom("gaus", 12345);
  shared_ptr<MonitorObject> mo1 = make_shared<MonitorObject>(h1, "task", "TST_KEEP");
  mo1->addMetadata("RunNumber", o2::quality_control::core::Version::GetQcVersion().getString());
  shared_ptr<QualityObject> qo1 = make_shared<QualityObject>(Quality::Bad, "check", "TST_KEEP", "OnAll", vector{ string("input1"), string("input2") });
  qo1->addMetadata("RunNumber", o2::quality_control::core::Version::GetQcVersion().getString());

  f.backend->storeMO(mo1);
  f.backend->storeQO(qo1);
}

BOOST_AUTO_TEST_CASE(ccdb_retrieve_mo, *utf::depends_on("ccdb_store"))
{
  test_fixture f;
  std::shared_ptr<MonitorObject> mo = f.backend->retrieveMO(f.getMoFolder("quarantine"), "quarantine");
  BOOST_REQUIRE_NE(mo, nullptr);
  BOOST_CHECK_EQUAL(mo->getName(), "quarantine");
  BOOST_CHECK_EQUAL(mo->getActivity().mId, 1234);
  BOOST_CHECK_EQUAL(mo->getActivity().mPeriodName, "LHC66");
  BOOST_CHECK_EQUAL(mo->getActivity().mPassName, "passName1");
  BOOST_CHECK_EQUAL(mo->getActivity().mProvenance, "qc");
}

BOOST_AUTO_TEST_CASE(ccdb_retrieve_timestamps, *utf::depends_on("ccdb_store"))
{
  test_fixture f;

  std::shared_ptr<MonitorObject> mo = f.backend->retrieveMO(f.getMoFolder("short"), "short", 15000);
  BOOST_REQUIRE_NE(mo, nullptr);
  BOOST_CHECK_EQUAL(mo->getName(), "short");

  std::shared_ptr<QualityObject> qo = f.backend->retrieveQO(f.getQoPath("short"), 15000);
  BOOST_REQUIRE_NE(qo, nullptr);
  BOOST_CHECK_EQUAL(qo->getName(), f.taskName + "/short");
}

BOOST_AUTO_TEST_CASE(ccdb_retrieve_inexisting_mo)
{
  test_fixture f;

  std::shared_ptr<MonitorObject> mo = f.backend->retrieveMO("non/existing", "object");
  BOOST_CHECK(mo == nullptr);
}

BOOST_AUTO_TEST_CASE(ccdb_retrieve_qo, *utf::depends_on("ccdb_store"))
{
  test_fixture f;
  std::shared_ptr<QualityObject> qo = f.backend->retrieveQO(RepoPathUtils::getQoPath("TST", f.taskName + "/test-ccdb-check"));
  BOOST_CHECK_NE(qo, nullptr);
  Quality q = qo->getQuality();
  BOOST_CHECK_EQUAL(q.getLevel(), 3);
  BOOST_CHECK_EQUAL(qo->getActivity().mId, 1234);
  BOOST_CHECK_EQUAL(qo->getActivity().mPeriodName, "LHC66");
  BOOST_CHECK_EQUAL(qo->getActivity().mPassName, "passName1");
  BOOST_CHECK_EQUAL(qo->getActivity().mProvenance, "qc");
}

BOOST_AUTO_TEST_CASE(ccdb_provenance, *utf::depends_on("ccdb_store"))
{
  test_fixture f;
  std::shared_ptr<QualityObject> qo = f.backend->retrieveQO(RepoPathUtils::getQoPath("TST", f.taskName + "/provenance", "", {}, "qc_hello"));
  BOOST_CHECK_NE(qo, nullptr);
  BOOST_CHECK_EQUAL(qo->getActivity().mProvenance, "qc_hello");

  std::shared_ptr<MonitorObject> mo = f.backend->retrieveMO(f.getMoFolder("provenance", "qc_hello"), "provenance");
  BOOST_CHECK_NE(mo, nullptr);
  BOOST_CHECK_EQUAL(mo->getActivity().mProvenance, "qc_hello");
}

unique_ptr<CcdbDatabase> backendGlobal = std::make_unique<CcdbDatabase>();

void askObject(std::string objectPath)
{
  cout << "in askObject" << endl;
  map<string, string> metadata;
  map<string, string> headers;
  auto json = backendGlobal->retrieveJson(objectPath, -1, metadata);
  cout << "std::string::max_size(): " << std::string().max_size() << endl;
  cout << "json string size: " << json.size() << endl;
  cout << "object " << json.substr(10) << endl;
  BOOST_CHECK(!json.empty());
  cout << "finished " << endl;
}

BOOST_AUTO_TEST_CASE(ccdb_test_thread, *utf::depends_on("ccdb_store"))
{
  ROOT::EnableThreadSafety();
  string pid = std::to_string(getpid());
  string taskName = "Test/pid" + pid;
  string objectPath = RepoPathUtils::getMoPath("TST", taskName, "quarantine");
  backendGlobal->connect(CCDB_ENDPOINT, "", "", "");
  int iterations = 10;
  vector<std::thread> threads;

  for (int i = 0; i < iterations; i++) {
    cout << "Asking for object, iteration " << i << endl;
    threads.emplace_back(askObject, objectPath);
  }

  for (std::thread& t : threads) {
    t.join();
  }
  threads.clear();
}

unique_ptr<o2::ccdb::CcdbApi> apiGlobal = std::make_unique<o2::ccdb::CcdbApi>();

void askObjectApi(std::string objectPath)
{
  cout << "in askObject" << endl;
  map<string, string> metadata;
  map<string, string> headers;

  auto* object = apiGlobal->retrieveFromTFileAny<TObject>(objectPath, metadata, -1, &headers);
  BOOST_CHECK(object != nullptr);
  cout << "finished " << endl;
}

BOOST_AUTO_TEST_CASE(ccdb_test_thread_api, *utf::depends_on("ccdb_store"))
{
  ROOT::EnableThreadSafety();
  string pid = std::to_string(getpid());
  string taskName = "Test/pid" + pid;
  string objectPath = RepoPathUtils::getMoPath("TST", taskName, "quarantine");
  cout << "objectPath: " << objectPath << endl;
  apiGlobal->init(CCDB_ENDPOINT);
  int iterations = 10;
  vector<std::thread> threads;

  for (int i = 0; i < iterations; i++) {
    cout << "Asking for object, iteration " << i << endl;
    threads.emplace_back(askObjectApi, objectPath);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }

  for (std::thread& t : threads) {
    t.join();
  }
  threads.clear();
}

BOOST_AUTO_TEST_CASE(ccdb_test_no_thread_api)
{
  unique_ptr<o2::ccdb::CcdbApi> api = std::make_unique<o2::ccdb::CcdbApi>();
  string ccdbUrl = "http://ccdb-test.cern.ch:8080";
  api->init(ccdbUrl);
  cout << "ccdb url: " << ccdbUrl << endl;
  bool hostReachable = api->isHostReachable();
  cout << "Is host reachable ? --> " << hostReachable << endl;
  string objectPath = "qc/DAQ/MO/daqTask/UNKNOWN/sumRdhSizesPerInputRecord";
  int iterations = 3;
  map<string, string> metadata;
  map<string, string> headers;

  for (int i = 0; i < iterations; i++) {
    cout << "Asking for object, iteration " << i << endl;
    auto* object = api->retrieveFromTFileAny<TObject>(objectPath, metadata);
    cout << "object : " << object << endl;
  }
}

BOOST_AUTO_TEST_CASE(ccdb_metadata, *utf::depends_on("ccdb_store"))
{
  test_fixture f;

  std::string pathQuarantine = f.getMoPath("quarantine");
  std::string pathMetadata = f.getMoPath("metadata");
  std::string pathQuality = f.getQoPath("test-ccdb-check");
  std::string pathQualityMetadata = f.getQoPath("metadata");

  std::map<std::string, std::string> headers1;
  std::map<std::string, std::string> headers2;
  TObject* obj1 = f.backend->retrieveTObject(pathQuarantine, f.metadata, -1, &headers1);
  TObject* obj2 = f.backend->retrieveTObject(pathMetadata, f.metadata, -1, &headers2);
  BOOST_CHECK_NE(obj1, nullptr);
  BOOST_CHECK_NE(obj2, nullptr);
  BOOST_CHECK(headers1.size() > 0);
  BOOST_CHECK(headers2.size() > 1);
  BOOST_CHECK_EQUAL(headers1.count("my_meta"), 0);
  BOOST_CHECK_EQUAL(headers2.count("my_meta"), 1);
  BOOST_CHECK_EQUAL(headers2.at("my_meta"), "is_good");

  // get the path without the objectName because of the interface retrieveMO
  auto obj1a = f.backend->retrieveMO(f.getMoFolder("quarantine"), "quarantine");
  auto obj2a = f.backend->retrieveMO(f.getQoPath(""), "metadata");
  BOOST_CHECK_NE(obj1a, nullptr);
  BOOST_CHECK_NE(obj2a, nullptr);
  BOOST_CHECK(obj1a->getMetadataMap().size() > 0);
  BOOST_CHECK(obj2a->getMetadataMap().size() > 1);
  BOOST_CHECK_EQUAL(obj1a->getMetadataMap().count("my_meta"), 0);
  BOOST_CHECK_EQUAL(obj2a->getMetadataMap().count("my_meta"), 1);
  BOOST_CHECK_EQUAL(obj2a->getMetadataMap().at("my_meta"), "is_good");

  auto obj3 = f.backend->retrieveQO(pathQuality);
  auto obj4 = f.backend->retrieveQO(pathQualityMetadata);
  BOOST_CHECK_NE(obj3, nullptr);
  BOOST_CHECK_NE(obj4, nullptr);
  BOOST_CHECK(obj3->getMetadataMap().size() > 0);
  BOOST_CHECK(obj4->getMetadataMap().size() > 1);
  BOOST_CHECK_EQUAL(obj3->getMetadataMap().count("my_meta"), 0);
  BOOST_CHECK_EQUAL(obj4->getMetadataMap().count("my_meta"), 1);
  BOOST_CHECK_EQUAL(obj4->getMetadataMap().at("my_meta"), "is_good");
}

BOOST_AUTO_TEST_CASE(ccdb_store_retrieve_any)
{
  test_fixture f;

  std::map<std::string, std::string> meta;
  TH1F* h1 = new TH1F("quarantine", "asdf", 100, 0, 99);
  h1->FillRandom("gaus", 10000);

  f.backend->storeAny(h1, typeid(TH1F), f.getMoPath("storeAny"), meta, "TST", "testStoreAny");

  meta.clear();
  void* rawResult = f.backend->retrieveAny(typeid(TH1F), f.getMoPath("storeAny"), meta);
  auto h1_back = static_cast<TH1F*>(rawResult);
  BOOST_CHECK(rawResult != nullptr);
  BOOST_CHECK(h1_back != nullptr);
  BOOST_CHECK(h1_back->GetNbinsX() == 100);
  BOOST_CHECK(h1_back->GetEntries() > 0);
}

} // namespace
} // namespace o2::quality_control::core
