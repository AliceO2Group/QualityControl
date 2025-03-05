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

#include "Framework/include/QualityControl/Reductor.h"
#include "QualityControl/TrendingTask.h"
#include "QualityControl/DatabaseFactory.h"
#include "QualityControl/MonitorObject.h"
#include "QualityControl/Triggers.h"

#include <Framework/ServiceRegistry.h>
#include <TH1I.h>
#include <TCanvas.h>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <sstream>
#include <utility>
#include <functional>

#include <catch_amalgamated.hpp>

using namespace o2::quality_control::core;
using namespace o2::quality_control::postprocessing;
using namespace o2::quality_control::repository;
using namespace o2::framework;

const std::string CCDB_ENDPOINT = "ccdb-test.cern.ch:8080";

struct CleanupAtDestruction {
 public:
  CleanupAtDestruction(std::function<void()> callback) : mCallback(std::move(callback)) {}
  ~CleanupAtDestruction()
  {
    if (mCallback) {
      mCallback();
    }
  }

 private:
  std::function<void()> mCallback = nullptr;
};

// https://stackoverflow.com/questions/424104/can-i-access-private-members-from-outside-the-class-without-using-friends
template <typename Accessor, typename Accessor::type Member>
struct DeclareGlobalGet {
  friend typename Accessor::type get(Accessor) { return Member; }
};

struct TrendingTaskReductorAccessor {
  using type = std::unordered_map<std::string, std::unique_ptr<Reductor>> TrendingTask::*;
  friend type get(TrendingTaskReductorAccessor);
};

struct ReductorConfigAccessor {
  using type = CustomParameters Reductor::*;
  friend type get(ReductorConfigAccessor);
};

template struct DeclareGlobalGet<TrendingTaskReductorAccessor, &TrendingTask::mReductors>;
template struct DeclareGlobalGet<ReductorConfigAccessor, &Reductor::mCustomParameters>;

