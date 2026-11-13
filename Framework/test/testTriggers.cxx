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
/// \file    testTriggers.cxx
/// \author  Piotr Konopka
///

#include "QualityControl/ObjectMetadataKeys.h"
#include "QualityControl/Triggers.h"

#include "QualityControl/MonitorObject.h"
#include "QualityControl/DatabaseFactory.h"
#include "QualityControl/CcdbDatabase.h"
#include "QualityControl/RepoPathUtils.h"

#include <CCDB/CcdbApi.h>
#include <TH1F.h>
#include <cstdlib>
#include <chrono>
#include <catch_amalgamated.hpp>
using namespace std::chrono;

using namespace o2::quality_control::postprocessing;
using namespace o2::quality_control::core;
using namespace o2::quality_control::repository;

// These tests upload, so this has to be a WRITABLE instance -- ccdb-test by
// default. ALICEO2_CCDB_HOST lets a network-isolated build container reach one
// through a broker instead; unset, behaviour is unchanged.
const std::string CCDB_ENDPOINT = [] {
  const char* host = std::getenv("ALICEO2_CCDB_HOST");
  return std::string((host && *host) ? host : "ccdb-test.cern.ch:8080");
}();

TEST_CASE("test_casting_triggers")
{
  auto once = triggers::Once();

  // confirm that enum values works
  CHECK(once() == TriggerType::Once);
  CHECK(once() == TriggerType::No);

  // confirm that casting to booleans works
  CHECK(once() == false);
  CHECK(!once());
  once = triggers::Once();
  CHECK(once() == true);
  once = triggers::Once();
  CHECK(once());
}

TEST_CASE("test_timestamps_triggers")
{
  Trigger t1(TriggerType::Once, false, 123);
  CHECK(t1.triggerType == TriggerType::Once);
  CHECK(t1.timestamp == 123);

  auto nowMs = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
  Trigger t2(TriggerType::Once);
  CHECK(t2.triggerType == TriggerType::Once);
  CHECK(std::abs(static_cast<long long>(t2.timestamp - nowMs)) < 100000); // 100 seconds of max. difference should be fine.
}

TEST_CASE("test_trigger_once")
{
  auto once = triggers::Once();

  CHECK(once() == TriggerType::Once);
  CHECK(once() == TriggerType::No);
  CHECK(once() == TriggerType::No);
  CHECK(once() == TriggerType::No);
  CHECK(once() == TriggerType::No);
}

TEST_CASE("test_trigger_new_object", "[ccdb]")
{
  // Setup and initialise objects
  const std::string pid = std::to_string(getpid());
  const std::string detectorCode = "TST";
  const std::string taskName = "testTriggersNewObject";
  const std::string objectName = "test_object" + pid;

  TH1I* obj = new TH1I(objectName.c_str(), objectName.c_str(), 10, 0, 10.0);
  obj->Fill(4);
  std::shared_ptr<MonitorObject> mo = std::make_shared<MonitorObject>(obj, taskName, "TestClass", detectorCode);

  const std::string fullObjectPath = RepoPathUtils::getMoPath(mo.get(), true);
  const std::string objectPath = RepoPathUtils::getMoPath(mo.get(), false);
  auto newObjectTrigger = triggers::NewObject(CCDB_ENDPOINT, "qcdb", objectPath);

  // Clean up existing objects
  auto directDBAPI = std::make_shared<o2::ccdb::CcdbApi>();
  directDBAPI->init(CCDB_ENDPOINT);
  REQUIRE(directDBAPI->isHostReachable());
  directDBAPI->truncate(fullObjectPath);

  // Check before update - no objects expected
  CHECK(newObjectTrigger() == TriggerType::No);
  CHECK(newObjectTrigger() == TriggerType::No);
  CHECK(newObjectTrigger() == TriggerType::No);

  // Send the object
  std::shared_ptr<DatabaseInterface> repository = DatabaseFactory::create("CCDB");
  repository->connect(CCDB_ENDPOINT, "", "", "");
  validity_time_t currentTimestamp = CcdbDatabase::getCurrentTimestamp();
  mo->setValidity({ currentTimestamp, gInvalidValidityInterval.getMax() });
  repository->storeMO(mo);

  // Check after sending
  CHECK(newObjectTrigger() == Trigger(TriggerType::NewObject, currentTimestamp));
  CHECK(newObjectTrigger() == TriggerType::No);
  CHECK(newObjectTrigger() == TriggerType::No);

  // Update the object
  obj->Fill(10);
  currentTimestamp = CcdbDatabase::getCurrentTimestamp();
  mo->setValidity({ currentTimestamp, gInvalidValidityInterval.getMax() });
  repository->storeMO(mo);

  // Check after the update
  CHECK(newObjectTrigger() == Trigger(TriggerType::NewObject, currentTimestamp));
  CHECK(newObjectTrigger() == TriggerType::No);
  CHECK(newObjectTrigger() == TriggerType::No);

  // Clean up remaining objects
  directDBAPI->truncate(fullObjectPath);
}

