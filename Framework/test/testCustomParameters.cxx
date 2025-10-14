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
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include "getTestDataDirectory.h"
#include <iostream>
#include <catch_amalgamated.hpp>

using namespace o2::quality_control::core;
using namespace std;

TEST_CASE("test_cp_basic")
{
  CustomParameters cp;

  cp.set("key", "value");
  CHECK(cp.at("key") == "value");
  CHECK(cp.at("key", "default") == "value");
  CHECK(cp.at("key", "default", "default") == "value");

  cp.set("key", "value_run1", "run1");
  CHECK(cp.at("key") == "value");
  CHECK(cp.at("key", "run1") == "value_run1");
  CHECK(cp.at("key", "run1", "default") == "value_run1");

  cp.set("key", "value_beam1", "default", "beam1");
  CHECK(cp.at("key") == "value");
  CHECK(cp.at("key", "default") == "value");
  CHECK(cp.at("key", "default", "beam1") == "value_beam1");

  cp.set("key", "value_run1_beam1", "run1", "beam1");
  CHECK(cp.at("key") == "value");
  CHECK(cp.at("key", "run1", "beam1") == "value_run1_beam1");

  CHECK_NOTHROW(cout << cp << endl);
}

TEST_CASE("test_cp_iterators")
{
  CustomParameters cp;
  cp.set("key1", "value1", "run1", "beam1");
  auto param = cp.find("key1", "run1", "beam1");
  CHECK(param != cp.end());
  if (param != cp.end()) {
    CHECK(param->second == "value1");
  }

  cp.set("key2", "value2");
  param = cp.find("key2");
  CHECK(param != cp.end());
  if (param != cp.end()) {
    CHECK(param->second == "value2");
  }

  param = cp.find("not_found");
  CHECK(param == cp.end());

  param = cp.find("not_found", "run1");
  CHECK(param == cp.end());

  param = cp.find("not_found", "run1", "beam1");
  CHECK(param == cp.end());
}

TEST_CASE("test_cp_misc")
{
  CustomParameters cp;
  cp.set("aaa", "AAA");
  cp.set("bbb", "BBB");
  cp.set("aaa", "AAA", "runX");
  cp.set("aaa", "AAA", "runX", "beamB");
  cp.set("ccc", "CCC");
  cp.set("bbb", "BBB", "runX");
  cp.set("ccc", "CCC", "runY");

  CHECK(cp.count("aaa") == 1);
  CHECK(cp.count("bbb") == 1);
  CHECK(cp.count("aaa") == 1);
  CHECK(cp.count("aaa") == 1);
  CHECK(cp.at("aaa") == "AAA");
  CHECK(cp.at("bbb") == "BBB");
  CHECK(cp.at("aaa", "runX") == "AAA");

  CHECK_THROWS_AS(cp.at("not_found"), std::out_of_range);
  CHECK(cp.atOrDefaultValue("not_found", "default_value") == "default_value");
  CHECK(cp.atOrDefaultValue("not_found") == "");
  CHECK(cp.atOrDefaultValue("not_found", "default_value2", "asdf", "adsf") == "default_value2");

  CHECK(cp["aaa"] == "AAA");
  CHECK(cp["ccc"] == "CCC");

  auto all = cp.getAllForRunBeam("runX", "default");
  auto another = cp.getAllForRunBeam("default", "default");
  auto same = cp.getAllDefaults();
  CHECK(another.size() == same.size());
  CHECK(all.size() == 2);
  CHECK(another.size() == 3);

  CHECK_NOTHROW(cp["not_found"]); // this won't throw, it will create an empty value at "not_found"
  CHECK(cp.count("not_found") == 1);
  CHECK(cp.at("not_found") == "");

  // test bracket assignemnt
  cp["something"] = "else";
  CHECK(cp.at("something") == "else");
  cp["something"] = "asdf";
  CHECK(cp.at("something") == "asdf");
}

