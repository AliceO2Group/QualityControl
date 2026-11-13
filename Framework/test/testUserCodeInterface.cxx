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
/// \file    testCommonInterface.cxx
/// \author  Barthelemy von Haller
///

#include "QualityControl/UserCodeInterface.h"

#include <TObject.h>
#include <string>
#include <TROOT.h>
#include <TH1F.h>

#include <QualityControl/CcdbDatabase.h>
#include <QualityControl/CustomParameters.h>
#include <cstdlib>
#include <unistd.h>
#include <catch_amalgamated.hpp>

using namespace std;
using namespace o2::quality_control::repository;
using namespace o2::quality_control::core;

// A QC-specific CCDB instance: ali-qcdb-test by default. It holds different
// objects from ccdb-test -- see the TODO in testTimekeeper.cxx -- so this is
// deliberately NOT ALICEO2_CCDB_HOST. ALICEO2_QCDB_HOST lets a network-isolated
// build container reach it through a broker instead; unset, behaviour is
// unchanged.
const std::string QCDB_ENDPOINT = [] {
  const char* host = std::getenv("ALICEO2_QCDB_HOST");
  return std::string((host && *host) ? host : "ali-qcdb-test.cern.ch:8083");
}();

namespace o2::quality_control
{

namespace test
{

// anonymous namespace to keep this class symbols visible only to this file (instead of all files combined into o2-qc-test-ccdb)
namespace
{
class TestInterface : public core::UserCodeInterface
{
 public:
  /// Default constructor
  TestInterface() = default;
  /// Destructor
  ~TestInterface() override = default;

  // Override interface
  void configure() override
  {
    configured = true;
  }

  string get(std::string key)
  {
    return mCustomParameters.at(key);
  }

  bool configured = false;
};

// Cleans up the objects stored by the tests in this file once the whole test run is over
class UserCodeInterfaceCleanupListener : public Catch::EventListenerBase
{
 public:
  using Catch::EventListenerBase::EventListenerBase;

  void testRunEnded(Catch::TestRunStats const&) override
  {
    try {
      auto backend = std::make_unique<CcdbDatabase>();
      backend->connect(QCDB_ENDPOINT, "", "", "");
      backend->truncate("qc/TST/MO/Test/pid" + std::to_string(getpid()), "*");
    } catch (...) {
    }
  }
};
} // namespace

CATCH_REGISTER_LISTENER(UserCodeInterfaceCleanupListener)

TEST_CASE("UserCodeInterface_test_invoke_all_methods", "[ccdb]")
{
  test::TestInterface testInterface;

  CHECK(testInterface.configured == false);

  TH1F* h1 = new TH1F("asdf", "asdf", 100, 0, 99);
  auto pid = std::to_string(getpid());
  auto taskName = "Test/pid" + pid;
  shared_ptr<MonitorObject> mo1 = make_shared<MonitorObject>(h1, taskName, "task", "TST");
  // explicit validity, so that the object can be retrieved with an explicit timestamp below
  mo1->setValidity({ 10000, 20000 });
  auto backend = std::make_unique<CcdbDatabase>();
  backend->connect(QCDB_ENDPOINT, "", "", "");
  backend->storeMO(mo1);

  // setting custom parameters should configure
  CustomParameters customParameters;
  customParameters["test"] = "asdf";
  testInterface.setCustomParameters(customParameters);
  CHECK(testInterface.configured == true);
  CHECK(testInterface.get("test") == "asdf");

  testInterface.setCcdbUrl(QCDB_ENDPOINT);
  auto obj = testInterface.retrieveConditionAny<TObject>("qc/TST/MO/" + taskName + "/asdf", {}, 15000);
  CHECK(obj != nullptr);
}
} /* namespace test */
} /* namespace o2::quality_control */