TEST_CASE("test_trigger_for_each_object", "[ccdb]")
{
  // Setup and initialise objects
  const std::string pid = std::to_string(getpid());
  const std::string detectorCode = "TST";
  const std::string taskName = "testTriggersForEachObject";
  const std::string objectName = "test_object" + pid;

  TH1I* obj = new TH1I(objectName.c_str(), objectName.c_str(), 10, 0, 10.0);
  obj->Fill(4);
  std::shared_ptr<MonitorObject> mo = std::make_shared<MonitorObject>(obj, taskName, "TestClass", detectorCode);
  const std::string fullObjectPath = RepoPathUtils::getMoPath(mo.get(), true);
  const std::string objectPath = RepoPathUtils::getMoPath(mo.get(), false);

  // Clean up existing objects
  auto directDBAPI = std::make_shared<o2::ccdb::CcdbApi>();
  directDBAPI->init(CCDB_ENDPOINT);
  REQUIRE(directDBAPI->isHostReachable());
  directDBAPI->truncate(fullObjectPath);

  // Send three objects with different metadata
  std::shared_ptr<DatabaseInterface> repository = DatabaseFactory::create("CCDB");
  repository->connect(CCDB_ENDPOINT, "", "", "");
  validity_time_t currentTimestamp = CcdbDatabase::getCurrentTimestamp();
  mo->setActivity({ 100, "TECHNICAL", "FCC42x", "tpass1", "qc", { currentTimestamp, gInvalidValidityInterval.getMax() } });
  repository->storeMO(mo);
  mo->setActivity({ 101, "TECHNICAL", "FCC42x", "tpass1", "qc", { currentTimestamp + 1000, gInvalidValidityInterval.getMax() } });
  repository->storeMO(mo);
  mo->setActivity({ 100, "TECHNICAL", "FCC42x", "tpass2", "qc", { currentTimestamp + 2000, gInvalidValidityInterval.getMax() } });
  mo->addOrUpdateMetadata(metadata_keys::cycleNumber, "42");
  repository->storeMO(mo);

  {
    const Activity activityAllRunsPass1{ 0, "TECHNICAL", "FCC42x", "tpass1", "qc" };
    auto forEachObjectTrigger = triggers::ForEachObject(CCDB_ENDPOINT, "qcdb", objectPath, activityAllRunsPass1);

    auto trigger = forEachObjectTrigger();
    CHECK(trigger == TriggerType::ForEachObject);
    CHECK(trigger.metadata.size() == 0);

    trigger = forEachObjectTrigger();
    CHECK(trigger == TriggerType::ForEachObject);
    CHECK(trigger.metadata.size() == 0);

    CHECK(forEachObjectTrigger() == TriggerType::No);
  }

  {
    const Activity activityRun100AllPasses{ 100, "TECHNICAL", "FCC42x", "", "qc" };
    auto forEachObjectTrigger = triggers::ForEachObject(CCDB_ENDPOINT, "qcdb", objectPath, activityRun100AllPasses);

    auto trigger = forEachObjectTrigger();
    CHECK(trigger == TriggerType::ForEachObject);
    CHECK(trigger.metadata.size() == 0);

    trigger = forEachObjectTrigger();
    CHECK(trigger == TriggerType::ForEachObject);
    CHECK(trigger.metadata.size() == 1);
    CHECK(trigger.metadata.at(metadata_keys::cycleNumber) == "42");

    CHECK(forEachObjectTrigger() == TriggerType::No);
  }

  {
    const Activity activityAll{ 0, "NONE", "", "", "qc" };
    auto forEachObjectTrigger = triggers::ForEachObject(CCDB_ENDPOINT, "qcdb", objectPath, activityAll);

    auto trigger = forEachObjectTrigger();
    CHECK(trigger == TriggerType::ForEachObject);
    CHECK(trigger.metadata.size() == 0);

    trigger = forEachObjectTrigger();
    CHECK(trigger == TriggerType::ForEachObject);
    CHECK(trigger.metadata.size() == 0);

    trigger = forEachObjectTrigger();
    CHECK(trigger == TriggerType::ForEachObject);
    CHECK(trigger.metadata.size() == 1);
    CHECK(trigger.metadata.at(metadata_keys::cycleNumber) == "42");

    CHECK(forEachObjectTrigger() == TriggerType::No);
  }

  {
    const Activity activityTimeRestriction{ 0, "NONE", "", "", "qc", { static_cast<validity_time_t>(currentTimestamp), static_cast<validity_time_t>(currentTimestamp + 5) } };
    auto forEachObjectTrigger = triggers::ForEachObject(CCDB_ENDPOINT, "qcdb", objectPath, activityTimeRestriction);

    CHECK(forEachObjectTrigger() == TriggerType::ForEachObject);
    CHECK(forEachObjectTrigger() == TriggerType::No);
  }
  // Clean up remaining objects
  directDBAPI->truncate(fullObjectPath);
}

