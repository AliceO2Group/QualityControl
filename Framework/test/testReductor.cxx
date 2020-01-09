// Copyright CERN and copyright holders of ALICE O2. This software is
// distributed under the terms of the GNU General Public License v3 (GPL
// Version 3), copied verbatim in the file "COPYING".
//
// See http://alice-o2.web.cern.ch/license for full licensing information.
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.

///
/// \file    testReductors.cxx
/// \author  Piotr Konopka
///

#include "QualityControl/Reductor.h"
#include <TH1I.h>
#include <TTree.h>

#define BOOST_TEST_MODULE Reductor test
#define BOOST_TEST_MAIN
#define BOOST_TEST_DYN_LINK

#include <boost/test/unit_test.hpp>

using namespace o2::quality_control;
using namespace o2::quality_control::postprocessing;

class MyReductor : public Reductor
{
 public:
  MyReductor() = default;
  ~MyReductor() = default;

  void* getBranchAddress() override
  {
    return &mStats;
  }

  const char* getBranchLeafList() override
  {
    return "integral/D";
  }

  void update(TObject* obj) override
  {
    TH1I* histo = dynamic_cast<TH1I*>(obj);
    mStats.integral = histo->Integral();
  }

 private:
  struct {
    Double_t integral;
  } mStats;
};

BOOST_AUTO_TEST_CASE(test_ReductorInterface)
{
  auto histo = std::make_unique<TH1I>("test", "test", 10, 0, 1000);
  auto reductor = std::make_unique<MyReductor>();
  reductor->update(histo.get());

  auto tree = std::make_unique<TTree>();
  tree->Branch("histo", reductor->getBranchAddress(), reductor->getBranchLeafList());

  tree->Fill();
  histo->Fill(5);
  reductor->update(histo.get());
  tree->Fill();
  histo->Fill(1);
  reductor->update(histo.get());
  tree->Fill();
  histo->Fill(6);
  histo->Fill(66);
  histo->Fill(666);
  reductor->update(histo.get());
  tree->Fill();

  BOOST_REQUIRE_EQUAL(tree->GetEntries(), 4);
  tree->Draw("histo.integral", "", "goff");
  Double_t* integrals = tree->GetVal(0);

  BOOST_CHECK_EQUAL(integrals[0], 0);
  BOOST_CHECK_EQUAL(integrals[1], 1);
  BOOST_CHECK_EQUAL(integrals[2], 2);
  BOOST_CHECK_EQUAL(integrals[3], 5);
}