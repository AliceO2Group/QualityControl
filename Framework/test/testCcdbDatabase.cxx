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

#include "QualityControl/Activity.h"
#include "QualityControl/CcdbDatabase.h"
#include "QualityControl/QcInfoLogger.h"
#include "QualityControl/Version.h"

#include <boost/property_tree/ptree.hpp>
#include <TH1F.h>
#include "QualityControl/RepoPathUtils.h"
#include "QualityControl/ObjectMetadataKeys.h"
#include "QualityControl/ActivityHelpers.h"
#include <DataFormatsQualityControl/QualityControlFlagCollection.h>
#include <DataFormatsQualityControl/FlagTypeFactory.h>
#include <TROOT.h>
#include <CCDB/CcdbApi.h>
#include <atomic>
#include <cstdlib>
#include <thread>
#include <unistd.h>
#include <catch_amalgamated.hpp>

namespace o2::quality_control::core
{

// anonymous namespace to keep these symbols visible only to this file (instead of all files combined into o2-qc-test-ccdb)
namespace
{

using namespace o2::quality_control;
using namespace o2::quality_control::core;
using namespace o2::quality_control::repository;
using namespace std;

// A QC-specific CCDB instance: ali-qcdb-test by default. It holds different
// objects from ccdb-test -- see the TODO in testTimekeeper.cxx -- so this is
// deliberately NOT ALICEO2_CCDB_HOST. ALICEO2_QCDB_HOST lets a network-isolated
// build container reach it through a broker instead; unset, behaviour is
// unchanged.
const std::string QCDB_ENDPOINT = [] {
  const char* host = std::getenv("ALICEO2_QCDB_HOST");
  return std::string((host && *host) ? host : "ali-qcdb-test.cern.ch:8083");
}();

// These tests upload, so this has to be a WRITABLE instance -- ccdb-test by
// default. ALICEO2_CCDB_HOST lets a network-isolated build container reach one
// through a broker instead; unset, behaviour is unchanged.
const std::string CCDB_ENDPOINT = [] {
  const char* host = std::getenv("ALICEO2_CCDB_HOST");
  return std::string((host && *host) ? host : "ccdb-test.cern.ch:8080");
}();

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
    ILOG(Info, Support) << "*** " << Catch::getResultCapture().getCurrentTestName() << " (" << pid
                        << ") ***" << ENDM;
  }

  ~test_fixture() = default;

  // shorthands to get the paths to the objects and their containing folder
  std::string getMoPath(const string& objectName, const string& provenance = "qc", bool includeProvenance = true) const
  {
    return RepoPathUtils::getMoPath(detector, taskName, objectName, provenance, includeProvenance);
  }
  std::string getQoPath(const string& checkName, const string& provenance = "qc", bool includeProvenance = true) const
  {
    return RepoPathUtils::getQoPath(detector, taskName + "/" + checkName, "", {}, provenance, includeProvenance);
  }
  std::string getMoFolder(const string& objectName) const
  {
    // we extract the path after the provenance and before the object name
    string fullMoPath = getMoPath(objectName, "", false);
    return fullMoPath.substr(0, fullMoPath.find_last_of('/'));
  }
  std::string getQoFolder(const string& checkName) const
  {
    // we extract the path after the provenance and before the object name
    string fullQoPath = getQoPath(checkName, "", false);
    return fullQoPath.substr(0, fullQoPath.find_last_of('/'));
  }
  std::unique_ptr<CcdbDatabase> backend;
  map<string, string> metadata;
  std::string pid;
  const std::string detector = "TST";
  std::string taskName;
};

// Cleans up the objects stored by the tests in this file once the whole test run is over
class CcdbDatabaseCleanupListener : public Catch::EventListenerBase
{
 public:
  using Catch::EventListenerBase::EventListenerBase;

