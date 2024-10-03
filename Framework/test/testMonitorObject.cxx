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
/// \file   testMonitorObject.cxx
/// \author Barthelemy von Haller
///

#include "QualityControl/MonitorObject.h"
#include "QualityControl/QcInfoLogger.h"

#include <catch_amalgamated.hpp>
#include <chrono>
#include <TH1.h>
#include <TH1F.h>
#include <TFile.h>
#include <TSystem.h>

using namespace std;

namespace o2::quality_control::core
{

TEST_CASE("mo")
{
  o2::quality_control::core::MonitorObject obj;
  CHECK(obj.getName() == "");
  CHECK(std::string(obj.GetName()) == "");
}

TEST_CASE("mo_save")
{
  string objectName = "asdf";
  TH1F h(objectName.data(), objectName.data(), 100, 0, 99);
  o2::quality_control::core::MonitorObject obj(&h, "task", "class", "DET");
  ILOG(Info, Support) << "getName : '" << obj.getName() << "'" << ENDM;
  ILOG(Info, Support) << "GetName : '" << obj.GetName() << "'" << ENDM;
  ILOG(Info, Support) << "title : '" << obj.GetTitle() << "'" << ENDM;
  CHECK(obj.getName() == "asdf");
  CHECK(std::string(obj.GetName()) == "asdf");
  CHECK(std::string(obj.GetTitle()) == "");
  obj.setIsOwner(false);
  string libName = "libraryName";
  string libName2 = "libraryName2";
  std::chrono::nanoseconds ns = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::system_clock::now().time_since_epoch());
  std::string filename = string("/tmp/test") + std::to_string(ns.count()) + ".root";
  TFile file(filename.data(), "RECREATE");
  obj.Write(obj.getName().data());
  file.Close();

  ILOG(Info, Support) << "***" << ENDM;
  TFile file2(filename.data());
  auto* mo = dynamic_cast<o2::quality_control::core::MonitorObject*>(file2.Get(objectName.data()));
  CHECK(mo != nullptr);
  ILOG(Info, Support) << "mo : " << mo << ENDM;
  CHECK(mo->GetName() == objectName);
  CHECK(mo->getName() == objectName);
  ILOG(Info, Support) << "name : " << mo->GetName() << ENDM;
  ILOG(Info, Support) << "name : " << mo->getName() << ENDM;
  gSystem->Unlink(filename.data());
}

TEST_CASE("mo_clone")
{
  auto th1 = std::make_unique<TNamed>("name", "title");
  TObject* obj = th1.get();
  auto* cloned = obj->Clone();
  auto* secondCloned = cloned->Clone();
  delete cloned;
  delete secondCloned;
}

