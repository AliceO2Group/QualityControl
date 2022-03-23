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

#include "QualityControl/Triggers.h"

#define BOOST_TEST_MODULE Triggers test
#define BOOST_TEST_MAIN
#define BOOST_TEST_DYN_LINK

#include "QualityControl/MonitorObject.h"
#include "QualityControl/DatabaseFactory.h"
#include "QualityControl/CcdbDatabase.h"
#include "QualityControl/RepoPathUtils.h"

#include <boost/test/unit_test.hpp>
#include <TH1F.h>
#include <cstdlib>
#include <chrono>
using namespace std::chrono;

using namespace o2::quality_control::postprocessing;
using namespace o2::quality_control::core;
using namespace o2::quality_control::repository;

const std::string CCDB_ENDPOINT = "ccdb-test.cern.ch:8080";

BOOST_AUTO_TEST_CASE(test_casting_triggers)
{
  auto once = triggers::Once();

  // confirm that enum values works
  BOOST_CHECK_EQUAL(once(), TriggerType::Once);
  BOOST_CHECK_EQUAL(once(), TriggerType::No);

  // confirm that casting to booleans works
  BOOST_CHECK_EQUAL(once(), false);
  BOOST_CHECK(!once());
  once = triggers::Once();
  BOOST_CHECK_EQUAL(once(), true);
  once = triggers::Once();
  BOOST_CHECK(once());
}

BOOST_AUTO_TEST_CASE(test_timestamps_triggers)
{
  Trigger t1(TriggerType::Once, false, 123);
  BOOST_CHECK_EQUAL(t1.triggerType, TriggerType::Once);
  BOOST_CHECK_EQUAL(t1.timestamp, 123);

  auto nowMs = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
  Trigger t2(TriggerType::Once);
  BOOST_CHECK_EQUAL(t2.triggerType, TriggerType::Once);
  BOOST_CHECK(std::abs(static_cast<long long>(t2.timestamp - nowMs)) < 100000); // 100 seconds of max. difference should be fine.
}

BOOST_AUTO_TEST_CASE(test_trigger_once)
{
  auto once = triggers::Once();

  BOOST_CHECK_EQUAL(once(), TriggerType::Once);
  BOOST_CHECK_EQUAL(once(), TriggerType::No);
  BOOST_CHECK_EQUAL(once(), TriggerType::No);
  BOOST_CHECK_EQUAL(once(), TriggerType::No);
  BOOST_CHECK_EQUAL(once(), TriggerType::No);
}

BOOST_AUTO_TEST_CASE(test_trigger_new_object)
{
  // Setup and initialise objects
  const std::string pid = std::to_string(getpid());
  const std::string detectorCode = "TST";
  const std::string taskName = "testTriggersNewObject";
  const std::string objectName = "test_object" + pid;

  TH1I* obj = new TH1I(objectName.c_str(), objectName.c_str(), 10, 0, 10.0);
  obj->Fill(4);
  std::shared_ptr<MonitorObject> mo = std::make_shared<MonitorObject>(obj, taskName, "TestClass", detectorCode);

  const std::string objectPath = RepoPathUtils::getMoPath(mo.get());
  auto newObjectTrigger = triggers::NewObject(CCDB_ENDPOINT, objectPath);

  // Clean up existing objects
  auto directDBAPI = std::make_shared<o2::ccdb::CcdbApi>();
  directDBAPI->init(CCDB_ENDPOINT);
  BOOST_REQUIRE(directDBAPI->isHostReachable());
  directDBAPI->truncate(objectPath);

  // Check before update - no objects expected
  BOOST_CHECK_EQUAL(newObjectTrigger(), TriggerType::No);
  BOOST_CHECK_EQUAL(newObjectTrigger(), TriggerType::No);
  BOOST_CHECK_EQUAL(newObjectTrigger(), TriggerType::No);

  // Send the object
  std::shared_ptr<DatabaseInterface> repository = DatabaseFactory::create("CCDB");
  repository->connect(CCDB_ENDPOINT, "", "", "");
  auto currentTimestamp = CcdbDatabase::getCurrentTimestamp();
  repository->storeMO(mo, currentTimestamp);

  // Check after sending
  BOOST_CHECK_EQUAL(newObjectTrigger(), Trigger(TriggerType::NewObject, currentTimestamp));
  BOOST_CHECK_EQUAL(newObjectTrigger(), TriggerType::No);
  BOOST_CHECK_EQUAL(newObjectTrigger(), TriggerType::No);

  // Update the object
  obj->Fill(10);
  repository->storeMO(mo, currentTimestamp);

  // Check after the update
  BOOST_CHECK_EQUAL(newObjectTrigger(), Trigger(TriggerType::NewObject, currentTimestamp));
  BOOST_CHECK_EQUAL(newObjectTrigger(), TriggerType::No);
  BOOST_CHECK_EQUAL(newObjectTrigger(), TriggerType::No);

  // Clean up remaining objects
  directDBAPI->truncate(objectPath);
}