TEST_CASE("test_trigger_for_each_latest", "[ccdb]")
{
  // Setup and initialise objects
  const std::string pid = std::to_string(getpid());
  const std::string detectorCode = "TST";
  const std::string taskName = "testTriggersForEachLatest";
  const std::string objectName = "test_object" + pid;

  TH1I* obj = new TH1I(objectName.c_str(), objectName.c_str(), 10, 0, 10.0);
  obj->Fill(4);
  std::shared_ptr<MonitorObject> mo = std::make_shared<MonitorObject>(obj, taskName, "TestClass", detectorCode);
  const std::string fullObjectPath = RepoPathUtils::getMoPath(mo.get(), true);
  const std::string objectPath = RepoPathUtils::getMoPath(mo.get(), false);

  // Clean up existing objects
  auto directDBAPI = std::make_shared<o2::ccdb::CcdbApi>();
  directDBAPI->init(CCDB_ENDPOINT);
  REQUIRE(directDBAPI->isHostReachable());
  directDBAPI->truncate(fullObjectPath);

  // Send three objects with different metadata
  std::shared_ptr<DatabaseInterface> repository = DatabaseFactory::create("CCDB");
  repository->connect(CCDB_ENDPOINT, "", "", "");
  mo->setActivity({ 100, "TECHNICAL", "FCC42x", "tpass1", "qc", gInvalidValidityInterval });
  repository->storeMO(mo);
  usleep(1000);
  repository->storeMO(mo);
  mo->setActivity({ 101, "TECHNICAL", "FCC42x", "tpass1", "qc", gInvalidValidityInterval });
  repository->storeMO(mo);
  usleep(1000);
  repository->storeMO(mo);
  mo->setActivity({ 100, "TECHNICAL", "FCC42x", "tpass2", "qc", gInvalidValidityInterval });
  repository->storeMO(mo);

  {
    const Activity activityAllRunsPass1{ 0, "TECHNICAL", "FCC42x", "tpass1", "qc" };
    auto forEachObjectTrigger = triggers::ForEachLatest(CCDB_ENDPOINT, "qcdb", objectPath, activityAllRunsPass1);

    CHECK(forEachObjectTrigger() == TriggerType::ForEachLatest);
    CHECK(forEachObjectTrigger() == TriggerType::ForEachLatest);
    CHECK(forEachObjectTrigger() == TriggerType::No);
  }

  {
    const Activity activityRun100AllPasses{ 100, "TECHNICAL", "FCC42x", "", "qc" };
    auto forEachObjectTrigger = triggers::ForEachLatest(CCDB_ENDPOINT, "qcdb", objectPath, activityRun100AllPasses);

    CHECK(forEachObjectTrigger() == TriggerType::ForEachLatest);
    CHECK(forEachObjectTrigger() == TriggerType::ForEachLatest);
    CHECK(forEachObjectTrigger() == TriggerType::No);
  }

  {
    const Activity activityAll{ 0, "NONE", "", "", "qc" };
    auto forEachObjectTrigger = triggers::ForEachLatest(CCDB_ENDPOINT, "qcdb", objectPath, activityAll);

    CHECK(forEachObjectTrigger() == TriggerType::ForEachLatest);
    CHECK(forEachObjectTrigger() == TriggerType::ForEachLatest);
    CHECK(forEachObjectTrigger() == TriggerType::ForEachLatest);
    CHECK(forEachObjectTrigger() == TriggerType::No);
  }

  // Clean up remaining objects
  directDBAPI->truncate(fullObjectPath);
}
