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

struct nonexistent {
};

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

TEST_CASE("Data iterator - constructing", "[Data]")
{
  Data data;
  data.insert("testint1", 1);
  data.insert("teststr1", std::string{ "1" });

  REQUIRE(data.size() == 2);

  SECTION("int iterator has some value")
  {
    REQUIRE(data.begin<int>() != data.end<int>());
  }

  SECTION("string iterator has some value")
  {
    REQUIRE(data.begin<std::string>() != data.end<std::string>());
  }

  SECTION("nonexistent value return end iterator")
  {
    REQUIRE(data.begin<nonexistent>() == data.end<nonexistent>());
  }
}

TEST_CASE("Data iterator - incrementing", "[Data]")
{
  Data data;
  data.insert("testint1", 1);
  data.insert("teststr1", std::string{ "1" });
  data.insert("testint2", 2);
  data.insert("teststr2", std::string{ "2" });

  REQUIRE(data.size() == 4);

  SECTION("postfix int iterator")
  {
    auto intIt = data.begin<int>();
    REQUIRE(intIt != data.end<int>());
    REQUIRE(intIt->first.get() == "testint1");
    REQUIRE(intIt->second.get() == 1);
    intIt++;
    REQUIRE(intIt != data.end<int>());
    REQUIRE(intIt->first.get() == "testint2");
    REQUIRE(intIt->second.get() == 2);
  }

  SECTION("prefix int iterator")
  {
    auto intIt = data.begin<int>();
    REQUIRE(intIt != data.end<int>());
    REQUIRE(intIt->first.get() == "testint1");
    REQUIRE(intIt->second.get() == 1);
    ++intIt;
    REQUIRE(intIt != data.end<int>());
    REQUIRE(intIt->first.get() == "testint2");
    REQUIRE(intIt->second.get() == 2);
  }

  SECTION("postfix str iterator")
  {
    auto strIt = data.begin<std::string>();
    REQUIRE(strIt != data.end<std::string>());
    REQUIRE(strIt->first.get() == "teststr1");
    REQUIRE(strIt->second.get() == "1");
    strIt++;
    REQUIRE(strIt != data.end<std::string>());
    REQUIRE(strIt->first.get() == "teststr2");
    REQUIRE(strIt->second.get() == "2");
  }

  SECTION("prefix str iterator")
  {
    auto strIt = data.begin<std::string>();
    REQUIRE(strIt != data.end<std::string>());
    REQUIRE(strIt->first.get() == "teststr1");
    REQUIRE(strIt->second.get() == "1");
    ++strIt;
    REQUIRE(strIt != data.end<std::string>());
    REQUIRE(strIt->first.get() == "teststr2");
    REQUIRE(strIt->second.get() == "2");
  }
}

TEST_CASE("Data iterator - for loop no filtering", "[Data]")
{
  Data data;
  data.insert("testint1", 1);
  data.insert("teststr1", std::string{ "1" });
  data.insert("testint2", 2);
  data.insert("teststr2", std::string{ "2" });

  REQUIRE(data.size() == 4);

  SECTION("int loop")
  {
    for (const auto& [key, value] : data.iterate<int>()) {
      REQUIRE((key.get() == "testint1" || key.get() == "testint2"));
      REQUIRE((value.get() == 1 || value.get() == 2));
    }
  }

  SECTION("string loop")
  {
    for (const auto& [key, value] : data.iterate<std::string>()) {
      REQUIRE((key.get() == "teststr1" || key.get() == "teststr2"));
      REQUIRE((value.get() == "1" || value.get() == "2"));
    }
  }

  SECTION("empty loop")
  {
    auto filteredRange = data.iterate<nonexistent>();
    REQUIRE(filteredRange.begin() == filteredRange.end());
  }
}

TEST_CASE("Data iterator - for loop filter", "[Data]")
{
  Data data;
  data.insert("testint1", 1);
  data.insert("teststr1", std::string{ "1" });
  data.insert("testint2", 2);
  data.insert("teststr2", std::string{ "2" });

  REQUIRE(data.size() == 4);

  SECTION("int loop")
  {
    for (const auto& [key, value] : data.iterateAndFilter<int>([](const int& val) { return val == 1; })) {
      REQUIRE((key.get() == "testint1"));
      REQUIRE((value.get() == 1));
    }
  }
}