TEST_CASE("test_trending_task")
{
  const std::string pid = std::to_string(getpid());
  const std::string trendingTaskName = "TestTrendingTask";
  const std::string trendingTaskID = "TSTTrendingTask";
  const std::string taskName = "TrendingTaskTest" + pid;  // this is the QC task we want to trend
  const std::string checkName = "TrendingTaskTest" + pid; // this is the QC check we want to trend
  const size_t trendTimes = 5;

  std::stringstream ss;
  ss << R"json({
  "qc": {
    "config": {
      "database": {
        "implementation": "CCDB",
        "host": "ccdb-test.cern.ch:8080"
      },
      "Activity": {},
      "monitoring": {
        "url": "infologger:///debug?qc"
      }
    },
    "postprocessing": {
      "TSTTrendingTask": {
        "active": "true",
        "taskName": "TestTrendingTask",
        "className": "o2::quality_control::postprocessing::TrendingTask",
        "trendIfAllInputs": true,
        "moduleName": "QualityControl",
        "detectorName": "TST",
        "dataSources": [
          {
            "type": "repository",
            "path": "TST/MO/)json" +
          taskName + R"json(",
            "name": "testHistoTrending",
            "reductorName": "o2::quality_control_modules::common::TH1Reductor",
            "reductorParameters": {
              "default": {
                "default": {
                  "key":"value"
                }
              }
            },
            "moduleName": "QcCommon"
          },
          {
            "type": "repository-quality",
            "path": "TST/QO",
            "names": [ ")json" +
          checkName + R"json(" ],
            "reductorName": "o2::quality_control_modules::common::QualityReductor",
            "moduleName": "QcCommon"
          }
        ],
        "plots": [
          {
            "name": "mean_of_histogram",
            "title": "Mean trend of the testHistoTrending histogram",
            "graphs": [{
              "varexp": "testHistoTrending.mean:time",
              "selection": "",
              "option": "*L"
            }]
          },
          {
            "name": "quality_histogram",
            "title": "Histogram of qualities",
            "varexp": ")json" +
          checkName + R"json(.level",
            "selection": "",
            "option": ""
          }
        ],
        "initTrigger": [],
        "updateTrigger": [],
        "stopTrigger": []
      }
    }
  }
})json";
  boost::property_tree::ptree config;
  boost::property_tree::read_json(ss, config);

  // clean
  std::shared_ptr<DatabaseInterface> repository = DatabaseFactory::create("CCDB");
  repository->connect(CCDB_ENDPOINT, "", "", "");
  repository->truncate("qc/TST/MO/" + taskName, "*");
  repository->truncate("qc/TST/QO", checkName);

  // Test "trendIfAllInputs". There should not be anything in DB so we don't have any input sources available
  {
    auto objectManager = std::make_shared<ObjectsManager>(taskName, "o2::quality_control::postprocessing::TrendingTask", "TST", "");
    ServiceRegistry services;
    services.registerService<DatabaseInterface>(repository.get());

    TrendingTask task;
    task.setName(trendingTaskName);
    task.setID(trendingTaskID);
    task.setObjectsManager(objectManager);
    REQUIRE_NOTHROW(task.configure(config));

    {
      auto& reductors = task.*get(TrendingTaskReductorAccessor());
      size_t foundCount{};
      for (const auto& reductor : reductors) {
        auto& config = (*reductor.second.get()).*get(ReductorConfigAccessor());
        if (auto found = config.find("key"); found != config.end()) {
          if (found->second == "value") {
            foundCount++;
          }
        }
      }
      REQUIRE(foundCount == 1);
    }

    // test initialize()
    REQUIRE_NOTHROW(task.initialize({ TriggerType::UserOrControl, true, { 0, "NONE", "", "", "qc" }, 1 }, services));
    REQUIRE(objectManager->getNumberPublishedObjects() == 1);
    auto treeMO = objectManager->getMonitorObject(trendingTaskName);
    REQUIRE(treeMO != nullptr);
    TTree* tree = dynamic_cast<TTree*>(treeMO->getObject());
    REQUIRE(tree != nullptr);
    REQUIRE(tree->GetEntries() == 0);

    // test update()
    task.update({ TriggerType::NewObject, false, { 0, "NONE", "", "", "qc", { 2, 100000 } }, 100000 - 1 }, services);
    objectManager->stopPublishing(PublicationPolicy::Once);
    task.update({ TriggerType::NewObject, false, { 0, "NONE", "", "", "qc", { 100000, 200000 } }, 200000 - 1 }, services);
    REQUIRE(objectManager->getNumberPublishedObjects() == 1);
    REQUIRE(tree->GetEntries() == 0);
  }

  // Putting the objects to trend into the database
  {
    TH1I* histo = new TH1I("testHistoTrending", "testHistoTrending", 10, 0, 10.0);
    histo->Fill(4);
    histo->Fill(5);
    histo->Fill(6);
    std::shared_ptr<MonitorObject> mo = std::make_shared<MonitorObject>(histo, taskName, "TestClass", "TST");
    mo->setValidity({ 2, 100000 });
    repository->storeMO(mo);

    histo->Fill(5);
    mo->setValidity({ 100001, 200000 });
    repository->storeMO(mo);

    std::shared_ptr<QualityObject> qo = std::make_shared<QualityObject>(Quality::Null, checkName, "TST");
    qo->updateQuality(Quality::Bad);
    qo->setValidity({ 2, 100000 });
    repository->storeQO(qo);
    qo->updateQuality(Quality::Good);
    qo->setValidity({ 100001, 200000 });
    repository->storeQO(qo);
  }

  // from here on, we clean up the database if we exit
  CleanupAtDestruction cleanTestObjects([repository, taskName, checkName]() {
    if (repository) {
      repository->truncate("qc/TST/MO/" + taskName, "*");
      repository->truncate("qc/TST/QO", checkName);
    }
  });

  // Running the task
  ServiceRegistry services;
  services.registerService<DatabaseInterface>(repository.get());
  auto objectManager = std::make_shared<ObjectsManager>(taskName, "o2::quality_control::postprocessing::TrendingTask", "TST", "");

  TrendingTask task;
  task.setName(trendingTaskName);
  task.setID(trendingTaskID);
  task.setObjectsManager(objectManager);
  REQUIRE_NOTHROW(task.configure(config));

  // test initialize()
  REQUIRE_NOTHROW(task.initialize({ TriggerType::UserOrControl, true, { 0, "NONE", "", "", "qc" }, 1 }, services));
  REQUIRE(objectManager->getNumberPublishedObjects() == 1);
  auto treeMO = objectManager->getMonitorObject(trendingTaskName);
  REQUIRE(treeMO != nullptr);
  TTree* tree = dynamic_cast<TTree*>(treeMO->getObject());
  REQUIRE(tree != nullptr);
  REQUIRE(tree->GetEntries() == 0);

  // test update()
  task.update({ TriggerType::NewObject, false, { 0, "NONE", "", "", "qc", { 2, 100000 } }, 100000 - 1 }, services);
  objectManager->stopPublishing(PublicationPolicy::Once);
  task.update({ TriggerType::NewObject, false, { 0, "NONE", "", "", "qc", { 100000, 200000 } }, 200000 - 1 }, services);
  REQUIRE(objectManager->getNumberPublishedObjects() == 3);
  REQUIRE(tree->GetEntries() == 2);
  auto varexp = "testHistoTrending.mean:testHistoTrending.entries:" + checkName + ".level";
  tree->Draw(varexp.c_str(), "", "goff");
  Double_t* means = tree->GetVal(0);
  Double_t* entries = tree->GetVal(1);
  Double_t* qualityLevels = tree->GetVal(2);
  CHECK_THAT(means[0], Catch::Matchers::WithinAbs(5, 0.01));
  CHECK_THAT(entries[0], Catch::Matchers::WithinAbs(3, 0.01));
  CHECK_THAT(qualityLevels[0], Catch::Matchers::WithinAbs(3, 0.01));
  CHECK_THAT(means[1], Catch::Matchers::WithinAbs(5, 0.01));
  CHECK_THAT(entries[1], Catch::Matchers::WithinAbs(4, 0.01));
  CHECK_THAT(qualityLevels[1], Catch::Matchers::WithinAbs(1, 0.01));

  auto histo1MO = objectManager->getMonitorObject("mean_of_histogram");
  REQUIRE(histo1MO != nullptr);
  auto histo1 = dynamic_cast<TCanvas*>(histo1MO->getObject());
  REQUIRE(histo1 != nullptr);
  CHECK(std::strcmp(histo1->GetName(), "mean_of_histogram") == 0);
  auto histo2MO = objectManager->getMonitorObject("quality_histogram");
  REQUIRE(histo2MO != nullptr);
  auto histo2 = dynamic_cast<TCanvas*>(histo2MO->getObject());
  REQUIRE(histo2 != nullptr);
  CHECK(std::strcmp(histo2->GetName(), "quality_histogram") == 0);
  objectManager->stopPublishing(PublicationPolicy::Once);

  // test finalize()
  REQUIRE_NOTHROW(task.finalize({ TriggerType::UserOrControl, true }, services));
  REQUIRE(objectManager->getNumberPublishedObjects() == 3);
  REQUIRE(tree->GetEntries() == 2);
  objectManager->stopPublishing(PublicationPolicy::Once);
  objectManager->stopPublishing(PublicationPolicy::ThroughStop);
}
