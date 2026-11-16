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
/// \file   testCcdbDatabaseExtra.cxx
/// \author Adam Wegrzynek
/// \author Barthelemy von Haller
///

#include "QualityControl/DatabaseFactory.h"
#include <unordered_map>
#include "QualityControl/CcdbDatabase.h"
#include "QualityControl/QcInfoLogger.h"

#include <TH1F.h>
#include <cstdlib>
#include <catch_amalgamated.hpp>

namespace o2::quality_control::core
{

// anonymous namespace to keep these symbols visible only to this file (instead of all files combined into o2-qc-test-ccdb)
namespace
{

using namespace o2::quality_control::core;
using namespace o2::quality_control::repository;
using namespace std;

// These tests upload, so this has to be a WRITABLE instance -- ccdb-test by
// default. ALICEO2_CCDB_HOST lets a network-isolated build container reach one
// through a broker instead; unset, behaviour is unchanged.
const std::string CCDB_ENDPOINT = [] {
  const char* host = std::getenv("ALICEO2_CCDB_HOST");
  return std::string((host && *host) ? host : "ccdb-test.cern.ch:8080");
}();
std::unordered_map<std::string, std::string> Objects;

/**
 * Fixture for the tests, i.e. code is ran in every test that uses it, i.e. it is like a setup and teardown for tests.
 */
struct test_fixture {
  test_fixture()
  {
    backend = DatabaseFactory::create("CCDB");
    backend->connect(CCDB_ENDPOINT, "", "", "");
    ILOG(Info, Support) << "*** " << Catch::getResultCapture().getCurrentTestName() << " ***" << ENDM;
  }

  ~test_fixture() = default;

  std::unique_ptr<DatabaseInterface> backend;
  map<string, string> metadata;
};

// These tests should not be executed automatically, too much error prone.
// It depends on what is in the database.
// They are hidden (tag "[ccdb][.manual]") and have to be requested explicitly:
// o2-qc-test-ccdb [ccdb][.manual]

TEST_CASE("ccdb_retrieve_all", "[ccdb][.manual]")
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

long oldTimestamp;

TEST_CASE("ccdb_extra_store", "[ccdb][.manual]")
{
  test_fixture f;

  TH1F* h1 = new TH1F("asdf/asdf", "asdf", 100, 0, 99);
  h1->FillRandom("gaus", 10000);
  shared_ptr<MonitorObject> mo1 = make_shared<MonitorObject>(h1, "my/task", "TestClass", "TST");
  oldTimestamp = CcdbDatabase::getCurrentTimestamp();
  f.backend->storeMO(mo1);

  shared_ptr<QualityObject> qo = make_shared<QualityObject>(Quality::Bad, "checkName", "TST", "OnAll");
  f.backend->storeQO(qo);
}

TEST_CASE("ccdb_retrieve_former_versions", "[ccdb][.manual]")
{
  // store a new object
  test_fixture f;
  TH1F* h1 = new TH1F("asdf/asdf", "asdf", 100, 0, 99);
  h1->FillRandom("gaus", 10001);
  shared_ptr<MonitorObject> mo1 = make_shared<MonitorObject>(h1, "my/task", "TestClass", "TST");
  f.backend->storeMO(mo1);

  // Retrieve old object stored at timestampStorage
  std::shared_ptr<MonitorObject> mo = f.backend->retrieveMO("TST/MO/my/task", "asdf/asdf", oldTimestamp);
  CHECK(mo);
  TH1F* old = dynamic_cast<TH1F*>(mo->getObject());
  CHECK(old != nullptr);
  CHECK(old->GetEntries() == 10000);

  // Retrieve latest object with timestamp
  std::shared_ptr<MonitorObject> mo2 = f.backend->retrieveMO("TST/MO/my/task", "asdf/asdf", CcdbDatabase::getCurrentTimestamp());
  CHECK(mo2);
  TH1F* latest = dynamic_cast<TH1F*>(mo2->getObject());
  CHECK(latest != nullptr);
  CHECK(latest->GetEntries() == 10001);

  // Retrieve latest object without timetsamp
  std::shared_ptr<MonitorObject> mo3 = f.backend->retrieveMO("TST/MO/my/task", "asdf/asdf");
  CHECK(mo3);
  TH1F* latest2 = dynamic_cast<TH1F*>(mo3->getObject());
  CHECK(latest2 != nullptr);
  CHECK(latest2->GetEntries() == 10001);
}

TEST_CASE("ccdb_getobjects_name", "[ccdb][.manual]")
{
  test_fixture f;

  CcdbDatabase* ccdb = static_cast<CcdbDatabase*>(f.backend.get());
  ILOG(Info, Support) << "getListing()" << ENDM;
  auto tasks = ccdb->getListing("/qc");
  CHECK(tasks.size() > 0); // we know that there are a few
  // print but only for TST
  auto objects = f.backend->getPublishedObjectNames("/qc/TST");
  CHECK(objects.size() > 0);
}
} // namespace
} // namespace o2::quality_control::core