TEST_CASE("mo_copy")
{
  auto compareWithoutObject = [](const MonitorObject& lhs, const MonitorObject& rhs) {
    REQUIRE(lhs.getName() == rhs.getName());
    REQUIRE(lhs.getTaskName() == rhs.getTaskName());
    REQUIRE(lhs.getDetectorName() == rhs.getDetectorName());
    REQUIRE(lhs.getTaskClass() == rhs.getTaskClass());
    REQUIRE(lhs.isIsOwner() == rhs.isIsOwner());
    REQUIRE(lhs.getActivity() == rhs.getActivity());
    REQUIRE(lhs.getCreateMovingWindow() == rhs.getCreateMovingWindow());
    REQUIRE(lhs.getDescription() == rhs.getDescription());
    REQUIRE(lhs.getMetadataMap() == rhs.getMetadataMap());
  };

  SECTION("Empty orignal MO")
  {
    MonitorObject original{};

    SECTION("copy of non owning object returns same pointer")
    {
      auto compareShallowNonOwning = [&compareWithoutObject](const MonitorObject& lhs, const MonitorObject& rhs) {
        compareWithoutObject(lhs, rhs);
        // we expect shallow rhs when lhs does not own the underlying object
        REQUIRE(lhs.getObject() == nullptr);
        REQUIRE(rhs.getObject() == nullptr);
      };

      SECTION("copy constructor")
      {
        MonitorObject copy{ original };
        compareShallowNonOwning(original, copy);
      }

      SECTION("copy assignment operator")
      {
        MonitorObject copy{};
        copy = original;
        compareShallowNonOwning(original, copy);
      }

      SECTION("Copy method")
      {
        MonitorObject copy{};
        original.Copy(copy);
        compareShallowNonOwning(original, copy);
      }
    }

    SECTION("copy of object owned by MO returns deep copy, non nullptr init")
    {
      auto compareTNamed = [](const MonitorObject& lhs, const MonitorObject& rhs) {
        REQUIRE(lhs.getObject() == nullptr);
        REQUIRE(rhs.getObject() == nullptr);
      };

      SECTION("copy owning object before")
      {
        MonitorObject copy{};
        copy.setObject(new TNamed("copy named", "title copy"));
        copy.setIsOwner(true);

        SECTION("copy assignment operator")
        {
          copy = original;
          compareTNamed(original, copy);
        }

        SECTION("Copy method")
        {
          original.Copy(copy);
          compareTNamed(original, copy);
        }
      }
    }
  }

  SECTION("original MO with data")
  {

    MonitorObject original{};

    original.setTaskName("taskName");
    original.setTaskClass("taskClass");
    original.setDescription("description");
    original.setDetectorName("TST");
    original.setActivity({ 123, "type", "periodName", "passName", "provenance", gFullValidityInterval, "beamType", "partitionName", 2 });
    original.setCreateMovingWindow(true);

    SECTION("copy of non owning object returns same pointer")
    {
      auto compareShallowNonOwning = [&compareWithoutObject](const MonitorObject& lhs, const MonitorObject& rhs) {
        compareWithoutObject(lhs, rhs);
        // we expect shallow rhs when lhs does not own the underlying object
        REQUIRE((lhs.getObject() != nullptr && lhs.getObject() == rhs.getObject()));
      };

      auto th1 = TH1I("name", "title", 10, 0, 10);
      th1.Fill(8);
      original.setObject(&th1);
      original.setIsOwner(false);

      SECTION("copy constructor")
      {
        MonitorObject copy{ original };
        compareShallowNonOwning(original, copy);
      }

      SECTION("copy assignment operator")
      {
        MonitorObject copy{};
        copy = original;
        compareShallowNonOwning(original, copy);
      }

      SECTION("Copy method")
      {
        MonitorObject copy{};
        original.Copy(copy);
        compareShallowNonOwning(original, copy);
      }
    }

    SECTION("copy of object owned by MO returns deep copy, init from nullptr")
    {
      auto compareTNamed = [](const MonitorObject& lhs, const MonitorObject& rhs) {
        auto* namedoriginal = static_cast<TNamed*>(lhs.getObject());
        auto* namedcopy = static_cast<TNamed*>(rhs.getObject());
        REQUIRE(std::string(namedoriginal->GetName()) == std::string(namedcopy->GetName()));
        REQUIRE(std::string(namedoriginal->GetTitle()) == std::string(namedcopy->GetTitle()));
      };

      auto* named = new TNamed("named", "title");
      original.setObject(named);
      original.setIsOwner(true);

      SECTION("copy constructor")
      {
        MonitorObject copy{ original };
        compareTNamed(original, copy);
      }

      SECTION("copy assignment operator")
      {
        MonitorObject copy{};
        copy = original;
        compareTNamed(original, copy);
      }

      SECTION("Copy method")
      {
        MonitorObject copy{};
        original.Copy(copy);
        compareTNamed(original, copy);
      }
    }

    SECTION("copy of object owned by MO returns deep copy, non nullptr init")
    {
      auto compareTNamed = [](const MonitorObject& lhs, const MonitorObject& rhs) {
        auto* namedoriginal = static_cast<TNamed*>(lhs.getObject());
        auto* namedcopy = static_cast<TNamed*>(rhs.getObject());
        REQUIRE(std::string(namedoriginal->GetName()) == std::string(namedcopy->GetName()));
        REQUIRE(std::string(namedoriginal->GetTitle()) == std::string(namedcopy->GetTitle()));
      };

      auto* named = new TNamed("named", "title");
      original.setObject(named);
      original.setIsOwner(true);

      SECTION("copy owning object before")
      {
        MonitorObject copy{};
        copy.setObject(new TNamed("copy named", "title copy"));
        copy.setIsOwner(true);

        SECTION("copy assignment operator")
        {
          copy = original;
          compareTNamed(original, copy);
        }

        SECTION("Copy method")
        {
          original.Copy(copy);
          compareTNamed(original, copy);
        }
      }
    }
  }
}