TEST_CASE("test_at_optional")
{
  CustomParameters cp;
  cp.set("aaa", "AAA");
  cp.set("bbb", "BBB");
  cp.set("aaa", "AAA", "runX");
  cp.set("aaa", "AAA", "runX", "beamB");

  CHECK(cp.atOptional("aaa").value() == "AAA");
  CHECK(cp.atOptional("abc").has_value() == false);
  CHECK(cp.atOptional("abc").value_or("bla") == "bla");
}

TEST_CASE("test_at_optional_activity")
{
  Activity activity;
  activity.mBeamType = "pp";
  activity.mType = "PHYSICS";

  CustomParameters cp;
  cp.set("aaa", "AAA");
  cp.set("bbb", "BBB");
  cp.set("aaa", "asdf", "PHYSICS");
  cp.set("aaa", "CCC", "PHYSICS", "pp");
  cp.set("aaa", "DDD", "PHYSICS", "PbPb");
  cp.set("aaa", "AAA", "TECHNICAL", "pp");

  CHECK(cp.atOptional("aaa", activity).value() == "CCC");
  CHECK(cp.atOptional("abc", activity).has_value() == false);
  CHECK(cp.atOptional("abc", activity).value_or("bla") == "bla");

  Activity activity2;
  activity.mBeamType = "PbPb";
  activity.mType = "PHYSICS";
  CHECK(cp.atOptional("aaa", activity).value() == "DDD");
}

TEST_CASE("test_cp_new_access_pattern")
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
  CHECK(std::stoi(param) == 1);
  param = cp.atOrDefaultValue("aaa", "1");
  CHECK(param == "AAA");

  // if we don't have a default value and only want to do something if there is a value:
  if (auto param2 = cp.find("ccc"); param2 != cp.end()) {
    CHECK(std::stoi(param2->second) == 1);
  }
}

TEST_CASE("test_load_from_ptree")
{
  boost::property_tree::ptree jsontree;
  std::string configFilePath = std::string(getTestDataDirectory()) + "testWorkflow.json";

  boost::property_tree::read_json(configFilePath, jsontree);

  string v0 = jsontree.get<string>("qc.tasks.skeletonTask.extendedTaskParameters.default.default.myOwnKey");

  boost::property_tree::ptree params = jsontree.get_child("qc.tasks.skeletonTask.extendedTaskParameters");

  CustomParameters cp;
  cp.populateCustomParameters(params);

  CHECK_NOTHROW(cout << cp << endl);

  CHECK(cp.at("myOwnKey") == "myOwnValue");
  CHECK(cp.at("myOwnKey1", "PHYSICS") == "myOwnValue1b");
  CHECK(cp.atOptional("asdf").has_value() == false);
}

TEST_CASE("test_default_if_not_found_at_optional")
{
  CustomParameters cp;

  // no default values are in the CP, we get an empty result
  CHECK(cp.atOptional("key", "PHYSICS", "proton-proton").has_value() == false);
  CHECK(cp.atOptional("key", "TECHNICAL", "proton-proton").has_value() == false);

  // prepare the CP
  cp.set("key", "valueDefaultDefault", "default", "default");
  cp.set("key", "valuePhysicsDefault", "PHYSICS", "default");
  cp.set("key", "valuePhysicsPbPb", "PHYSICS", "PbPb");
  cp.set("key", "valueCosmicsDefault", "COSMICS", "default");
  cp.set("key", "valueCosmicsDefault", "default", "pp");

  // check the data
  CHECK(cp.atOptional("key").value() == "valueDefaultDefault");
  CHECK(cp.atOptional("key", "PHYSICS").value() == "valuePhysicsDefault");
  CHECK(cp.atOptional("key", "PHYSICS", "PbPb").value() == "valuePhysicsPbPb");
  CHECK(cp.atOptional("key", "COSMICS", "default").value() == "valueCosmicsDefault");
  CHECK(cp.atOptional("key", "default", "pp").value() == "valueCosmicsDefault");

  // check when something is missing
  CHECK(cp.atOptional("key", "PHYSICS", "pp").value() == "valuePhysicsDefault");        // key is not defined for pp
  CHECK(cp.atOptional("key", "TECHNICAL", "STRANGE").value() == "valueDefaultDefault"); // key is not defined for run nor beam
  CHECK(cp.atOptional("key", "TECHNICAL", "pp").value() == "valueCosmicsDefault");      // key is not defined for technical
}

