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
/// \file    testRootFileStorage.cxx
/// \author  Piotr Konopka
///

#include "QualityControl/RootFileStorage.h"
#include "QualityControl/MonitorObjectCollection.h"
#include "QualityControl/MonitorObject.h"
#include "QualityControl/QcInfoLogger.h"

#include <filesystem>
#include <catch_amalgamated.hpp>
#include <TH1I.h>

using namespace o2::quality_control::core;

struct TestFileFixture {
  TestFileFixture(const std::string& testCase)
  {
    filePath = "/tmp/qc_test_root_file_storage_" + testCase + "_" + std::to_string(getpid()) + ".root";
    if (!filePath.empty()) {
      std::filesystem::remove(filePath);
    }
  }

  ~TestFileFixture()
  {
    if (!filePath.empty()) {
      std::filesystem::remove(filePath);
    }
  }

  std::string filePath;
};

const size_t bins = 10;
const size_t min = 0;
const size_t max = 10;

TEST_CASE("int_write_read")
{
  // the fixture will do the cleanup when being destroyed only after any file readers are destroyed earlier
  TestFileFixture fixture("int_write_read");

  MonitorObjectCollection* mocBefore = new MonitorObjectCollection();
  mocBefore->SetOwner(true);

  TH1I* histoBefore = new TH1I("histo 1d", "histo 1d", bins, min, max);
  histoBefore->Fill(5);
  MonitorObject* moHistoBefore = new MonitorObject(histoBefore, "histo 1d", "class", "DET");
  moHistoBefore->setActivity({ 300000, "PHYSICS", "LHC32x", "apass2", "qc_async", { 100, 300 } });
  moHistoBefore->setIsOwner(true);
  mocBefore->Add(moHistoBefore);
  {
    RootFileStorage storage(fixture.filePath, RootFileStorage::ReadMode::Update);
    REQUIRE(storage.readStructure(false).children.empty());

    // store and read back, check results
    {
      REQUIRE_NOTHROW(storage.storeIntegralMOC(mocBefore));
      auto mocAfter = storage.readMonitorObjectCollection("int/TST/Test");
      REQUIRE(mocAfter != nullptr);

      REQUIRE(mocBefore->GetEntries() == mocAfter->GetEntries());
      auto moHistoAfter = dynamic_cast<MonitorObject*>(mocAfter->At(0));
      REQUIRE(moHistoAfter != nullptr);

      CHECK(moHistoAfter->getActivity() == moHistoBefore->getActivity());
      REQUIRE(moHistoAfter->getObject() != nullptr);
      auto histoAfter = dynamic_cast<TH1I*>(moHistoAfter->getObject());
      CHECK(histoAfter->GetBinContent(histoAfter->FindBin(5)) == 1);
      CHECK(histoAfter->GetSum() == 1);
    }
    // merge mocBefore into the existing file, check results
    {
      REQUIRE_NOTHROW(storage.storeIntegralMOC(mocBefore));
      auto mocAfter = storage.readMonitorObjectCollection("int/TST/Test");
      REQUIRE(mocAfter != nullptr);

      REQUIRE(mocBefore->GetEntries() == mocAfter->GetEntries());
      auto moHistoAfter = dynamic_cast<MonitorObject*>(mocAfter->At(0));
      REQUIRE(moHistoAfter != nullptr);

      CHECK(moHistoAfter->getActivity() == moHistoBefore->getActivity());
      REQUIRE(moHistoAfter->getObject() != nullptr);
      auto histoAfter = dynamic_cast<TH1I*>(moHistoAfter->getObject());
      CHECK(histoAfter->GetBinContent(histoAfter->FindBin(5)) == 2);
      CHECK(histoAfter->GetSum() == 2);
    }
  }

  // close and reopen the file, then merge again, check results
  {
    RootFileStorage storage(fixture.filePath, RootFileStorage::ReadMode::Update);
    REQUIRE_NOTHROW(storage.storeIntegralMOC(mocBefore));
    auto mocAfter = storage.readMonitorObjectCollection("int/TST/Test");
    REQUIRE(mocAfter != nullptr);

    REQUIRE(mocBefore->GetEntries() == mocAfter->GetEntries());
    auto moHistoAfter = dynamic_cast<MonitorObject*>(mocAfter->At(0));
    REQUIRE(moHistoAfter != nullptr);

    CHECK(moHistoAfter->getActivity() == moHistoBefore->getActivity());
    REQUIRE(moHistoAfter->getObject() != nullptr);
    auto histoAfter = dynamic_cast<TH1I*>(moHistoAfter->getObject());
    CHECK(histoAfter->GetBinContent(histoAfter->FindBin(5)) == 3.0);
    CHECK(histoAfter->GetSum() == 3.0);
  }
}