BOOST_AUTO_TEST_CASE(test_trigger_for_each_object)
{
  // Setup and initialise objects
  const std::string pid = std::to_string(getpid());
  const std::string detectorCode = "TST";
  const std::string taskName = "testTriggersForEachObject";
  const std::string objectName = "test_object" + pid;

  TH1I* obj = new TH1I(objectName.c_str(), objectName.c_str(), 10, 0, 10.0);
  obj->Fill(4);
  std::shared_ptr<MonitorObject> mo = std::make_shared<MonitorObject>(obj, taskName, "TestClass", detectorCode);
  const std::string objectPath = RepoPathUtils::getMoPath(mo.get());

  // Clean up existing objects
  auto directDBAPI = std::make_shared<o2::ccdb::CcdbApi>();
  directDBAPI->init(CCDB_ENDPOINT);
  BOOST_REQUIRE(directDBAPI->isHostReachable());
  directDBAPI->truncate(objectPath);

  // Send three objects with different metadata
  std::shared_ptr<DatabaseInterface> repository = DatabaseFactory::create("CCDB");
  repository->connect(CCDB_ENDPOINT, "", "", "");
  auto currentTimestamp = CcdbDatabase::getCurrentTimestamp();
  mo->setActivity({ 100, 2, "FCC42x", "tpass1", "qc" });
  repository->storeMO(mo, currentTimestamp);
  mo->setActivity({ 101, 2, "FCC42x", "tpass1", "qc" });
  repository->storeMO(mo, currentTimestamp);
  mo->setActivity({ 100, 2, "FCC42x", "tpass2", "qc" });
  repository->storeMO(mo, currentTimestamp);

  {
    const Activity activityAllRunsPass1{ 0, 2, "FCC42x", "tpass1", "qc" };
    auto forEachObjectTrigger = triggers::ForEachObject(CCDB_ENDPOINT, objectPath, activityAllRunsPass1);

    BOOST_CHECK_EQUAL(forEachObjectTrigger(), TriggerType::ForEachObject);
    BOOST_CHECK_EQUAL(forEachObjectTrigger(), TriggerType::ForEachObject);
    BOOST_CHECK_EQUAL(forEachObjectTrigger(), TriggerType::No);
  }

  {
    const Activity activityRun100AllPasses{ 100, 2, "FCC42x", "", "qc" };
    auto forEachObjectTrigger = triggers::ForEachObject(CCDB_ENDPOINT, objectPath, activityRun100AllPasses);

    BOOST_CHECK_EQUAL(forEachObjectTrigger(), TriggerType::ForEachObject);
    BOOST_CHECK_EQUAL(forEachObjectTrigger(), TriggerType::ForEachObject);
    BOOST_CHECK_EQUAL(forEachObjectTrigger(), TriggerType::No);
  }

  {
    const Activity activityAll{ 0, 0, "", "", "qc" };
    auto forEachObjectTrigger = triggers::ForEachObject(CCDB_ENDPOINT, objectPath, activityAll);

    BOOST_CHECK_EQUAL(forEachObjectTrigger(), TriggerType::ForEachObject);
    BOOST_CHECK_EQUAL(forEachObjectTrigger(), TriggerType::ForEachObject);
    BOOST_CHECK_EQUAL(forEachObjectTrigger(), TriggerType::ForEachObject);
    BOOST_CHECK_EQUAL(forEachObjectTrigger(), TriggerType::No);
  }

  // Clean up remaining objects
  directDBAPI->truncate(objectPath);
}

