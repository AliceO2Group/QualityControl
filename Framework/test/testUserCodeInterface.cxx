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

#define BOOST_TEST_MODULE CommonInterface test
#define BOOST_TEST_MAIN
#define BOOST_TEST_DYN_LINK

#include <boost/test/unit_test.hpp>
#include <TObject.h>
#include <string>
#include <TROOT.h>
#include <TH1F.h>

#include <QualityControl/CcdbDatabase.h>
#include <QualityControl/CustomParameters.h>

using namespace std;
using namespace o2::quality_control::repository;
using namespace o2::quality_control::core;

namespace o2::quality_control
{

namespace test
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

struct MyGlobalFixture {
  void teardown()
  {
    auto backend = std::make_unique<CcdbDatabase>();
    backend->connect("ccdb-test.cern.ch:8080", "", "", "");
    backend->truncate("qc/TST/MO/Test/pid" + std::to_string(getpid()), "*");
  }
};
BOOST_TEST_GLOBAL_FIXTURE(MyGlobalFixture);

BOOST_AUTO_TEST_CASE(test_invoke_all_methods)
{
  test::TestInterface testInterface;

  BOOST_CHECK_EQUAL(testInterface.configured, false);

  TH1F* h1 = new TH1F("asdf", "asdf", 100, 0, 99);
  auto pid = std::to_string(getpid());
  auto taskName = "Test/pid" + pid;
  shared_ptr<MonitorObject> mo1 = make_shared<MonitorObject>(h1, taskName, "task", "TST");
  auto backend = std::make_unique<CcdbDatabase>();
  backend->connect("ccdb-test.cern.ch:8080", "", "", "");
  backend->storeMO(mo1);

  // setting custom parameters should configure
  CustomParameters customParameters;
  customParameters["test"] = "asdf";
  testInterface.setCustomParameters(customParameters);
  BOOST_CHECK_EQUAL(testInterface.configured, true);
  BOOST_CHECK_EQUAL(testInterface.get("test"), "asdf");

  testInterface.setCcdbUrl("ccdb-test.cern.ch:8080");
  auto obj = testInterface.retrieveConditionAny<TObject>("qc/TST/MO/" + taskName + "/asdf");
  BOOST_CHECK_NE(obj, nullptr);
}
} /* namespace test */
} /* namespace o2::quality_control */