TEST_CASE("mw_write_read")
{
  // the fixture will do the cleanup when being destroyed only after any file readers are destroyed earlier
  TestFileFixture fixture("mw_write_read");

  MonitorObjectCollection* mocBefore = new MonitorObjectCollection();
  mocBefore->SetOwner(true);

  TH1I* histoBefore = new TH1I("histo 1d", "histo 1d", bins, min, max);
  histoBefore->Fill(5);
  MonitorObject* moHistoBefore = new MonitorObject(histoBefore, "histo 1d", "class", "DET");
  moHistoBefore->setActivity({ 300000, "PHYSICS", "LHC32x", "apass2", "qc_async", { 100, 300 } });
  moHistoBefore->setIsOwner(true);
  mocBefore->Add(moHistoBefore);
  {
    RootFileStorage storage(fixture.filePath, RootFileStorage::ReadMode::Update);
    REQUIRE(storage.readStructure(false).children.empty());

    // store and read back, check results
    {
      REQUIRE_NOTHROW(storage.storeMovingWindowMOC(mocBefore));
      auto mocAfter = storage.readMonitorObjectCollection("mw/TST/Test/100");
      REQUIRE(mocAfter != nullptr);

      REQUIRE(mocBefore->GetEntries() == mocAfter->GetEntries());
      auto moHistoAfter = dynamic_cast<MonitorObject*>(mocAfter->At(0));
      REQUIRE(moHistoAfter != nullptr);

      CHECK(moHistoAfter->getActivity() == moHistoBefore->getActivity());
      REQUIRE(moHistoAfter->getObject() != nullptr);
      auto histoAfter = dynamic_cast<TH1I*>(moHistoAfter->getObject());
      CHECK(histoAfter->GetBinContent(histoAfter->FindBin(5)) == 1);
      CHECK(histoAfter->GetSum() == 1);
    }
    // merge mocBefore into the existing file, check results
    {
      // extend the validity forward, start stays the same.
      moHistoBefore->setValidity({ 100, 500 });
      REQUIRE_NOTHROW(storage.storeMovingWindowMOC(mocBefore));
      auto mocAfter = storage.readMonitorObjectCollection("mw/TST/Test/100");
      REQUIRE(mocAfter != nullptr);

      REQUIRE(mocBefore->GetEntries() == mocAfter->GetEntries());
      auto moHistoAfter = dynamic_cast<MonitorObject*>(mocAfter->At(0));
      REQUIRE(moHistoAfter != nullptr);

      CHECK(moHistoAfter->getActivity() == moHistoBefore->getActivity());
      REQUIRE(moHistoAfter->getObject() != nullptr);
      auto histoAfter = dynamic_cast<TH1I*>(moHistoAfter->getObject());
      CHECK(histoAfter->GetBinContent(histoAfter->FindBin(5)) == 2);
      CHECK(histoAfter->GetSum() == 2);
    }
  }

  // move the validity to the future, a new object should be stored in the file
  moHistoBefore->setValidity({ 300, 500 });
  // close and reopen the file, then merge again, check results
  {
    RootFileStorage storage(fixture.filePath, RootFileStorage::ReadMode::Update);
    REQUIRE_NOTHROW(storage.storeMovingWindowMOC(mocBefore));
    auto mocAfter = storage.readMonitorObjectCollection("mw/TST/Test/300");
    REQUIRE(mocAfter != nullptr);

    REQUIRE(mocBefore->GetEntries() == mocAfter->GetEntries());
    auto moHistoAfter = dynamic_cast<MonitorObject*>(mocAfter->At(0));
    REQUIRE(moHistoAfter != nullptr);

    CHECK(moHistoAfter->getActivity() == moHistoBefore->getActivity());
    REQUIRE(moHistoAfter->getObject() != nullptr);
    auto histoAfter = dynamic_cast<TH1I*>(moHistoAfter->getObject());
    CHECK(histoAfter->GetBinContent(histoAfter->FindBin(5)) == 1.0);
    CHECK(histoAfter->GetSum() == 1.0);
  }
}

