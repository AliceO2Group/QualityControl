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
/// \file    testQCInputs.cxx
/// \author  Michal Tichak
///

#include <TH1.h>
#include <boost/container/flat_map.hpp>
#include <catch_amalgamated.hpp>
#include "Framework/include/QualityControl/MonitorObject.h"
#include "QualityControl/QCInputs.h"
#include "QualityControl/QCInputsAdapters.h"
#include <cstring>
#include <memory>
#include <string>

using namespace o2::quality_control::core;

struct nonexistent {
};

TEST_CASE("Data - constructor", "[Data]")
{
  REQUIRE_NOTHROW([]() { QCInputs data{}; });
}

TEST_CASE("Data insert and get", "[Data]")
{
  QCInputs data;
  data.insert("test", 1);
  auto valueStr = data.get<std::string>("test");
  REQUIRE(!valueStr.has_value());
  auto valueInt = data.get<int>("test");
  REQUIRE(valueInt.has_value());
  REQUIRE(valueInt == 1);
}

TEST_CASE("Data - iterateByType", "[Data]")
{
  QCInputs data;
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
  QCInputs data;
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

TEST_CASE("Data - iterateByTypeFilterAndTransform", "[Data]")
{

  auto* h1 = new TH1F("th11", "th11", 100, 0, 99);
  std::shared_ptr<MonitorObject> mo1 = std::make_shared<MonitorObject>(h1, "taskname", "class1", "TST");

  auto h2 = new TH1F("th12", "th12", 100, 0, 99);
  std::shared_ptr<MonitorObject> mo2 = std::make_shared<MonitorObject>(h2, "taskname", "class2", "TST");

  QCInputs data;
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

TEST_CASE("Data - raw pointers", "[Data]")
{
  QCInputs data;
  int a = 1;
  int b = 2;
  data.insert("1", &a);
  data.insert("2", &b);

  auto ints = data.iterateByType<int>();
  REQUIRE(!ints.empty());

  size_t count{};
  for (const auto& v : ints) {
    REQUIRE((v == 1 || v == 2));
    ++count;
  }
  REQUIRE(count == 2);
}

using stdmap = std::map<std::string, std::any, std::less<>>;
using boostflatmap = boost::container::flat_map<std::string, std::any, std::less<>>;

TEMPLATE_TEST_CASE("Data - inserting fundamental types", "[.Data-benchmark]", stdmap, boostflatmap, transparent_unordered_map)
{
  constexpr size_t iterations = 20'000;

  BENCHMARK("insert size_t")
  {
    QCInputsGeneric<TestType> data;
    // for (size_t i = 0; i != iterations; ++i) {
    for (size_t i = iterations; i != 0; --i) {
      data.insert(std::to_string(i), i);
    }
  };
}

TEMPLATE_TEST_CASE("Data - iterating fundamental types", "[.Data-benchmark]", stdmap, boostflatmap, transparent_unordered_map)
{
  constexpr size_t iterations = 20000;
  QCInputsGeneric<TestType> data;
  for (size_t i = 0; i != iterations; ++i) {
    data.insert(std::to_string(i), i);
  }

  REQUIRE(data.size() == iterations);
  BENCHMARK("iterate size_t")
  {
    REQUIRE(data.size() == iterations);
    size_t r{};
    size_t count{};
    for (const auto& v : data.template iterateByType<size_t>()) {
      r += v;
      count++;
    }
    REQUIRE(count == iterations);
  };
}

TEMPLATE_TEST_CASE("Data - get fundamental types", "[.Data-benchmark]", stdmap, boostflatmap, transparent_unordered_map)
{
  constexpr size_t iterations = 20000;
  QCInputsGeneric<TestType> data;
  for (size_t i = 0; i != iterations; ++i) {
    data.insert(std::to_string(i), i);
  }

  REQUIRE(data.size() == iterations);
  BENCHMARK("iterate size_t")
  {
    size_t r{};
    size_t count{};
    for (size_t i = 0; i != iterations; ++i) {
      auto opt = data.template get<size_t>(std::to_string(i));
      r += opt.value();
      auto& v = opt.value();
      count++;
    }
    REQUIRE(count == iterations);
  };
}

std::string generateRandomString(size_t length)
{
  static constexpr std::string_view CHARACTERS = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
  thread_local std::mt19937 generator(std::random_device{}());
  std::uniform_int_distribution<size_t> distribution(0, CHARACTERS.length() - 1);

  std::string random_string;
  random_string.reserve(length);
  for (size_t i = 0; i < length; ++i) {
    random_string += CHARACTERS[distribution(generator)];
  }
  return random_string;
}

TEMPLATE_TEST_CASE("Data - inserting and iterating MOs", "[.Data-benchmark]", stdmap, boostflatmap, transparent_unordered_map)
{
  constexpr size_t iterations = 1000;
  std::vector<std::shared_ptr<MonitorObject>> MOs;

  for (size_t i = 0; i != iterations; ++i) {
    const auto name = generateRandomString(20);
    auto* h = new TH1F(name.c_str(), name.c_str(), 100, 0, 99);
    std::shared_ptr<MonitorObject> mo = std::make_shared<MonitorObject>(h, "taskname", "class1", "TST");
    MOs.push_back(mo);
  }

  BENCHMARK("insert - iterate MOs")
  {
    QCInputsGeneric<TestType> data;
    for (const auto& mo : MOs) {
      data.insert(mo->getFullName(), mo);
    }

    const auto filterMOByName = [](const auto& pair) {
      return std::string_view(pair.second->GetName()) == "nonexistent";
    };

    const auto getInternalObject = [](const MonitorObject* ptr) -> const auto* {
      return dynamic_cast<const TH1F*>(ptr->getObject());
    };
    REQUIRE(data.template iterateByTypeFilterAndTransform<MonitorObject, TH1F>(filterMOByName, getInternalObject).empty());
  };
}

TEST_CASE("Data adapters - helper functions", "[Data]")
{

  QCInputs data;
  {
    for (size_t i{}; i != 10; ++i) {
      const auto iStr = std::to_string(i);
      const auto thName = std::string("TH1F_") + iStr;
      const auto moName = "testMO_" + iStr;
      auto* h = new TH1F(thName.c_str(), thName.c_str(), 100, 0, 99);
      data.insert(moName, std::make_shared<MonitorObject>(h, "taskname_" + iStr, "class1", "TST"));
    }

    auto* h = new TH1F("TH1F_duplicate", "TH1F_duplicate", 100, 0, 99);
    data.insert("testMO_duplicate", std::make_shared<MonitorObject>(h, "taskname_8", "class1", "TST"));

    data.insert("testQO_1", std::make_shared<QualityObject>(Quality::Good, "QO_1"));
    data.insert("testQO_2", std::make_shared<QualityObject>(Quality::Good, "QO_2"));
  }

  REQUIRE(data.size() == 13);

  SECTION("getMonitorObject")
  {
    const auto moOpt = getMonitorObject(data, "TH1F_1");
    REQUIRE(moOpt.has_value());
    REQUIRE(std::string_view(moOpt.value().get().GetName()) == "TH1F_1");
    const auto th1Opt = getMonitorObject<TH1F>(data, "TH1F_8");
    REQUIRE(th1Opt.has_value());
    REQUIRE(std::string_view(th1Opt.value().get().GetName()) == "TH1F_8");

    const auto moSpecificOpt = getMonitorObject(data, "TH1F_duplicate", "taskname_8");
    REQUIRE(moSpecificOpt.has_value());
    REQUIRE(moSpecificOpt.value().get().GetName() == std::string_view{ "TH1F_duplicate" });
    REQUIRE(moSpecificOpt.value().get().getTaskName() == std::string_view{ "taskname_8" });
    const auto th1SpecificOpt = getMonitorObject<TH1F>(data, "TH1F_duplicate", "taskname_8");
    REQUIRE(th1SpecificOpt.has_value());
    REQUIRE(th1SpecificOpt.value().get().GetName() == std::string_view{ "TH1F_duplicate" });
    REQUIRE(!getMonitorObject<nonexistent>(data, "TH1F_duplicate", "taskname_8").has_value());
  }

  SECTION("iterateMonitorObjects")
  {
    size_t count{};
    for (auto& mo : iterateMonitorObjects(data)) {
      ++count;
    }
    REQUIRE(count == 11);

    count = 0;
    for (auto& mo : iterateMonitorObjects(data, "taskname_8")) {
      ++count;
    }
    REQUIRE(count == 2);
  }

  SECTION("getQualityObject")
  {
    const auto qoOpt = getQualityObject(data, "QO_1");
    REQUIRE(qoOpt.has_value());
    REQUIRE(std::string_view{ qoOpt.value().get().GetName() } == "QO_1");
  }

  SECTION("iterateQualityObjects")
  {
    size_t count{};
    for (const auto& qo : iterateQualityObjects(data)) {
      ++count;
    }
    REQUIRE(count == 2);
  }
}
