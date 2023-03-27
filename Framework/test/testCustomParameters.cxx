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
/// \file    testCheck.cxx
/// \author  Rafal Pacholek
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

  cp.set("value", "key");
  BOOST_CHECK_EQUAL(cp.at("key"), "value");
  BOOST_CHECK_EQUAL(cp.at("key", "default"), "value");
  BOOST_CHECK_EQUAL(cp.at("key", "default", "default"), "value");

  cp.set("value_run1", "key", "run1");
  BOOST_CHECK_EQUAL(cp.at("key"), "value");
  BOOST_CHECK_EQUAL(cp.at("key", "run1"), "value_run1");
  BOOST_CHECK_EQUAL(cp.at("key", "run1", "default"), "value_run1");

  cp.set("value_beam1", "key", "default", "beam1");
  BOOST_CHECK_EQUAL(cp.at("key"), "value");
  BOOST_CHECK_EQUAL(cp.at("key", "default"), "value");
  BOOST_CHECK_EQUAL(cp.at("key", "default", "beam1"), "value_beam1");

  cp.set("value_run1_beam1", "key", "run1", "beam1");
  BOOST_CHECK_EQUAL(cp.at("key"), "value");
  BOOST_CHECK_EQUAL(cp.at("key", "run1", "beam1"), "value_run1_beam1");

  cout << cp << endl;
}

BOOST_AUTO_TEST_CASE(test_cp_iterators)
{
  CustomParameters cp;
  cp.set("value1", "key1", "run1", "beam1");
  auto param = cp.find("key1", "run1", "beam1");
  BOOST_CHECK(param != cp.end());
  if (param != cp.end()) {
    BOOST_CHECK_EQUAL(param->second, "value1");
  }

  cp.set("value2", "key2");
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
  cp.set("AAA", "aaa");
  cp.set("BBB", "bbb");
  cp.set("AAA", "aaa", "runX");
  cp.set("AAA", "aaa", "runX", "beamB");
  cp.set("CCC", "ccc");
  cp.set("BBB", "bbb", "runX");
  cp.set("CCC", "ccc", "runY");

  BOOST_CHECK_EQUAL(cp.count("aaa"), 1);
  BOOST_CHECK_EQUAL(cp.count("bbb"), 1);
  BOOST_CHECK_EQUAL(cp.count("aaa"), 1);
  BOOST_CHECK_EQUAL(cp.count("aaa"), 1);
  BOOST_CHECK_EQUAL(cp.at("aaa"), "AAA");
  BOOST_CHECK_EQUAL(cp.at("bbb"), "BBB");
  BOOST_CHECK_EQUAL(cp.at("aaa", "runX"), "AAA");

  BOOST_CHECK_THROW(cp.at("not_found"), std::out_of_range);
  BOOST_CHECK_EQUAL(cp.atOrDefaultValue("not_found", "default_value"), "default_value");
  BOOST_CHECK_EQUAL(cp.atOrDefaultValue("not_found", "asdf", "adsf", "default_value2"), "default_value2");

  BOOST_CHECK_EQUAL(cp["aaa"], "AAA");
  BOOST_CHECK_EQUAL(cp["ccc"], "CCC");
  BOOST_CHECK_THROW(cp["not_found"], std::out_of_range);

  auto all = cp.getAllForRunBeam("runX", "default");
  auto another = cp.getAllForRunBeam("default", "default");
  auto same = cp.getAllDefaults();
  BOOST_CHECK_EQUAL(another.size(), same.size());
  BOOST_CHECK_EQUAL(all.size(), 2);
  BOOST_CHECK_EQUAL(another.size(), 3);
}

BOOST_AUTO_TEST_CASE(test_cp_new_access_pattern)
{
  CustomParameters cp;
  cp.set("AAA", "aaa");
  cp.set("BBB", "bbb");
  cp.set("AAA", "aaa", "runX");
  cp.set("AAA", "aaa", "runX", "beamB");
  cp.set("CCC", "ccc");
  cp.set("BBB", "bbb", "runX");
  cp.set("CCC", "ccc", "runY");

  // if we have a default value
  std::string param = cp.atOrDefaultValue("myOwnKey", "1");
  BOOST_CHECK_EQUAL(std::stoi(param), 1);
  param = cp.atOrDefaultValue("aaa", "1");
  BOOST_CHECK_EQUAL(std::stoi(param), 1);

  // if we don't have a default value and only want to do something if there is a value:
  if (auto param2 = cp.find("aaa"); param2 != cp.end()) {
    BOOST_CHECK_EQUAL(std::stoi(param), 1);
  }
}
