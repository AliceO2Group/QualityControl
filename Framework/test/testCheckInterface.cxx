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
/// \file    testCheckInterface.cxx
/// \author  Piotr Konopka
///

#include "QualityControl/CheckInterface.h"

#define BOOST_TEST_MODULE CheckInterface test
#define BOOST_TEST_MAIN
#define BOOST_TEST_DYN_LINK

#include <boost/test/unit_test.hpp>
#include <iostream>
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
  Quality check(const MonitorObject* mo) override
  {
    if (mValidString.empty()) {
      return Quality::Null;
    }

    TObjString* str = reinterpret_cast<TObjString*>(mo->getObject());
    return mValidString == str->String() ? Quality::Good : Quality::Bad;
  }

  void beautify(MonitorObject* mo, Quality = Quality::Null) override
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

  MonitorObject mo(new TObjString("A string"), "str");

  BOOST_CHECK_EQUAL(testCheck.check(&mo), Quality::Null);

  testCheck.configure("A different string");
  BOOST_CHECK_EQUAL(testCheck.check(&mo), Quality::Bad);

  testCheck.configure("A string");
  BOOST_CHECK_EQUAL(testCheck.check(&mo), Quality::Good);

  testCheck.beautify(&mo);
  BOOST_CHECK_EQUAL(reinterpret_cast<TObjString*>(mo.getObject())->String(), "A string is beautiful now");

  BOOST_CHECK_EQUAL(testCheck.getAcceptedType(), "TObjString");
}