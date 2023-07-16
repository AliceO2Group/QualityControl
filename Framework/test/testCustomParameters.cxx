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
/// \file    testCustomParameters.cxx
/// \author  Barthelemy von Haller
///

#include "QualityControl/CustomParameters.h"

#define BOOST_TEST_MODULE Check test
#define BOOST_TEST_MAIN
#define BOOST_TEST_DYN_LINK

#include <boost/test/unit_test.hpp>

using namespace o2::quality_control::core;
using namespace std;

BOOST_AUTO_TEST_CASE(test_cp_basic)
{
  CustomParameters cp;

  cp.set("key", "value");
  BOOST_CHECK_EQUAL(cp.at("key"), "value");
  BOOST_CHECK_EQUAL(cp.at("key", "default"), "value");
  BOOST_CHECK_EQUAL(cp.at("key", "default", "default"), "value");

  cp.set("key", "value_run1", "run1");
  BOOST_CHECK_EQUAL(cp.at("key"), "value");
  BOOST_CHECK_EQUAL(cp.at("key", "run1"), "value_run1");
  BOOST_CHECK_EQUAL(cp.at("key", "run1", "default"), "value_run1");

  cp.set("key", "value_beam1", "default", "beam1");
  BOOST_CHECK_EQUAL(cp.at("key"), "value");
  BOOST_CHECK_EQUAL(cp.at("key", "default"), "value");
  BOOST_CHECK_EQUAL(cp.at("key", "default", "beam1"), "value_beam1");

  cp.set("key", "value_run1_beam1", "run1", "beam1");
  BOOST_CHECK_EQUAL(cp.at("key"), "value");
  BOOST_CHECK_EQUAL(cp.at("key", "run1", "beam1"), "value_run1_beam1");

  cout << cp << endl;
}

BOOST_AUTO_TEST_CASE(test_cp_iterators)
{
  CustomParameters cp;
  cp.set("key1", "value1", "run1", "beam1");
  auto param = cp.find("key1", "run1", "beam1");
  BOOST_CHECK(param != cp.end());
  if (param != cp.end()) {
    BOOST_CHECK_EQUAL(param->second, "value1");
  }

  cp.set("key2", "value2");
  param = cp.find("key2");
  BOOST_CHECK(param != cp.end());
  if (param != cp.end()) {
    BOOST_CHECK_EQUAL(param->second, "value2");
  }

  param = cp.find("not_found");
  BOOST_CHECK(param == cp.end());

  param = cp.find("not_found", "run1");
  BOOST_CHECK(param == cp.end());

  param = cp.find("not_found", "run1", "beam1");
  BOOST_CHECK(param == cp.end());
}

BOOST_AUTO_TEST_CASE(test_cp_misc)
{
  CustomParameters cp;
  cp.set("aaa", "AAA");
  cp.set("bbb", "BBB");
  cp.set("aaa", "AAA", "runX");
  cp.set("aaa", "AAA", "runX", "beamB");
  cp.set("ccc", "CCC");
  cp.set("bbb", "BBB", "runX");
  cp.set("ccc", "CCC", "runY");

  BOOST_CHECK_EQUAL(cp.count("aaa"), 1);
  BOOST_CHECK_EQUAL(cp.count("bbb"), 1);
  BOOST_CHECK_EQUAL(cp.count("aaa"), 1);
  BOOST_CHECK_EQUAL(cp.count("aaa"), 1);
  BOOST_CHECK_EQUAL(cp.at("aaa"), "AAA");
  BOOST_CHECK_EQUAL(cp.at("bbb"), "BBB");
  BOOST_CHECK_EQUAL(cp.at("aaa", "runX"), "AAA");

  BOOST_CHECK_THROW(cp.at("not_found"), std::out_of_range);
  BOOST_CHECK_EQUAL(cp.atOrDefaultValue("not_found", "default_value"), "default_value");
  BOOST_CHECK_EQUAL(cp.atOrDefaultValue("not_found"), "");
  BOOST_CHECK_EQUAL(cp.atOrDefaultValue("not_found", "default_value2", "asdf", "adsf"), "default_value2");

  BOOST_CHECK_EQUAL(cp["aaa"], "AAA");
  BOOST_CHECK_EQUAL(cp["ccc"], "CCC");

  auto all = cp.getAllForRunBeam("runX", "default");
  auto another = cp.getAllForRunBeam("default", "default");
  auto same = cp.getAllDefaults();
  BOOST_CHECK_EQUAL(another.size(), same.size());
  BOOST_CHECK_EQUAL(all.size(), 2);
  BOOST_CHECK_EQUAL(another.size(), 3);

  BOOST_CHECK_NO_THROW(cp["not_found"]); // this won't throw, it will create an empty value at "not_found"
  BOOST_CHECK_EQUAL(cp.count("not_found"), 1);
  BOOST_CHECK_EQUAL(cp.at("not_found"), "");

  // test bracket assignemnt
  cp["something"] = "else";
  BOOST_CHECK_EQUAL(cp.at("something"), "else");
  cp["something"] = "asdf";
  BOOST_CHECK_EQUAL(cp.at("something"), "asdf");
}

