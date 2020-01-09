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
/// \file    testTrendingTask.cxx
/// \author  Piotr Konopka
///

#include "getTestDataDirectory.h"
#include "QualityControl/TrendingTask.h"
#include "QualityControl/DatabaseFactory.h"
#include "QualityControl/MonitorObject.h"
#include "QualityControl/Triggers.h"
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

  // Putting the objects to trend into the database
  std::shared_ptr<DatabaseInterface> repository = DatabaseFactory::create("CCDB");
  repository->connect(CCDB_ENDPOINT, "", "", "");

  TH1I* histo = new TH1I("testHistoTrending", "testHistoTrending", 10, 0, 10.0);
  histo->Fill(4);
  histo->Fill(5);
  histo->Fill(6);
  std::shared_ptr<MonitorObject> mo = std::make_shared<MonitorObject>(histo, taskName, "TST");
  repository->storeMO(mo);

  std::shared_ptr<QualityObject> qo = std::make_shared<QualityObject>("testTrendingTaskCheck");
  qo->updateQuality(Quality::Bad);
  repository->storeQO(qo);

  ServiceRegistry services;
  services.registerService<DatabaseInterface>(repository.get());

  // Running the task
  TrendingTask task;
  task.setName(taskName);
  task.configure(taskName, *ConfigurationFactory::getConfiguration(configFilePath));
  task.initialize(Trigger::Once, services);
  for (size_t i = 0; i < trendTimes; i++) {
    task.update(Trigger::Always, services);
  }
  task.finalize(Trigger::UserExit, services);

  // The test itself
  auto treeMO = repository->retrieveMO("qc/TST/" + taskName, taskName); //the tree is stored under the same name as task
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