  void testRunEnded(Catch::TestRunStats const&) override
  {
    try {
      std::unique_ptr<CcdbDatabase> backend = std::make_unique<CcdbDatabase>();
      backend->connect(CCDB_ENDPOINT, "", "", "");
      backend->truncate("qc/TST/MO/Test/pid" + std::to_string(getpid()), "*");
      backend->truncate("qc/TST/QO/Test/pid" + std::to_string(getpid()), "*");
      backend->truncate("qc/TST/QCFC/Test_pid" + std::to_string(getpid()), "*");
      backend->truncate("qc_hello/TST/MO/Test/pid" + std::to_string(getpid()), "*");
      backend->truncate("qc_hello/TST/QO/Test/pid" + std::to_string(getpid()), "*");
    } catch (...) {
    }
  }
};

// NOTE: the tests below depend on the objects stored by "ccdb_store" and rely on Catch2 running the test cases
// of a file in the order of their declaration, which is true by default (argument --order=decl).
long oldTimestamp;

TEST_CASE("ccdb_store", "[ccdb]")
{
  test_fixture f;

  TH1F* h1 = new TH1F("quarantine", "asdf", 100, 0, 99);
  h1->FillRandom("gaus", 10000);
  shared_ptr<MonitorObject> mo1 = make_shared<MonitorObject>(h1, f.taskName, "TestClass", "TST");
  mo1->updateActivity(1234, "LHC66", "passName1", "qc");

  TH1F* h2 = new TH1F("metadata", "asdf", 100, 0, 99);
  shared_ptr<MonitorObject> mo2 = make_shared<MonitorObject>(h2, f.taskName, "TestClass", "TST");
  mo2->addMetadata("my_meta", "is_good");

  TH1F* h3 = new TH1F("short", "asdf", 100, 0, 99);
  shared_ptr<MonitorObject> mo3 = make_shared<MonitorObject>(h3, f.taskName, "TestClass", "TST");

  TH1F* h4 = new TH1F("provenance", "asdf", 100, 0, 99);
  shared_ptr<MonitorObject> mo4 = make_shared<MonitorObject>(h4, f.taskName, "TestClass", "TST");
  mo4->updateActivity(1234, "LHC66", "passName1", "qc_hello");

  TH1F* h5 = new TH1F("cycle", "asdf", 100, 0, 99);
  shared_ptr<MonitorObject> mo5 = make_shared<MonitorObject>(h5, f.taskName, "TestClass", "TST");
  mo5->addMetadata(metadata_keys::cycleNumber, "1");
  mo5->setValidity({ 10000, 20000 });
  mo5->updateActivity(1234, "LHC66", "passName1", "qc");

  TH1F* h6 = new TH1F("cycle", "asdf", 100, 0, 99);
  shared_ptr<MonitorObject> mo6 = make_shared<MonitorObject>(h6, f.taskName, "TestClass", "TST");
  mo6->addMetadata(metadata_keys::cycleNumber, "2");
  mo6->setValidity({ 10000, 20000 });
  mo6->updateActivity(1234, "LHC66", "passName1", "qc");

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
  f.backend->storeMO(mo5);
  f.backend->storeMO(mo6);

  f.backend->storeQO(qo1);
  f.backend->storeQO(qo2);
  f.backend->storeQO(qo4);

  // with timestamps
  mo3->setValidity({ 10000, 20000 });
  qo3->setValidity({ 10000, 20000 });
  f.backend->storeMO(mo3);
  f.backend->storeQO(qo3);

  // test the max size
  f.backend->setMaxObjectSize(1);
  f.backend->storeMO(mo3); // should fail
}