TEST_CASE("test_default_if_not_found_at")
{
  CustomParameters cp;

  // no default values are in the CP, we get an empty result
  CHECK_THROWS_AS(cp.at("key", "PHYSICS", "proton-proton"), std::out_of_range);
  CHECK_THROWS_AS(cp.at("key", "TECHNICAL", "proton-proton"), std::out_of_range);

  // prepare the CP
  cp.set("key", "valueDefaultDefault", "default", "default");
  cp.set("key", "valuePhysicsDefault", "PHYSICS", "default");
  cp.set("key", "valuePhysicsPbPb", "PHYSICS", "PbPb");
  cp.set("key", "valueCosmicsDefault", "COSMICS", "default");
  cp.set("key", "valueCosmicsDefault", "default", "pp");

  // check the data
  CHECK(cp.at("key") == "valueDefaultDefault");
  CHECK(cp.at("key", "PHYSICS") == "valuePhysicsDefault");
  CHECK(cp.at("key", "PHYSICS", "PbPb") == "valuePhysicsPbPb");
  CHECK(cp.at("key", "COSMICS", "default") == "valueCosmicsDefault");
  CHECK(cp.at("key", "default", "pp") == "valueCosmicsDefault");

  // check when something is missing
  CHECK(cp.at("key", "PHYSICS", "pp") == "valuePhysicsDefault");        // key is not defined for pp
  CHECK(cp.at("key", "TECHNICAL", "STRANGE") == "valueDefaultDefault"); // key is not defined for run nor beam
  CHECK(cp.at("key", "TECHNICAL", "pp") == "valueCosmicsDefault");      // key is not defined for technical
}

TEST_CASE("test_getAllDefaults")
{
  CustomParameters cp;
  auto result = cp.getAllDefaults();
  CHECK(result.size() == 0);
}

TEST_CASE("test_getOptionalPtree")
{
  CustomParameters cp;
  std::string content = R""""(
[
          {
            "name": "mean_of_histogram",
            "title": "Mean trend of the example histogram",
            "graphAxisLabel": "Mean X:time",
            "graphYRange": "0:10000",
            "graphs" : [
              {
                "name": "mean_trend",
                "title": "mean trend",
                "varexp": "example.mean:time",
                "selection": "",
                "option": "*L PLC PMC"
              }, {
                "name": "mean_trend_1000",
                "title": "mean trend + 1000",
                "varexp": "example.mean + 1000:time",
                "selection": "",
                "option": "* PMC",
                "graphErrors": "1:200"
              }
            ]
          },
          {
            "name": "histogram_of_means",
            "title": "Distribution of mean values in the example histogram",
            "graphs" : [{
                "varexp": "example.mean",
                "selection": "",
                "option": ""
              }]
          },
          {
            "name": "example_quality",
            "title": "Trend of the example histogram's quality",
            "graphs" : [{
              "varexp": "QcCheck.name:time",
              "selection": "",
              "option": "*"
            }]
          }
        ]
  )"""";
  cp.set("key", content);
  auto pt = cp.getOptionalPtree("key");
  CHECK(pt.has_value());

  std::size_t number_plots = std::distance(pt->begin(), pt->end());
  CHECK(number_plots == 3);

  auto first_plot = pt->begin()->second;
  CHECK(first_plot.get<string>("name") == "mean_of_histogram");
  auto graphs = first_plot.get_child("graphs");
  CHECK(graphs.size() == 2);

  auto last_plot = std::prev(pt->end())->second;
  CHECK(last_plot.get<string>("name") == "example_quality");

  // test for failure
  CustomParameters cp2;
  cp2.set("key", "blabla");
  auto pt2 = cp2.getOptionalPtree("key");
  CHECK(!pt2.has_value());

  // try to get it as text
  auto text = cp.atOptional("key");
  CHECK(text.has_value());
  CHECK(text == content);
}