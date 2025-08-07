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

#include <TH1.h>
#include <catch_amalgamated.hpp>
#include "Framework/include/QualityControl/MonitorObject.h"
#include "QualityControl/Data.h"
#include "QualityControl/DataAdapters.h"
#include <cstring>
#include <iostream>
#include <memory>

using namespace o2::quality_control::core;

struct nonexistent {
};

TEST_CASE("Data - constructor", "[Data]")
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

TEST_CASE("Data - iterateByType", "[Data]")
{
  Data data;
  data.insert("testint1", 1);
  data.insert("teststr1", std::string{ "1" });
  REQUIRE(data.size() == 2);

  SECTION("iterate by int")
  {
    size_t count{};
    for (auto& v : data.iterateByType<int>()) {
      REQUIRE(v == 1);
      count++;
    }
    REQUIRE(count == 1);
  }

  SECTION("iterate by nonexistent")
  {
    size_t count{};
    REQUIRE(data.iterateByType<nonexistent>().empty());
  }
}

TEST_CASE("Data - iterateByTypeAndFilter", "[Data]")
{
  Data data;
  data.insert("1", 1);
  data.insert("2", 2);
  data.insert("str", "str");
  REQUIRE(data.size() == 3);

  size_t count{};
  for (const auto& v : data.iterateByTypeAndFilter<int>([](const auto& pair) -> bool { return *pair.second == 2; })) {
    ++count;
    REQUIRE(v == 2);
  }
  REQUIRE(count == 1);
}

struct Base {
  int v;
};

struct Derived : public Base {
};

TEST_CASE("Data - iterateByTypeFilterAndTransform", "[Data]")
{

  auto* h1 = new TH1F("th11", "th11", 100, 0, 99);
  std::shared_ptr<MonitorObject> mo1 = std::make_shared<MonitorObject>(h1, "taskname", "class1", "TST");

  auto h2 = new TH1F("th12", "th12", 100, 0, 99);
  std::shared_ptr<MonitorObject> mo2 = std::make_shared<MonitorObject>(h2, "taskname", "class2", "TST");

  Data data;
  data.insert("1", mo1);
  data.insert("2", mo2);
  data.insert("str", "str");
  REQUIRE(data.size() == 3);
  auto filtered = data.iterateByTypeFilterAndTransform<MonitorObject, TH1F>(
    [](const auto& pair) -> bool { return std::string_view{ pair.second->GetName() } == "th11"; },
    [](const MonitorObject* ptr) -> const TH1F* { return dynamic_cast<const TH1F*>(ptr->getObject()); });

  REQUIRE(!filtered.empty());
  size_t count{};
  for (const auto& th1 : filtered) {
    REQUIRE(std::string_view{ th1.GetName() } == "th11");
    ++count;
  }
  REQUIRE(count == 1);
}

TEST_CASE("Data - Monitor adaptors", "[Data]")
{
  auto* h1 = new TH1F("th11", "th11", 100, 0, 99);
  std::shared_ptr<MonitorObject> mo1 = std::make_shared<MonitorObject>(h1, "taskname", "class1", "TST");

  auto* h2 = new TH1F("th12", "th12", 100, 0, 99);
  std::shared_ptr<MonitorObject> mo2 = std::make_shared<MonitorObject>(h2, "taskname", "class2", "TST");

  std::map<std::string, std::shared_ptr<MonitorObject>> map;
  map.emplace(mo1->getFullName(), mo1);
  map.emplace(mo2->getFullName(), mo2);

  auto data = createData(map);

  REQUIRE(data.size() == 2);

  auto filteredHistos = iterateMOsFilterByNameAndTransform<TH1F>(data, "th11");
  std::vector result(filteredHistos.begin(), filteredHistos.end());
  // REQUIRE(!filteredHistos.empty());
  size_t count{};
  for (const auto& histo1d : result) {
    REQUIRE(std::string_view{ histo1d.GetName() } == "th11");
    ++count;
  }
  REQUIRE(count == 1);
}