TEST_CASE("ccdb_store_for_future_tests", "[ccdb]")
{
  // this test is storing a version of the objects in a different directory.
  // The goal is to keep old versions of the objects, in old formats, for future backward compatibility testing.
  test_fixture f;

  TH1F* h1 = new TH1F("to_be_kept", "asdf", 100, 0, 99);
  h1->FillRandom("gaus", 12345);
  shared_ptr<MonitorObject> mo1 = make_shared<MonitorObject>(h1, "TestClass", "task", "TST_KEEP");
  mo1->addMetadata(metadata_keys::runNumber, o2::quality_control::core::Version::GetQcVersion().getString());
  shared_ptr<QualityObject> qo1 = make_shared<QualityObject>(Quality::Bad, "check", "TST_KEEP", "OnAll", vector{ string("input1"), string("input2") });
  qo1->addMetadata(metadata_keys::runNumber, o2::quality_control::core::Version::GetQcVersion().getString());

  f.backend->storeMO(mo1);
  f.backend->storeQO(qo1);
}

TEST_CASE("ccdb_retrieve_mo", "[ccdb]")
{
  test_fixture f;
  std::shared_ptr<MonitorObject> mo = f.backend->retrieveMO(f.getMoFolder("quarantine"), "quarantine");
  REQUIRE(mo != nullptr);
  CHECK(mo->getName() == "quarantine");
  CHECK(mo->getActivity().mId == 1234);
  CHECK(mo->getActivity().mPeriodName == "LHC66");
  CHECK(mo->getActivity().mPassName == "passName1");
  CHECK(mo->getActivity().mProvenance == "qc");
}

TEST_CASE("ccdb_retrieve_timestamps", "[ccdb]")
{
  test_fixture f;

  std::shared_ptr<MonitorObject> mo = f.backend->retrieveMO(f.getMoFolder("short"), "short", 15000);
  REQUIRE(mo != nullptr);
  CHECK(mo->getName() == "short");

  std::shared_ptr<QualityObject> qo = f.backend->retrieveQO(f.getQoPath("short", "", false), 15000);
  REQUIRE(qo != nullptr);
  CHECK(qo->getName() == f.taskName + "/short");

  auto qoValidity = f.backend->getLatestObjectValidity(f.getQoPath("short", "qc", true), {});
  ValidityInterval expectedValidity{ 10000, 20000 };
  CHECK(qoValidity == expectedValidity);
}

TEST_CASE("ccdb_retrieve_inexisting_mo", "[ccdb]")
{
  test_fixture f;

  std::shared_ptr<MonitorObject> mo = f.backend->retrieveMO("non/existing", "object");
  CHECK(mo == nullptr);
}

TEST_CASE("ccdb_retrieve_mo_with_cycle", "[ccdb]")
{
  test_fixture f;
  std::shared_ptr<MonitorObject> mo{};
  mo = f.backend->retrieveMO(f.getMoFolder("cycle"), "cycle",
                             15000, Activity{}, { { metadata_keys::cycleNumber, "1" } });
  REQUIRE(mo.get() != nullptr);
  REQUIRE_NOTHROW(mo->getMetadata(metadata_keys::cycleNumber));
  REQUIRE(mo->getMetadata(metadata_keys::cycleNumber) == "1");

  mo = f.backend->retrieveMO(f.getMoFolder("cycle"), "cycle",
                             15000, Activity{}, { { metadata_keys::cycleNumber, "2" } });
  REQUIRE(mo.get() != nullptr);
  REQUIRE_NOTHROW(mo->getMetadata(metadata_keys::cycleNumber));
  REQUIRE(mo->getMetadata(metadata_keys::cycleNumber) == "2");
}

