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
/// \file    testTrendingTask.cxx
/// \author  Piotr Konopka
///

#include "getTestDataDirectory.h"
#include "QualityControl/TrendingTask.h"
#include "QualityControl/DatabaseFactory.h"
#include "QualityControl/MonitorObject.h"
#include "QualityControl/Triggers.h"
#include "QualityControl/PostProcessingRunner.h"
#include <Framework/ServiceRegistry.h>

#include <Configuration/ConfigurationFactory.h>
#include <TH1I.h>

#define BOOST_TEST_MODULE TrendingTask test
#define BOOST_TEST_MAIN
#define BOOST_TEST_DYN_LINK

#include <boost/test/unit_test.hpp>

using namespace o2::quality_control::core;
using namespace o2::quality_control::postprocessing;
using namespace o2::quality_control::repository;
using namespace o2::configuration;
using namespace o2::framework;

const std::string CCDB_ENDPOINT = "ccdb-test.cern.ch:8080";

// WARNING!
// This test might not pass if run concurrently - it interacts with a common CCDB instance.
BOOST_AUTO_TEST_CASE(test_task)
{
  const std::string configFilePath = std::string("json://") + getTestDataDirectory() + "testTrendingTask.json";
  const std::string taskName = "TestTrendingTask";
  const size_t trendTimes = 5;

  std::shared_ptr<DatabaseInterface> repository = DatabaseFactory::create("CCDB");
  repository->connect(CCDB_ENDPOINT, "", "", "");

  // Putting the objects to trend into the database
  {
    TH1I* histo = new TH1I("testHistoTrending", "testHistoTrending", 10, 0, 10.0);
    histo->Fill(4);
    histo->Fill(5);
    histo->Fill(6);
    std::shared_ptr<MonitorObject> mo = std::make_shared<MonitorObject>(histo, taskName, "TestClass", "TST");
    repository->storeMO(mo, 1, 100000);

    std::shared_ptr<QualityObject> qo = std::make_shared<QualityObject>(Quality::Null, "testTrendingTaskCheck", "TST");
    qo->updateQuality(Quality::Bad);
    repository->storeQO(qo, 1, 100000);
  }

  // We make sure, that destroy the previous, possibly correct test result
  {
    TTree* dummyTree = new TTree(taskName.c_str(), taskName.c_str());
    repository->storeMO(std::make_shared<MonitorObject>(dummyTree, taskName, "TestClass", "TST"));
    auto treeMO = repository->retrieveMO("TST/MO/" + taskName, taskName);
    BOOST_REQUIRE(treeMO != nullptr);
    TTree* treeFromRepo = dynamic_cast<TTree*>(treeMO->getObject());
    BOOST_REQUIRE(treeFromRepo != nullptr);
    BOOST_REQUIRE_EQUAL(treeFromRepo->GetEntries(), 0);
  }

  // Running the task
  {
    ServiceRegistry services;
    services.registerService<DatabaseInterface>(repository.get());
    auto objectManager = std::make_shared<ObjectsManager>(taskName, "o2::quality_control::postprocessing::TrendingTask", "TST", "");
    auto publicationCallback = publishToRepository(*repository);

    TrendingTask task;
    task.setName(taskName);
    task.setObjectsManager(objectManager);
    task.configure(taskName, ConfigurationFactory::getConfiguration(configFilePath)->getRecursive());
    task.initialize({ TriggerType::Once, false, { 0, 0, "", "", "qc"}, 1 }, services);
    for (size_t i = 0; i < trendTimes; i++) {
      task.update({ TriggerType::Always, false, { 0, 0, "", "", "qc" }, i * 1000 + 50 }, services);
      publicationCallback(objectManager->getNonOwningArray(), i * 1000, i * 1000 + 100);
    }
    task.finalize({ TriggerType::UserOrControl, false, {0, 0, "", "", "qc"}, trendTimes * 1000 }, services);
  }

  // The test itself
  {
    auto treeMO = repository->retrieveMO("TST/MO/" + taskName, taskName, (trendTimes - 1) * 1000 + 5); // the tree is stored under the same name as task
    BOOST_REQUIRE(treeMO != nullptr);
    TTree* tree = dynamic_cast<TTree*>(treeMO->getObject());
    BOOST_REQUIRE(tree != nullptr);

    BOOST_REQUIRE_EQUAL(tree->GetEntries(), trendTimes);
    tree->Draw("testHistoTrending.mean:testHistoTrending.entries:testTrendingTaskCheck.level", "", "goff");

    Double_t* means = tree->GetVal(0);
    Double_t* entries = tree->GetVal(1);
    Double_t* qualityLevels = tree->GetVal(2);
    for (size_t i = 0; i < trendTimes; i++) {
      BOOST_CHECK_CLOSE(means[i], 5, 0.01);
      BOOST_CHECK_CLOSE(entries[i], 3, 0.01);
      BOOST_CHECK_CLOSE(qualityLevels[i], 3, 0.01);
    }
  }
}