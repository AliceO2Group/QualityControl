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

#include "QualityControl/Reductor.h"
#include "QualityControl/QualityObject.h"
#include "Common/TH1Reductor.h"
#include "Common/TH2Reductor.h"
#include "Common/QualityReductor.h"
#include <TH1I.h>
#include <TH2I.h>
#include <TTree.h>

#define BOOST_TEST_MODULE CommonReductors test
#define BOOST_TEST_MAIN
#define BOOST_TEST_DYN_LINK

#include <boost/test/unit_test.hpp>

using namespace o2::quality_control;
using namespace o2::quality_control::core;
using namespace o2::quality_control::postprocessing;
using namespace o2::quality_control_modules::common;

BOOST_AUTO_TEST_CASE(test_TH1Reductor)
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

  BOOST_REQUIRE_EQUAL(tree->GetEntries(), 3);
  tree->Draw("histo.mean:histo.stddev:histo.entries", "", "goff");

  Double_t* means = tree->GetVal(0);
  BOOST_CHECK_CLOSE(means[0], 5, 0.01);
  BOOST_CHECK_CLOSE(means[1], 3, 0.01);
  BOOST_CHECK_CLOSE(means[2], 5, 0.01);

  Double_t* stddevs = tree->GetVal(1);
  BOOST_CHECK_CLOSE(stddevs[0], 0, 0.01);
  BOOST_CHECK_CLOSE(stddevs[1], 2, 0.01);
  BOOST_CHECK_CLOSE(stddevs[2], 2.55, 0.05);

  Double_t* entries = tree->GetVal(2);
  BOOST_CHECK_CLOSE(entries[0], 1, 0.01);
  BOOST_CHECK_CLOSE(entries[1], 2, 0.01);
  BOOST_CHECK_CLOSE(entries[2], 4, 0.01);
}

BOOST_AUTO_TEST_CASE(test_TH2Reductor)
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

  BOOST_REQUIRE_EQUAL(tree->GetEntries(), 3);
  tree->Draw("histo.sumw:histo.sumwxy:histo.entries", "", "goff");

  Double_t* sumws = tree->GetVal(0);
  BOOST_CHECK_CLOSE(sumws[0], 1, 0.01);
  BOOST_CHECK_CLOSE(sumws[1], 2, 0.01);
  BOOST_CHECK_CLOSE(sumws[2], 4, 0.01);

  Double_t* sumwxys = tree->GetVal(1);
  BOOST_CHECK_CLOSE(sumwxys[0], 25, 0.01);
  BOOST_CHECK_CLOSE(sumwxys[1], 25 + 1, 0.01);
  BOOST_CHECK_CLOSE(sumwxys[2], 25 + 1 + 36 + 64, 0.01);

  Double_t* entries = tree->GetVal(2);
  BOOST_CHECK_CLOSE(entries[0], 1, 0.01);
  BOOST_CHECK_CLOSE(entries[1], 2, 0.01);
  BOOST_CHECK_CLOSE(entries[2], 4, 0.01);
}

BOOST_AUTO_TEST_CASE(test_QualityReductor)
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

  BOOST_REQUIRE_EQUAL(tree->GetEntries(), 4);
  tree->Draw("quality.level", "", "goff");

  Double_t* levels = tree->GetVal(0);
  BOOST_CHECK_CLOSE(levels[0], 3, 0.01);
  BOOST_CHECK_CLOSE(levels[1], 1, 0.01);
  BOOST_CHECK_CLOSE(levels[2], 1, 0.01);
  BOOST_CHECK_CLOSE(levels[3], 2, 0.01);

  struct {
    UInt_t level;
    char name[QualityReductor::NAME_SIZE];
  } qualityStats;
  tree->GetBranch("quality")->SetAddress(&qualityStats);

  tree->GetEntry(0);
  BOOST_CHECK(!strncmp(qualityStats.name, "Bad", QualityReductor::NAME_SIZE));
  tree->GetEntry(1);
  BOOST_CHECK(!strncmp(qualityStats.name, "Good", QualityReductor::NAME_SIZE));
  tree->GetEntry(2);
  BOOST_CHECK(!strncmp(qualityStats.name, "Good", QualityReductor::NAME_SIZE));
  tree->GetEntry(3);
  BOOST_CHECK(!strncmp(qualityStats.name, "Medium", QualityReductor::NAME_SIZE));
}