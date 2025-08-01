// Copyright 2025 CERN and copyright holders of ALICE O2.
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
/// \file    testData.cxx
/// \author  Michal Tichak
///

#include <catch_amalgamated.hpp>
#include "QualityControl/Data.h"

using namespace o2::quality_control::core;

TEST_CASE("Data constructor", "[Data]")
{
  REQUIRE_NOTHROW([]() { Data data{}; });
}

TEST_CASE("Data insert and get", "[Data]")
{
  Data data;
  data.insert("test", 1);
  auto valueStr = data.get<std::string>("test");
  REQUIRE(!valueStr.has_value());
  auto valueInt = data.get<int>("test");
  REQUIRE(valueInt.has_value());
  REQUIRE(valueInt.value() == 1);
}

TEST_CASE("Data getAllOfType", "[Data]")
{
  Data data;
  data.insert("int1", 1);
  data.insert("string", std::string{ "1" });
  data.insert("int2", 1);
  data.insert("long", 1l);

  const auto ints = data.getAllOfType<int>();
  REQUIRE(ints.size() == 2);
  REQUIRE(ints[0] == 1);
  REQUIRE(ints[1] == 1);

  const auto strings = data.getAllOfType<std::string>();
  REQUIRE(strings.size() == 1);
  REQUIRE(strings[0] == "1");

  const auto longs = data.getAllOfType<long>();
  REQUIRE(longs.size() == 1);
  REQUIRE(longs[0] == 1u);

  struct nonexistent {
  };

  const auto nonexistens = data.getAllOfType<nonexistent>();
  REQUIRE(nonexistens.empty());
}

template <typename T>
struct named {
  std::string name;
  T val;
};

TEST_CASE("Data getAllOfTypeIf", "[Data]")
{
  Data data;
  data.insert("1", named{ "1", 4 });
  data.insert("1", named{ "1", 4l });
  auto filtered = data.getAllOfTypeIf<named<int>>([](const auto& val) { return val.name == "1"; });
  REQUIRE(filtered.size() == 1);
}

TEST_CASE("Data iterator", "[Data]")
{
  Data data;
  data.insert("testint1", 1);
  data.insert("teststr1", std::string{ "1" });
  data.insert("testint2", 2);
  data.insert("teststr2", std::string{ "2" });

  auto intIt = data.begin<int>();
  REQUIRE(intIt != data.end<int>());
  REQUIRE(intIt->first.get() == "testint1");
  REQUIRE(intIt->second.get() == 1);
  intIt++;
  REQUIRE(intIt->first.get() == "testint2");
  REQUIRE(intIt->second.get() == 2);

  auto strIt = data.begin<std::string>();
  REQUIRE(strIt != data.end<std::string>());
  REQUIRE(strIt->first.get() == "teststr1");
  REQUIRE(strIt->second.get() == "1");
  ++strIt;
  REQUIRE(strIt->first.get() == "teststr2");
  REQUIRE(strIt->second.get() == "2");
}
