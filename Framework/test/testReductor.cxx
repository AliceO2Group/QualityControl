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
/// \file    testReductors.cxx
/// \author  Piotr Konopka
///

#include "QualityControl/Reductor.h"
#include "QualityControl/ReductorTObject.h"
#include "QualityControl/ReductorConditionAny.h"
#include "QualityControl/RootClassFactory.h"
#include "QualityControl/PostProcessingInterface.h"
#include "QualityControl/CcdbDatabase.h"
#include "QualityControl/ConditionAccess.h"
#include <TH1I.h>
#include <TTree.h>

#define BOOST_TEST_MODULE Reductor test
#define BOOST_TEST_MAIN
#define BOOST_TEST_DYN_LINK

#include <boost/test/unit_test.hpp>

const std::string CCDB_ENDPOINT = "ccdb-test.cern.ch:8080";

using namespace o2::quality_control;
using namespace o2::quality_control::core;
using namespace o2::quality_control::postprocessing;
using namespace o2::quality_control::repository;

class MyReductor : public ReductorTObject
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

BOOST_AUTO_TEST_CASE(test_ReductorTObjectInterface)
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

class MyReductorAny : public ReductorConditionAny
{
 public:
  MyReductorAny() = default;
  ~MyReductorAny() = default;

  void* getBranchAddress() override
  {
    return &mStats;
  }

  const char* getBranchLeafList() override
  {
    return "a/I";
  }

  bool update(ConditionRetriever& retriever) override
  {
    auto obj = retriever.retrieve<TString>();
    if (obj != nullptr) {
      mStats.a = obj->Length();
      return true;
    }
    std::cerr << "could not get the object from db" << std::endl;
    return false;
  }

 private:
  struct {
    Int_t a;
  } mStats;
};

std::string pathToTestObject()
{
  return "qc/TST/MO/TestReductor/pid" + std::to_string(getpid());
}

std::string fullTestObjectPath()
{
  return pathToTestObject() + "/string";
}

struct MyGlobalFixture {
  void teardown()
  {
    std::unique_ptr<CcdbDatabase> backend = std::make_unique<CcdbDatabase>();
    backend->connect(CCDB_ENDPOINT, "", "", "");
    // cannot use the test_fixture because we are tearing down
    backend->truncate(pathToTestObject(), "*");
  }
};
BOOST_TEST_GLOBAL_FIXTURE(MyGlobalFixture);

BOOST_AUTO_TEST_CASE(test_ReductorAnyInterface)
{
  std::unique_ptr<CcdbDatabase> backend = std::make_unique<CcdbDatabase>();
  backend->connect(CCDB_ENDPOINT, "", "", "");
  backend->truncate(pathToTestObject(), "*");
  TString secret = "1234567890";
  backend->storeAny(&secret, typeid(TString), fullTestObjectPath(), {}, "TST", "TestReductor", 1, 10);

  ConditionAccess conditionAccess;
  conditionAccess.setCcdbUrl(CCDB_ENDPOINT);

  std::unique_ptr<ReductorConditionAny> reductor = std::make_unique<MyReductorAny>();
  auto tree = std::make_unique<TTree>();
  tree->Branch("numbers", reductor->getBranchAddress(), reductor->getBranchLeafList());
  reductor->update(conditionAccess, 5, fullTestObjectPath());
  tree->Fill();
  BOOST_REQUIRE_EQUAL(tree->GetEntries(), 1);
  tree->Draw("numbers.a", "", "goff");
  Double_t* integrals = tree->GetVal(0);

  BOOST_CHECK_EQUAL(integrals[0], secret.Length());
}