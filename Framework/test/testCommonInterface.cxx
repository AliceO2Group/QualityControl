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

#include "QualityControl/CommonInterface.h"

#define BOOST_TEST_MODULE CommonInterface test
#define BOOST_TEST_MAIN
#define BOOST_TEST_DYN_LINK

#include <boost/test/unit_test.hpp>
#include <TObject.h>
#include <string>
#include <TROOT.h>

using namespace std;

namespace o2::quality_control
{

namespace test
{

class TestInterface : public core::CommonInterface
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
    return mCustomParameters[key];
  }

  bool configured = false;
};

BOOST_AUTO_TEST_CASE(test_invoke_all_methods)
{
  test::TestInterface testInterface;

  BOOST_CHECK_EQUAL(testInterface.configured, false);

  // setting custom parameters should configure
  std::unordered_map<std::string, std::string> customParameters;
  customParameters["test"] = "asdf";
  testInterface.setCustomParameters(customParameters);
  BOOST_CHECK_EQUAL(testInterface.configured, true);
  BOOST_CHECK_EQUAL(testInterface.get("test"), "asdf");

  testInterface.setCcdbUrl("ccdb-test.cern.ch:8080");
  auto obj = testInterface.retrieveConditionAny<TObject>("qc/TST/MO/QcTask/example");
  BOOST_CHECK_NE(obj, nullptr);
}
} /* namespace test */
} /* namespace o2::quality_control */