BOOST_AUTO_TEST_CASE(test_trigger_for_each_latest)
{
  // Setup and initialise objects
  const std::string pid = std::to_string(getpid());
  const std::string detectorCode = "TST";
  const std::string taskName = "testTriggersForEachLatest";
  const std::string objectName = "test_object" + pid;

  TH1I* obj = new TH1I(objectName.c_str(), objectName.c_str(), 10, 0, 10.0);
  obj->Fill(4);
  std::shared_ptr<MonitorObject> mo = std::make_shared<MonitorObject>(obj, taskName, "TestClass", detectorCode);
  const std::string objectPath = RepoPathUtils::getMoPath(mo.get());

  // Clean up existing objects
  auto directDBAPI = std::make_shared<o2::ccdb::CcdbApi>();
  directDBAPI->init(CCDB_ENDPOINT);
  BOOST_REQUIRE(directDBAPI->isHostReachable());
  directDBAPI->truncate(objectPath);

  // Send three objects with different metadata
  std::shared_ptr<DatabaseInterface> repository = DatabaseFactory::create("CCDB");
  repository->connect(CCDB_ENDPOINT, "", "", "");
  mo->setActivity({ 100, 2, "FCC42x", "tpass1", "qc" });
  repository->storeMO(mo);
  usleep(1000);
  repository->storeMO(mo);
  mo->setActivity({ 101, 2, "FCC42x", "tpass1", "qc" });
  repository->storeMO(mo);
  usleep(1000);
  repository->storeMO(mo);
  mo->setActivity({ 100, 2, "FCC42x", "tpass2", "qc" });
  repository->storeMO(mo);

  {
    const Activity activityAllRunsPass1{ 0, 2, "FCC42x", "tpass1", "qc" };
    auto forEachObjectTrigger = triggers::ForEachLatest(CCDB_ENDPOINT, objectPath, activityAllRunsPass1);

    BOOST_CHECK_EQUAL(forEachObjectTrigger(), TriggerType::ForEachLatest);
    BOOST_CHECK_EQUAL(forEachObjectTrigger(), TriggerType::ForEachLatest);
    BOOST_CHECK_EQUAL(forEachObjectTrigger(), TriggerType::No);
  }

  {
    const Activity activityRun100AllPasses{ 100, 2, "FCC42x", "", "qc" };
    auto forEachObjectTrigger = triggers::ForEachLatest(CCDB_ENDPOINT, objectPath, activityRun100AllPasses);

    BOOST_CHECK_EQUAL(forEachObjectTrigger(), TriggerType::ForEachLatest);
    BOOST_CHECK_EQUAL(forEachObjectTrigger(), TriggerType::ForEachLatest);
    BOOST_CHECK_EQUAL(forEachObjectTrigger(), TriggerType::No);
  }

  {
    const Activity activityAll{ 0, 0, "", "", "qc" };
    auto forEachObjectTrigger = triggers::ForEachLatest(CCDB_ENDPOINT, objectPath, activityAll);

    BOOST_CHECK_EQUAL(forEachObjectTrigger(), TriggerType::ForEachLatest);
    BOOST_CHECK_EQUAL(forEachObjectTrigger(), TriggerType::ForEachLatest);
    BOOST_CHECK_EQUAL(forEachObjectTrigger(), TriggerType::ForEachLatest);
    BOOST_CHECK_EQUAL(forEachObjectTrigger(), TriggerType::No);
  }

  // Clean up remaining objects
  directDBAPI->truncate(objectPath);
}
