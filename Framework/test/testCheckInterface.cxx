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
#include <TObjString.h>
#include "QualityControl/MonitorObject.h"
#include <string>
#include <catch_amalgamated.hpp>

using namespace o2::quality_control;
using namespace o2::quality_control::core;
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
  void configure() override
  {
  }
  
  Quality check(std::map<std::string, std::shared_ptr<MonitorObject>>* moMap) override
  {
    auto mo = moMap->begin()->second;
    if (mValidString.empty()) {
      return Quality::Null;
    }

    auto* str = reinterpret_cast<TObjString*>(mo->getObject());
    return mValidString == str->String() ? Quality::Good : Quality::Bad;
  }

  void beautify(std::shared_ptr<MonitorObject> mo, Quality = Quality::Null) override
  {
    auto* str = reinterpret_cast<TObjString*>(mo->getObject());
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

TEST_CASE("test_invoke_all_interface_methods")
{
  test::TestCheck testCheck;

  std::shared_ptr<MonitorObject> mo(new MonitorObject(new TObjString("A string"), "str", "class", "DET"));
  std::map<std::string, std::shared_ptr<MonitorObject>> moMap = { { "test", mo } };

  CHECK(testCheck.check(&moMap) == Quality::Null);

  testCheck.mValidString = "A different string";
  CHECK(testCheck.check(&moMap) == Quality::Bad);

  testCheck.mValidString = "A string";
  CHECK(testCheck.check(&moMap) == Quality::Good);

  testCheck.beautify(mo);
  CHECK(reinterpret_cast<TObjString*>(mo->getObject())->String() == "A string is beautiful now");

  CHECK(testCheck.getAcceptedType() == "TObjString");
}
