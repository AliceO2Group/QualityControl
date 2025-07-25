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
/// \file   testMonitorObjectCollection.cxx
/// \author Piotr Konopka
///

#include "Framework/include/QualityControl/ObjectMetadataKeys.h"
#include "QualityControl/MonitorObjectCollection.h"
#include "QualityControl/MonitorObject.h"

#include <TH1.h>
#include <TH1I.h>
#include <TH2I.h>
#include <TH2I.h>
#include <Mergers/CustomMergeableTObject.h>
#include <Mergers/MergerAlgorithm.h>

#include <catch_amalgamated.hpp>

using namespace o2::mergers;

namespace o2::quality_control::core
{

TEST_CASE("monitor_object_collection_merge")
{
  const size_t bins = 10;
  const size_t min = 0;
  const size_t max = 10;

  // Setting up the target. Histo 1D
  MonitorObjectCollection* target = new MonitorObjectCollection();
  target->SetOwner(true);

  TH1I* targetTH1I = new TH1I("histo 1d", "histo 1d", bins, min, max);
  targetTH1I->Fill(5);
  MonitorObject* targetMoTH1I = new MonitorObject(targetTH1I, "histo 1d", "class", "DET");
  targetMoTH1I->setActivity({ 300000, "PHYSICS", "LHC32x", "apass2", "qc_async", gInvalidValidityInterval });
  targetMoTH1I->setIsOwner(true);
  target->Add(targetMoTH1I);

  // Setting up the other. Histo 1D + Histo 2D
  MonitorObjectCollection* other = new MonitorObjectCollection();
  other->SetOwner(true);

  TH1I* otherTH1I = new TH1I("histo 1d", "histo 1d", bins, min, max);
  otherTH1I->Fill(5);
  MonitorObject* otherMoTH1I = new MonitorObject(otherTH1I, "histo 1d", "class", "DET");
  otherMoTH1I->setActivity({ 300000, "PHYSICS", "LHC32x", "apass2", "qc_async", { 43, 60 } });
  otherMoTH1I->setIsOwner(true);
  other->Add(otherMoTH1I);

  TH2I* otherTH2I = new TH2I("histo 2d", "histo 2d", bins, min, max, bins, min, max);
  otherTH2I->Fill(5, 5);
  MonitorObject* otherMoTH2I = new MonitorObject(otherTH2I, "histo 2d", "class", "DET");
  other->Add(otherMoTH2I);

  // Merge 1st time
  CHECK_NOTHROW(algorithm::merge(target, other));

  // Merge 2nd time to check stability and correct bevahiour with objects of wrong validity
  otherMoTH1I->setValidity(gInvalidValidityInterval);
  otherMoTH2I->setValidity(gInvalidValidityInterval);
  otherTH1I->Reset();
  otherTH2I->Reset();
  CHECK_NOTHROW(algorithm::merge(target, other));

  // Make sure that deleting the object present only in `other` doesn't delete it in the `target`
  delete other;

  // Checks
  REQUIRE(target->GetEntries() == 2);

  MonitorObject* resultMoTH1I = dynamic_cast<MonitorObject*>(target->FindObject("histo 1d"));
  REQUIRE(resultMoTH1I != nullptr);
  TH1I* resultTH1I = dynamic_cast<TH1I*>(resultMoTH1I->getObject());
  REQUIRE(resultTH1I != nullptr);
  CHECK(resultTH1I->GetBinContent(resultTH1I->FindBin(5)) == 2);
  CHECK(resultMoTH1I->getValidity() == ValidityInterval{ 43, 60 });

  MonitorObject* resultMoTH2I = dynamic_cast<MonitorObject*>(target->FindObject("histo 2d"));
  REQUIRE(resultMoTH2I != nullptr);
  TH2I* resultTH2I = dynamic_cast<TH2I*>(resultMoTH2I->getObject());
  REQUIRE(resultTH2I != nullptr);
  CHECK(resultTH2I->GetBinContent(resultTH2I->FindBin(5, 5)) == 1);

  delete target;
}

TEST_CASE("monitor_object_collection_merge_different_id")
{
  const auto toHisto = [](std::unique_ptr<MonitorObjectCollection>& collection) -> TH1I* {
    return dynamic_cast<TH1I*>(dynamic_cast<MonitorObject*>(collection->At(0))->getObject());
  };

  constexpr size_t bins = 10;
  constexpr size_t min = 0;
  constexpr size_t max = 10;

  SECTION("other has higher run number than target")
  {
    auto target = std::make_unique<MonitorObjectCollection>();

    auto* targetTH1I = new TH1I("histo 1d", "original", bins, min, max);
    targetTH1I->Fill(5);
    auto* targetMoTH1I = new MonitorObject(targetTH1I, "histo 1d", "class", "DET");
    targetMoTH1I->setActivity({ 123, "PHYSICS", "LHC32x", "apass2", "qc_async", { 10, 20 } });
    targetMoTH1I->setIsOwner(true);
    target->Add(targetMoTH1I);

    auto other = std::make_unique<MonitorObjectCollection>();
    other->SetOwner(true);

    auto* otherTH1I = new TH1I("histo 1d", "input", bins, min, max);
    otherTH1I->Fill(2);
    auto* otherMoTH1I = new MonitorObject(otherTH1I, "histo 1d", "class", "DET");
    otherMoTH1I->setActivity({ 1234, "PHYSICS", "LHC32x", "apass2", "qc_async", { 43, 60 } });
    otherMoTH1I->setIsOwner(true);
    other->Add(otherMoTH1I);

    CHECK_NOTHROW(algorithm::merge(target.get(), other.get()));
    auto* h1orig = toHisto(target);
    auto* h1other = toHisto(other);
    REQUIRE(h1orig->GetAt(3) == 1);
    for (size_t i = 0; i != h1orig->GetSize(); ++i) {
      REQUIRE(h1orig->GetAt(i) == h1other->GetAt(i));
    }
  }

  SECTION("other has lower run number than target")
  {
    auto target = std::make_unique<MonitorObjectCollection>();

    auto* targetTH1I = new TH1I("histo 1d", "original", bins, min, max);
    targetTH1I->Fill(5);
    auto* targetMoTH1I = new MonitorObject(targetTH1I, "histo 1d", "class", "DET");
    targetMoTH1I->setActivity({ 1234, "PHYSICS", "LHC32x", "apass2", "qc_async", { 10, 20 } });
    targetMoTH1I->setIsOwner(true);
    target->Add(targetMoTH1I);

    auto other = std::make_unique<MonitorObjectCollection>();
    other->SetOwner(true);

    auto* otherTH1I = new TH1I("histo 1d", "input", bins, min, max);
    otherTH1I->Fill(2);
    auto* otherMoTH1I = new MonitorObject(otherTH1I, "histo 1d", "class", "DET");
    otherMoTH1I->setActivity({ 123, "PHYSICS", "LHC32x", "apass2", "qc_async", { 43, 60 } });
    otherMoTH1I->setIsOwner(true);
    other->Add(otherMoTH1I);

    CHECK_NOTHROW(algorithm::merge(target.get(), other.get()));
    auto* h1orig = toHisto(target);
    auto* h1other = toHisto(other);
    REQUIRE(h1orig->At(h1orig->FindBin(5)) == 1);
    REQUIRE(h1other->At(h1other->FindBin(5)) == 0);
    REQUIRE(h1orig->At(h1orig->FindBin(2)) == 0);
    REQUIRE(h1other->At(h1other->FindBin(2)) == 1);
  }
}

TEST_CASE("monitor_object_collection_post_deserialization")
{
  const size_t bins = 10;
  const size_t min = 0;
  const size_t max = 10;

  // Setting up the moc. Histo 1D
  MonitorObjectCollection* moc = new MonitorObjectCollection();
  moc->SetOwner(false);

  TH1I* objTH1I = new TH1I("histo 1d", "histo 1d", bins, min, max);
  objTH1I->Fill(5);
  MonitorObject* moTH1I = new MonitorObject(objTH1I, "histo 1d", "class", "DET");
  moTH1I->setIsOwner(false);
  moc->Add(moTH1I);

  moc->postDeserialization();

  CHECK(moc->IsOwner() == true);
  CHECK(moTH1I->isIsOwner() == true);

  delete moc;
}

TEST_CASE("monitor_object_collection_clone_mw")
{
  const size_t bins = 10;
  const size_t min = 0;
  const size_t max = 10;

  // Setting up the moc. Histo 1D
  MonitorObjectCollection* moc = new MonitorObjectCollection();
  moc->SetOwner(false);

  TH1I* objTH1I = new TH1I("histo 1d", "histo 1d", bins, min, max);
  objTH1I->Fill(5);
  MonitorObject* moTH1I = new MonitorObject(objTH1I, "histo 1d", "class", "DET");
  moTH1I->setIsOwner(false);
  moTH1I->setCreateMovingWindow(true);
  moTH1I->setValidity({ 10, 432000 });
  moc->Add(moTH1I);

  TH2I* objTH2I = new TH2I("histo 2d", "histo 2d", bins, min, max, bins, min, max);
  MonitorObject* moTH2I = new MonitorObject(objTH2I, "histo 2d", "class", "DET");
  moTH2I->setCreateMovingWindow(false);
  moc->Add(moTH2I);

  auto mwMergeInterface = moc->cloneMovingWindow();

  REQUIRE(mwMergeInterface != nullptr);
  auto mwMOC = dynamic_cast<MonitorObjectCollection*>(mwMergeInterface);
  REQUIRE(mwMOC != nullptr);
  REQUIRE(mwMOC->GetEntries() == 1);
  CHECK(mwMOC->IsOwner() == true);

  MonitorObject* mwMoTH1I = dynamic_cast<MonitorObject*>(mwMOC->FindObject("histo 1d"));
  REQUIRE(mwMoTH1I != nullptr);
  CHECK(mwMoTH1I->isIsOwner() == true);
  TH1I* mwTH1I = dynamic_cast<TH1I*>(mwMoTH1I->getObject());
  REQUIRE(mwTH1I != nullptr);
  CHECK(mwTH1I->GetBinContent(mwTH1I->FindBin(5)) == 1);
  CHECK(std::strcmp(mwTH1I->GetTitle(), "histo 1d (7m11s window)") == 0);

  moTH1I->setValidity(gInvalidValidityInterval);
  auto mwMergeInterface2 = moc->cloneMovingWindow();
  REQUIRE(mwMergeInterface2 != nullptr);
  auto mwMOC2 = dynamic_cast<MonitorObjectCollection*>(mwMergeInterface2);
  REQUIRE(mwMOC2 != nullptr);
  REQUIRE(mwMOC2->GetEntries() == 0);

  delete moc;
  delete mwMOC;
  delete mwMOC2;
}

TEST_CASE("monitor_object_collection_merge_cycle")
{
  MonitorObjectCollection target;
  MonitorObjectCollection other;
  constexpr size_t bins = 10;
  constexpr size_t min = 0;
  constexpr size_t max = 10;

  target.SetOwner(true);

  TH1I* targetTH1I = new TH1I("histo 1d", "histo 1d", bins, min, max);
  MonitorObject* targetMoTH1I = new MonitorObject(targetTH1I, "histo 1d", "class", "DET");
  targetMoTH1I->setIsOwner(true);
  target.Add(targetMoTH1I);
  targetMoTH1I->addOrUpdateMetadata(repository::metadata_keys::cycleNumber, "1");

  other.SetOwner(true);

  TH1I* otherTH1I = new TH1I("histo 1d", "histo 1d", bins, min, max);
  MonitorObject* otherMoTH1I = new MonitorObject(otherTH1I, "histo 1d", "class", "DET");
  otherMoTH1I->setIsOwner(true);
  other.Add(otherMoTH1I);
  otherMoTH1I->addOrUpdateMetadata(repository::metadata_keys::cycleNumber, "2");

  algorithm::merge(&target, &other);

  const auto mergedCycle = targetMoTH1I->getMetadata(repository::metadata_keys::cycleNumber);
  REQUIRE(mergedCycle.has_value());
  REQUIRE(mergedCycle.value() == "2");
}

} // namespace o2::quality_control::core