TEST_CASE("read_structure")
{
  // the fixture will do the cleanup when being destroyed only after any file readers are destroyed earlier
  TestFileFixture fixture("read_structure");

  MonitorObjectCollection* moc = new MonitorObjectCollection();
  moc->SetOwner(true);
  moc->setDetector("TST");

  TH1I* histo1 = new TH1I("histo 1", "histo 1", bins, min, max);
  histo1->Fill(5);
  MonitorObject* moHisto1 = new MonitorObject(histo1, "histo 1", "class", "DET");
  moHisto1->setActivity({ 300000, "PHYSICS", "LHC32x", "apass2", "qc_async", { 100, 300 } });
  moHisto1->setIsOwner(true);
  moc->Add(moHisto1);

  TH1I* histo2 = new TH1I("histo 2", "histo 2", bins, min, max);
  histo1->Fill(5);
  MonitorObject* moHisto2 = new MonitorObject(histo2, "histo 2", "class", "DET");
  moHisto2->setActivity({ 300000, "PHYSICS", "LHC32x", "apass2", "qc_async", { 100, 300 } });
  moHisto2->setIsOwner(true);
  moc->Add(moHisto2);

  RootFileStorage storage(fixture.filePath, RootFileStorage::ReadMode::Update);
  REQUIRE(storage.readStructure(false).children.empty());

  storage.storeIntegralMOC(moc);
  storage.storeMovingWindowMOC(moc);
  dynamic_cast<MonitorObject*>(moc->At(0))->setValidity({ 300, 500 });
  dynamic_cast<MonitorObject*>(moc->At(1))->setValidity({ 300, 500 });
  storage.storeMovingWindowMOC(moc);

  auto structure = storage.readStructure(false);
  CHECK(structure.children.size() == 2);
  {
    REQUIRE(structure.children.find("int") != structure.children.end());
    REQUIRE(std::holds_alternative<RootFileStorage::DirectoryNode>(structure.children.at("int")));
    auto intDir = std::get<RootFileStorage::DirectoryNode>(structure.children.at("int"));
    CHECK(intDir.name == "int");
    CHECK(intDir.fullPath == "int");
    REQUIRE(intDir.children.size() == 1);
    REQUIRE(intDir.children.find("TST") != intDir.children.end());
    REQUIRE(std::holds_alternative<RootFileStorage::DirectoryNode>(intDir.children.at("TST")));
    {
      auto intTstDir = std::get<RootFileStorage::DirectoryNode>(intDir.children.at("TST"));
      CHECK(intTstDir.name == "TST");
      CHECK(intTstDir.fullPath == "int/TST");
      REQUIRE(intTstDir.children.size() == 1);
      REQUIRE(intTstDir.children.find("Test") != intTstDir.children.end());
      REQUIRE(std::holds_alternative<RootFileStorage::MonitorObjectCollectionNode>(intTstDir.children.at("Test")));
      {
        auto intTstTestMoc = std::get<RootFileStorage::MonitorObjectCollectionNode>(intTstDir.children.at("Test"));
        CHECK(intTstTestMoc.name == "Test");
        CHECK(intTstTestMoc.fullPath == "int/TST/Test");
        REQUIRE(intTstTestMoc.moc == nullptr);
      }
    }
  }
  {
    REQUIRE(structure.children.find("mw") != structure.children.end());
    REQUIRE(std::holds_alternative<RootFileStorage::DirectoryNode>(structure.children.at("mw")));
    auto mwDir = std::get<RootFileStorage::DirectoryNode>(structure.children.at("mw"));
    CHECK(mwDir.name == "mw");
    CHECK(mwDir.fullPath == "mw");
    REQUIRE(mwDir.children.size() == 1);
    REQUIRE(mwDir.children.find("TST") != mwDir.children.end());
    REQUIRE(std::holds_alternative<RootFileStorage::DirectoryNode>(mwDir.children.at("TST")));
    {
      auto mwTstDir = std::get<RootFileStorage::DirectoryNode>(mwDir.children.at("TST"));
      CHECK(mwTstDir.name == "TST");
      CHECK(mwTstDir.fullPath == "mw/TST");
      REQUIRE(mwTstDir.children.size() == 1);
      REQUIRE(mwTstDir.children.find("Test") != mwTstDir.children.end());
      REQUIRE(std::holds_alternative<RootFileStorage::DirectoryNode>(mwTstDir.children.at("Test")));
      {
        auto mwTstTestDir = std::get<RootFileStorage::DirectoryNode>(mwTstDir.children.at("Test"));
        CHECK(mwTstTestDir.name == "Test");
        CHECK(mwTstTestDir.fullPath == "mw/TST/Test");
        CHECK(mwTstTestDir.children.size() == 2);
        REQUIRE(mwTstTestDir.children.find("100") != mwTstTestDir.children.end());
        REQUIRE(std::holds_alternative<RootFileStorage::MonitorObjectCollectionNode>(mwTstTestDir.children.at("100")));
        {
          auto mwTstTestMoc100 = std::get<RootFileStorage::MonitorObjectCollectionNode>(mwTstTestDir.children.at("100"));
          CHECK(mwTstTestMoc100.name == "100");
          CHECK(mwTstTestMoc100.fullPath == "mw/TST/Test/100");
          REQUIRE(mwTstTestMoc100.moc == nullptr);
        }
        REQUIRE(mwTstTestDir.children.find("300") != mwTstTestDir.children.end());
        REQUIRE(std::holds_alternative<RootFileStorage::MonitorObjectCollectionNode>(mwTstTestDir.children.at("300")));
        {
          auto mwTstTestMoc300 = std::get<RootFileStorage::MonitorObjectCollectionNode>(mwTstTestDir.children.at("300"));
          CHECK(mwTstTestMoc300.name == "300");
          CHECK(mwTstTestMoc300.fullPath == "mw/TST/Test/300");
          REQUIRE(mwTstTestMoc300.moc == nullptr);
        }
      }
    }
  }

  // now we read MonitorObjectCollections and delete them
  structure = storage.readStructure(true);
  auto intTstTest = std::get<RootFileStorage::MonitorObjectCollectionNode>(
    std::get<RootFileStorage::DirectoryNode>(
      std::get<RootFileStorage::DirectoryNode>(
        structure.children.at("int"))
        .children.at("TST"))
      .children.at("Test"));
  CHECK(intTstTest.moc != nullptr);
  delete intTstTest.moc;

  auto mwTstTest100 = std::get<RootFileStorage::MonitorObjectCollectionNode>(
    std::get<RootFileStorage::DirectoryNode>(
      std::get<RootFileStorage::DirectoryNode>(
        std::get<RootFileStorage::DirectoryNode>(
          structure.children.at("mw"))
          .children.at("TST"))
        .children.at("Test"))
      .children.at("100"));
  CHECK(mwTstTest100.moc != nullptr);
  delete mwTstTest100.moc;

  auto mwTstTest300 = std::get<RootFileStorage::MonitorObjectCollectionNode>(
    std::get<RootFileStorage::DirectoryNode>(
      std::get<RootFileStorage::DirectoryNode>(
        std::get<RootFileStorage::DirectoryNode>(
          structure.children.at("mw"))
          .children.at("TST"))
        .children.at("Test"))
      .children.at("300"));
  CHECK(mwTstTest300.moc != nullptr);
  delete mwTstTest300.moc;
}