BOOST_AUTO_TEST_CASE(test_at_optional)
{
  CustomParameters cp;
  cp.set("aaa", "AAA");
  cp.set("bbb", "BBB");
  cp.set("aaa", "AAA", "runX");
  cp.set("aaa", "AAA", "runX", "beamB");

  BOOST_CHECK_EQUAL(cp.atOptional("aaa").value(), "AAA");
  BOOST_CHECK_EQUAL(cp.atOptional("abc").has_value(), false);
  BOOST_CHECK_EQUAL(cp.atOptional("abc").value_or("bla"), "bla");
}

BOOST_AUTO_TEST_CASE(test_at_optional_activity)
{
  Activity activity;
  activity.mBeamType = "PROTON-PROTON";
  activity.mType = 1;

  CustomParameters cp;
  cp.set("aaa", "AAA");
  cp.set("bbb", "BBB");
  cp.set("aaa", "asdf", "PHYSICS");
  cp.set("aaa", "CCC", "PHYSICS", "PROTON-PROTON");
  cp.set("aaa", "DDD", "PHYSICS", "Pb-Pb");
  cp.set("aaa", "AAA", "TECHNICAL", "PROTON-PROTON");

  BOOST_CHECK_EQUAL(cp.atOptional("aaa", activity).value(), "CCC");
  BOOST_CHECK_EQUAL(cp.atOptional("abc", activity).has_value(), false);
  BOOST_CHECK_EQUAL(cp.atOptional("abc", activity).value_or("bla"), "bla");

  Activity activity2;
  activity.mBeamType = "Pb-Pb";
  activity.mType = 1;
  BOOST_CHECK_EQUAL(cp.atOptional("aaa", activity).value(), "DDD");
}

BOOST_AUTO_TEST_CASE(test_cp_new_access_pattern)
{
  CustomParameters cp;
  cp.set("aaa", "AAA");
  cp.set("bbb", "BBB");
  cp.set("aaa", "AAA", "runX");
  cp.set("aaa", "AAA", "runX", "beamB");
  cp.set("ccc", "1");
  cp.set("bbb", "BBB", "runX");
  cp.set("ccc", "CCC", "runY");

  // if we have a default value
  std::string param = cp.atOrDefaultValue("myOwnKey", "1");
  BOOST_CHECK_EQUAL(std::stoi(param), 1);
  param = cp.atOrDefaultValue("aaa", "1");
  BOOST_CHECK_EQUAL(param, "AAA");

  // if we don't have a default value and only want to do something if there is a value:
  if (auto param2 = cp.find("ccc"); param2 != cp.end()) {
    BOOST_CHECK_EQUAL(std::stoi(param2->second), 1);
  }
}