TEST_CASE("ccdb_retrieve_qo", "[ccdb]")
{
  test_fixture f;
  std::shared_ptr<QualityObject> qo = f.backend->retrieveQO(RepoPathUtils::getQoPath("TST", f.taskName + "/test-ccdb-check", "", {}, "", false));
  REQUIRE(qo != nullptr);
  Quality q = qo->getQuality();
  CHECK(q.getLevel() == 3);
  CHECK(qo->getActivity().mId == 1234);
  CHECK(qo->getActivity().mPeriodName == "LHC66");
  CHECK(qo->getActivity().mPassName == "passName1");
  CHECK(qo->getActivity().mProvenance == "qc");

  qo = f.backend->retrieveQO(RepoPathUtils::getQoPath("TST", f.taskName + "/metadata", "", {}, "", false), repository::CcdbDatabase::Timestamp::Current, {}, { { "my_meta", "is_good" } });
  REQUIRE(qo != nullptr);
  REQUIRE_NOTHROW(qo->getMetadata("my_meta"));
  CHECK(qo->getMetadata("my_meta") == "is_good");

  qo = f.backend->retrieveQO(RepoPathUtils::getQoPath("TST", f.taskName + "/metadata", "", {}, "", false), repository::CcdbDatabase::Timestamp::Current, {}, { { "my_meta", "nonexistent" } });
  REQUIRE(qo == nullptr);
}

TEST_CASE("ccdb_provenance", "[ccdb]")
{
  test_fixture f;
  std::shared_ptr<QualityObject> qo = f.backend->retrieveQO(RepoPathUtils::getQoPath("TST", f.taskName + "/provenance", "", {}, "", false), -1, { 0, "NONE", "", "", "qc_hello" });
  REQUIRE(qo != nullptr);
  CHECK(qo->getActivity().mProvenance == "qc_hello");

  std::shared_ptr<MonitorObject> mo = f.backend->retrieveMO(f.getMoFolder("provenance"), "provenance", -1, { 0, "NONE", "", "", "qc_hello" });
  REQUIRE(mo != nullptr);
  CHECK(mo->getActivity().mProvenance == "qc_hello");
}

unique_ptr<CcdbDatabase> backendGlobal = std::make_unique<CcdbDatabase>();
// Catch2 assertions are not thread-safe, so the worker threads only count the failures and the test case checks them.
std::atomic<int> emptyResultsGlobal{ 0 };

void askObject(std::string objectPath)
{
  cout << "in askObject" << endl;
  map<string, string> metadata;
  map<string, string> headers;
  auto json = backendGlobal->retrieveJson(objectPath, -1, metadata);
  cout << "std::string::max_size(): " << std::string().max_size() << endl;
  cout << "json string size: " << json.size() << endl;
  cout << "object " << json.substr(0, 10) << endl;
  if (json.empty()) {
    emptyResultsGlobal++;
  }
  cout << "finished " << endl;
}

TEST_CASE("ccdb_test_thread", "[ccdb]")
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
  CHECK(emptyResultsGlobal.load() == 0);
}

unique_ptr<o2::ccdb::CcdbApi> apiGlobal = std::make_unique<o2::ccdb::CcdbApi>();
// Catch2 assertions are not thread-safe, so the worker threads only count the failures and the test case checks them.
std::atomic<int> missingObjectsGlobal{ 0 };

void askObjectApi(std::string objectPath)
{
  cout << "in askObject" << endl;
  map<string, string> metadata;
  map<string, string> headers;

  auto* object = apiGlobal->retrieveFromTFileAny<TObject>(objectPath, metadata, -1, &headers);
  if (object == nullptr) {
    missingObjectsGlobal++;
  }
  cout << "finished " << endl;
}

TEST_CASE("ccdb_test_thread_api", "[ccdb]")
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
  CHECK(missingObjectsGlobal.load() == 0);
}

