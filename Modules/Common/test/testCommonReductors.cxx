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
/// \file    testCommonReductors.cxx
/// \author  Piotr Konopka
///

#include "QualityControl/ReductorTObject.h"
#include "QualityControl/QualityObject.h"
#include "Common/TH1Reductor.h"
#include "Common/TH2Reductor.h"
#include "Common/QualityReductor.h"
#include <TH1I.h>
#include <TH2I.h>
#include <TTree.h>
#include <catch_amalgamated.hpp>

using namespace o2::quality_control;
using namespace o2::quality_control::core;
using namespace o2::quality_control::postprocessing;
using namespace o2::quality_control_modules::common;

TEST_CASE("test_TH1Reductor")
{
  auto histo = std::make_unique<TH1I>("test", "test", 10, 0, 10.0);
  auto reductor = std::make_unique<TH1Reductor>();

  auto tree = std::make_unique<TTree>();
  tree->Branch("histo", reductor->getBranchAddress(), reductor->getBranchLeafList());

  histo->Fill(5);
  reductor->update(histo.get());
  tree->Fill();
  histo->Fill(1);
  reductor->update(histo.get());
  tree->Fill();
  histo->Fill(6);
  histo->Fill(8);
  reductor->update(histo.get());
  tree->Fill();

  REQUIRE(tree->GetEntries() == 3);
  tree->Draw("histo.mean:histo.stddev:histo.entries", "", "goff");

  Double_t* means = tree->GetVal(0);
  CHECK_THAT(means[0], Catch::Matchers::WithinRel(5, 0.01 / 100));
  CHECK_THAT(means[1], Catch::Matchers::WithinRel(3, 0.01 / 100));
  CHECK_THAT(means[2], Catch::Matchers::WithinRel(5, 0.01 / 100));

  Double_t* stddevs = tree->GetVal(1);
  CHECK_THAT(stddevs[0], Catch::Matchers::WithinRel(0, 0.01 / 100));
  CHECK_THAT(stddevs[1], Catch::Matchers::WithinRel(2, 0.01 / 100));
  CHECK_THAT(stddevs[2], Catch::Matchers::WithinRel(2.55, 0.05 / 100));

  Double_t* entries = tree->GetVal(2);
  CHECK_THAT(entries[0], Catch::Matchers::WithinRel(1, 0.01 / 100));
  CHECK_THAT(entries[1], Catch::Matchers::WithinRel(2, 0.01 / 100));
  CHECK_THAT(entries[2], Catch::Matchers::WithinRel(4, 0.01 / 100));
}

TEST_CASE("test_TH2Reductor")
{
  auto histo = std::make_unique<TH2I>("test", "test", 10, 0.0, 10.0, 10, 0.0, 10.0);
  auto reductor = std::make_unique<TH2Reductor>();

  auto tree = std::make_unique<TTree>();
  tree->Branch("histo", reductor->getBranchAddress(), reductor->getBranchLeafList());

  histo->Fill(5, 5);
  reductor->update(histo.get());
  tree->Fill();
  histo->Fill(1, 1);
  reductor->update(histo.get());
  tree->Fill();
  histo->Fill(6, 6);
  histo->Fill(8, 8);
  reductor->update(histo.get());
  tree->Fill();

  REQUIRE(tree->GetEntries() == 3);
  tree->Draw("histo.sumw:histo.sumwxy:histo.entries", "", "goff");

  Double_t* sumws = tree->GetVal(0);
  CHECK_THAT(sumws[0], Catch::Matchers::WithinRel(1, 0.01 / 100));
  CHECK_THAT(sumws[1], Catch::Matchers::WithinRel(2, 0.01 / 100));
  CHECK_THAT(sumws[2], Catch::Matchers::WithinRel(4, 0.01 / 100));

  Double_t* sumwxys = tree->GetVal(1);
  CHECK_THAT(sumwxys[0], Catch::Matchers::WithinRel(25, 0.01 / 100));
  CHECK_THAT(sumwxys[1], Catch::Matchers::WithinRel(25 + 1, 0.01 / 100));
  CHECK_THAT(sumwxys[2], Catch::Matchers::WithinRel(25 + 1 + 36 + 64, 0.01 / 100));

  Double_t* entries = tree->GetVal(2);
  CHECK_THAT(entries[0], Catch::Matchers::WithinRel(1, 0.01 / 100));
  CHECK_THAT(entries[1], Catch::Matchers::WithinRel(2, 0.01 / 100));
  CHECK_THAT(entries[2], Catch::Matchers::WithinRel(4, 0.01 / 100));
}

TEST_CASE("test_QualityReductor")
{
  auto reductor = std::make_unique<QualityReductor>();

  auto tree = std::make_unique<TTree>();
  tree->Branch("quality", reductor->getBranchAddress(), reductor->getBranchLeafList());

  QualityObject qoBad(Quality::Bad, "check1");
  QualityObject qoMedium(Quality::Medium, "check1");
  QualityObject qoGood(Quality::Good, "check1");

  reductor->update(&qoBad);
  tree->Fill();
  reductor->update(&qoBad);
  reductor->update(&qoGood);
  tree->Fill();
  reductor->update(&qoGood);
  tree->Fill();
  reductor->update(&qoGood);
  reductor->update(&qoMedium);
  tree->Fill();

  REQUIRE(tree->GetEntries() == 4);
  tree->Draw("quality.level", "", "goff");

  Double_t* levels = tree->GetVal(0);
  CHECK_THAT(levels[0], Catch::Matchers::WithinRel(3, 0.01 / 100));
  CHECK_THAT(levels[1], Catch::Matchers::WithinRel(1, 0.01 / 100));
  CHECK_THAT(levels[2], Catch::Matchers::WithinRel(1, 0.01 / 100));
  CHECK_THAT(levels[3], Catch::Matchers::WithinRel(2, 0.01 / 100));

  struct {
    UInt_t level;
    char name[QualityReductor::NAME_SIZE];
  } qualityStats;
  tree->GetBranch("quality")->SetAddress(&qualityStats);

  tree->GetEntry(0);
  CHECK(!strncmp(qualityStats.name, "Bad", QualityReductor::NAME_SIZE));
  tree->GetEntry(1);
  CHECK(!strncmp(qualityStats.name, "Good", QualityReductor::NAME_SIZE));
  tree->GetEntry(2);
  CHECK(!strncmp(qualityStats.name, "Good", QualityReductor::NAME_SIZE));
  tree->GetEntry(3);
  CHECK(!strncmp(qualityStats.name, "Medium", QualityReductor::NAME_SIZE));
}