TEST_CASE("walking")
{
  // the fixture will do the cleanup when being destroyed only after any file readers are destroyed earlier
  TestFileFixture fixture("walking");

  MonitorObjectCollection* moc = new MonitorObjectCollection();
  moc->SetOwner(true);
  moc->setDetector("TST");

  TH1I* histo1 = new TH1I("histo 1", "histo 1", bins, min, max);
  histo1->Fill(5);
  MonitorObject* moHisto1 = new MonitorObject(histo1, "histo 1", "class", "DET");
  moHisto1->setActivity({ 300000, "PHYSICS", "LHC32x", "apass2", "qc_async", { 100, 300 } });
  moHisto1->setIsOwner(true);
  moc->Add(moHisto1);

  TH1I* histo2 = new TH1I("histo 2", "histo 2", bins, min, max);
  histo1->Fill(5);
  MonitorObject* moHisto2 = new MonitorObject(histo2, "histo 2", "class", "DET");
  moHisto2->setActivity({ 300000, "PHYSICS", "LHC32x", "apass2", "qc_async", { 100, 300 } });
  moHisto2->setIsOwner(true);
  moc->Add(moHisto2);

  RootFileStorage storage(fixture.filePath, RootFileStorage::ReadMode::Update);
  auto structure = storage.readStructure(false);
  REQUIRE(structure.children.empty());

  // check if walkers do not crash when the file is empty
  {
    IntegralMocWalker intWalker(structure);
    REQUIRE(!intWalker.hasNextPath());
    REQUIRE(intWalker.nextPath() == "");
  }
  {
    MovingWindowMocWalker mwWalker(structure);
    REQUIRE(!mwWalker.hasNextPath());
    REQUIRE(mwWalker.nextPath() == "");
  }

  // now we put some data in the file and validate walkers in a usual scenario
  storage.storeIntegralMOC(moc);
  storage.storeMovingWindowMOC(moc);
  dynamic_cast<MonitorObject*>(moc->At(0))->setValidity({ 300, 500 });
  dynamic_cast<MonitorObject*>(moc->At(1))->setValidity({ 300, 500 });
  storage.storeMovingWindowMOC(moc);

  structure = storage.readStructure(false);
  {
    IntegralMocWalker intWalker(structure);
    REQUIRE(intWalker.hasNextPath());
    auto path = intWalker.nextPath();
    REQUIRE(path == "int/TST/Test");
    auto* readMoc = storage.readMonitorObjectCollection(path);
    CHECK(readMoc != nullptr);
    delete readMoc;
    REQUIRE(!intWalker.hasNextPath());
    CHECK(intWalker.nextPath().empty());
  }
  {
    MovingWindowMocWalker mwWalker(structure);
    REQUIRE(mwWalker.hasNextPath());
    auto path = mwWalker.nextPath();
    REQUIRE(path == "mw/TST/Test/100");
    auto* readMoc = storage.readMonitorObjectCollection(path);
    CHECK(readMoc != nullptr);
    delete readMoc;
    REQUIRE(mwWalker.hasNextPath());
    path = mwWalker.nextPath();
    REQUIRE(path == "mw/TST/Test/300");
    readMoc = storage.readMonitorObjectCollection(path);
    CHECK(readMoc != nullptr);
    delete readMoc;
    REQUIRE(!mwWalker.hasNextPath());
    CHECK(mwWalker.nextPath().empty());
  }
}