TEST_CASE("metadata")
{
  string objectName = "asdf";
  TH1F h(objectName.data(), objectName.data(), 100, 0, 99);

  // no metadata at creation
  o2::quality_control::core::MonitorObject obj(&h, "task", "class", "DET");
  obj.setIsOwner(false);
  CHECK(obj.getMetadataMap().size() == 0);

  // add metadata with key value, check it is there
  obj.addMetadata("key1", "value1");
  CHECK(obj.getMetadataMap().size() == 1);
  CHECK(obj.getMetadataMap().at("key1") == "value1");

  // add same key again -> ignore
  obj.addMetadata("key1", "value1");
  CHECK(obj.getMetadataMap().size() == 1);
  auto test = obj.getMetadataMap();
  CHECK(obj.getMetadataMap().at("key1") == "value1");

  // add map
  map<string, string> another = { { "key2", "value2" }, { "key3", "value3" } };
  obj.addMetadata(another);
  CHECK(obj.getMetadataMap().size() == 3);
  CHECK(obj.getMetadataMap().at("key1") == "value1");
  CHECK(obj.getMetadataMap().at("key2") == "value2");
  CHECK(obj.getMetadataMap().at("key3") == "value3");

  // add map sharing some keys -> those are ignored not the others
  map<string, string> another2 = { { "key2", "value2a" }, { "key4", "value4" } };
  obj.addMetadata(another2);
  CHECK(obj.getMetadataMap().size() == 4);
  CHECK(obj.getMetadataMap().at("key1") == "value1");
  CHECK(obj.getMetadataMap().at("key2") == "value2");
  CHECK(obj.getMetadataMap().at("key3") == "value3");
  CHECK(obj.getMetadataMap().at("key4") == "value4");

  // update value of existing key
  obj.updateMetadata("key1", "value11");
  CHECK(obj.getMetadataMap().size() == 4);
  CHECK(obj.getMetadataMap().at("key1") == "value11");

  // update value of non-existing key -> ignore
  obj.updateMetadata("asdf", "asdf");
  CHECK(obj.getMetadataMap().size() == 4);
}

TEST_CASE("path")
{
  string objectName = "asdf";
  TH1F h(objectName.data(), objectName.data(), 100, 0, 99);
  o2::quality_control::core::MonitorObject obj(&h, "task", "class", "DET");
  obj.setIsOwner(false);
  string path = obj.getPath();
  CHECK(path == "qc/DET/MO/task/asdf");
}

TEST_CASE("validity")
{
  o2::quality_control::core::MonitorObject obj;

  CHECK(obj.getValidity() == gInvalidValidityInterval);

  obj.updateValidity(1234);
  CHECK(obj.getValidity() == o2::quality_control::core::ValidityInterval(1234, 1234));
  obj.updateValidity(9000);
  CHECK(obj.getValidity() == o2::quality_control::core::ValidityInterval(1234, 9000));

  obj.setValidity({ 3, 4 });
  CHECK(obj.getValidity() == o2::quality_control::core::ValidityInterval(3, 4));
}

} // namespace o2::quality_control::core