TEST_CASE("ccdb_test_no_thread_api", "[ccdb]")
{
  unique_ptr<o2::ccdb::CcdbApi> api = std::make_unique<o2::ccdb::CcdbApi>();
  string ccdbUrl = "http://" + QCDB_ENDPOINT;
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

TEST_CASE("ccdb_metadata", "[ccdb]")
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
  CHECK(obj1 != nullptr);
  CHECK(obj2 != nullptr);
  CHECK(headers1.size() > 0);
  CHECK(headers2.size() > 1);
  CHECK(headers1.count("my_meta") == 0);
  CHECK(headers2.count("my_meta") == 1);
  CHECK(headers2.at("my_meta") == "is_good");

  // get the path without the objectName because of the interface retrieveMO
  auto obj1a = f.backend->retrieveMO(f.getMoFolder("quarantine"), "quarantine");
  auto obj2a = f.backend->retrieveMO(f.getQoFolder(""), "metadata");
  REQUIRE(obj1a != nullptr);
  REQUIRE(obj2a != nullptr);
  CHECK(obj1a->getMetadataMap().size() > 0);
  CHECK(obj2a->getMetadataMap().size() > 1);
  CHECK(obj1a->getMetadataMap().count("my_meta") == 0);
  CHECK(obj2a->getMetadataMap().count("my_meta") == 1);
  CHECK(obj2a->getMetadataMap().at("my_meta") == "is_good");

  auto obj3 = f.backend->retrieveQO(pathQuality.substr(3));
  auto obj4 = f.backend->retrieveQO(pathQualityMetadata.substr(3));
  REQUIRE(obj3 != nullptr);
  REQUIRE(obj4 != nullptr);
  CHECK(obj3->getMetadataMap().size() > 0);
  CHECK(obj4->getMetadataMap().size() > 1);
  CHECK(obj3->getMetadataMap().count("my_meta") == 0);
  CHECK(obj4->getMetadataMap().count("my_meta") == 1);
  CHECK(obj4->getMetadataMap().at("my_meta") == "is_good");
}

TEST_CASE("ccdb_store_retrieve_any", "[ccdb]")
{
  test_fixture f;

  std::map<std::string, std::string> meta;
  TH1F* h1 = new TH1F("quarantine", "asdf", 100, 0, 99);
  h1->FillRandom("gaus", 10000);

  f.backend->storeAny(h1, typeid(TH1F), f.getMoPath("storeAny"), meta, "TST", "testStoreAny");

  meta.clear();
  void* rawResult = f.backend->retrieveAny(typeid(TH1F), f.getMoPath("storeAny"), meta);
  auto h1_back = static_cast<TH1F*>(rawResult);
  CHECK(rawResult != nullptr);
  CHECK(h1_back != nullptr);
  CHECK(h1_back->GetNbinsX() == 100);
  CHECK(h1_back->GetEntries() > 0);
}

TEST_CASE("ccdb_store_retrieve_latest", "[ccdb]")
{
  test_fixture f;

  TH1F* h1 = new TH1F("latest_test", "latest_test", 100, 0, 99);
  h1->FillRandom("gaus", 10000);
  shared_ptr<MonitorObject> mo1 = make_shared<MonitorObject>(h1, f.taskName, "TestClass", "TST");
  mo1->updateActivity(1234, "LHC66", "passName1", "qc");
  mo1->setValidity({ 30, 50 });
  f.backend->storeMO(mo1);

  h1->FillRandom("gaus", 10000);
  mo1->updateActivity(1234, "LHC66", "passName1", "qc");
  mo1->setValidity({ 10, 30 });
  f.backend->storeMO(mo1); // this is going to be the latest version matching the provided Activity

  h1->FillRandom("gaus", 10000);
  mo1->updateActivity(1234, "LHC66", "differentPassName", "qc");
  mo1->setValidity({ 10, 30 });
  f.backend->storeMO(mo1);

  std::shared_ptr<MonitorObject> moBack = f.backend->retrieveMO(f.getMoFolder("latest_test"), "latest_test", DatabaseInterface::Timestamp::Latest, { 1234, "NONE", "LHC66", "passName1", "qc" });
  REQUIRE(moBack != nullptr);
  auto h1Back = dynamic_cast<TH1F*>(moBack->getObject());
  REQUIRE(h1Back != nullptr);
  CHECK(h1Back->GetEntries() == 20000);
}

} // namespace

CATCH_REGISTER_LISTENER(CcdbDatabaseCleanupListener)

} // namespace o2::quality_control::core
