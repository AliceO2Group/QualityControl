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
/// \file    testCheckInterface.cxx
/// \author  Piotr Konopka
///

#include "QualityControl/CheckInterface.h"

#define BOOST_TEST_MODULE CheckInterface test
#define BOOST_TEST_MAIN
#define BOOST_TEST_DYN_LINK

#include <boost/test/unit_test.hpp>
#include <TObjString.h>
#include <string>

using namespace o2::quality_control;
using namespace std;

namespace o2::quality_control
{

using namespace core;

namespace test
{

class TestCheck : public checker::CheckInterface
{
 public:
  /// Default constructor
  TestCheck() = default;
  /// Destructor
  ~TestCheck() override = default;

  // Override interface
  void configure(std::string name) override
  {
    mValidString = name;
  }
  
  Quality check(std::map<std::string, std::shared_ptr<MonitorObject>>* moMap) override
  {
    auto mo = moMap->begin()->second;
    if (mValidString.empty()) {
      return Quality::Null;
    }

    TObjString* str = reinterpret_cast<TObjString*>(mo->getObject());
    return mValidString == str->String() ? Quality::Good : Quality::Bad;
  }

  void beautify(std::shared_ptr<MonitorObject> mo, Quality = Quality::Null) override
  {
    TObjString* str = reinterpret_cast<TObjString*>(mo->getObject());
    str->String().Append(" is beautiful now");
  }

  std::string getAcceptedType() override
  {
    return "TObjString";
  }

  string mValidString;
};

} /* namespace test */
} /* namespace o2::quality_control */

BOOST_AUTO_TEST_CASE(test_invoke_all_methods)
{
  test::TestCheck testCheck;

  std::shared_ptr<MonitorObject> mo(new MonitorObject(new TObjString("A string"), "str", "class"));
  std::map<std::string, std::shared_ptr<MonitorObject>> moMap = { { "test", mo } };

  BOOST_CHECK_EQUAL(testCheck.check(&moMap), Quality::Null);

  testCheck.configure("A different string");
  BOOST_CHECK_EQUAL(testCheck.check(&moMap), Quality::Bad);

  testCheck.configure("A string");
  BOOST_CHECK_EQUAL(testCheck.check(&moMap), Quality::Good);

  testCheck.beautify(mo);
  BOOST_CHECK_EQUAL(reinterpret_cast<TObjString*>(mo->getObject())->String(), "A string is beautiful now");

  BOOST_CHECK_EQUAL(testCheck.getAcceptedType(), "TObjString");
}
