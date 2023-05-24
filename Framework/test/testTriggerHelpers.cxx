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
/// \file    testTriggerHelpers.cxx
/// \author  Piotr Konopka
///

#include "QualityControl/TriggerHelpers.h"
#include "QualityControl/PostProcessingConfig.h"
#include <catch_amalgamated.hpp>

using namespace o2::quality_control::postprocessing;
const std::string CCDB_ENDPOINT = "ccdb-test.cern.ch:8080";

TEST_CASE("test_factory")
{
  PostProcessingConfig dummyConfig;
  // check if it ignores letter case
  CHECK_NOTHROW(trigger_helpers::triggerFactory("once", dummyConfig));
  CHECK_NOTHROW(trigger_helpers::triggerFactory("Once", dummyConfig));
  CHECK_NOTHROW(trigger_helpers::triggerFactory("ONCE", dummyConfig));

  // check if it creates correct triggers
  auto once = trigger_helpers::triggerFactory("once", dummyConfig);
  CHECK(once() == TriggerType::Once);
  CHECK(once() == TriggerType::No);
  CHECK(once() == TriggerType::No);
  CHECK(once() == TriggerType::No);
  CHECK(once() == TriggerType::No);
  auto always = trigger_helpers::triggerFactory("always", dummyConfig);
  CHECK(always() == TriggerType::Always);
  CHECK(always() == TriggerType::Always);
  CHECK(always() == TriggerType::Always);
  CHECK(always() == TriggerType::Always);
  CHECK(always() == TriggerType::Always);

  // unknown trigger
  CHECK_THROWS_AS(trigger_helpers::triggerFactory("adsfzxcvadsf", dummyConfig), std::invalid_argument);
  CHECK_THROWS_AS(trigger_helpers::triggerFactory("", dummyConfig), std::invalid_argument);

  // generating periodic trigger
  CHECK_NOTHROW(trigger_helpers::triggerFactory("1sec", dummyConfig));
  CHECK_NOTHROW(trigger_helpers::triggerFactory("1.23sec", dummyConfig));
  CHECK_NOTHROW(trigger_helpers::triggerFactory("123 seconds", dummyConfig));
  CHECK_NOTHROW(trigger_helpers::triggerFactory("2min", dummyConfig));
  CHECK_NOTHROW(trigger_helpers::triggerFactory("2mins", dummyConfig));
  CHECK_NOTHROW(trigger_helpers::triggerFactory("2mins", dummyConfig));
  CHECK_NOTHROW(trigger_helpers::triggerFactory("2minutes", dummyConfig));
  CHECK_NOTHROW(trigger_helpers::triggerFactory("3hour", dummyConfig));
  CHECK_NOTHROW(trigger_helpers::triggerFactory("3hours", dummyConfig));
  CHECK_THROWS_AS(trigger_helpers::triggerFactory("-1sec", dummyConfig), std::invalid_argument);
  CHECK_THROWS_AS(trigger_helpers::triggerFactory("sec", dummyConfig), std::invalid_argument);
  CHECK_THROWS_AS(trigger_helpers::triggerFactory("asec", dummyConfig), std::invalid_argument);

  // generating new object trigger
  PostProcessingConfig configWithDBs;
  configWithDBs.qcdbUrl = CCDB_ENDPOINT;
  configWithDBs.ccdbUrl = CCDB_ENDPOINT;
  CHECK_NOTHROW(trigger_helpers::triggerFactory("newobject:qcdb:qc/asdf/vcxz", configWithDBs));
  CHECK_NOTHROW(trigger_helpers::triggerFactory("newobject:ccdb:qc/asdf/vcxz", configWithDBs));
  CHECK_NOTHROW(trigger_helpers::triggerFactory("newobject:QCDB:qc/asdf/vcxz", configWithDBs));
  CHECK_NOTHROW(trigger_helpers::triggerFactory("newobject:CCDB:qc/asdf/vcxz", configWithDBs));
  CHECK_NOTHROW(trigger_helpers::triggerFactory("NewObject:QcDb:qc/asdf/vcxz", configWithDBs));
  CHECK_THROWS_AS(trigger_helpers::triggerFactory("newobject", configWithDBs), std::invalid_argument);
  CHECK_THROWS_AS(trigger_helpers::triggerFactory("newobject:", configWithDBs), std::invalid_argument);
  CHECK_THROWS_AS(trigger_helpers::triggerFactory("newobject::", configWithDBs), std::invalid_argument);
  CHECK_THROWS_AS(trigger_helpers::triggerFactory("newobject::qc/no/db/specified", configWithDBs), std::invalid_argument);
  CHECK_THROWS_AS(trigger_helpers::triggerFactory("newobject:nodb:qc/incorrect/db/speficied", configWithDBs), std::invalid_argument);
  CHECK_THROWS_AS(trigger_helpers::triggerFactory("newobject:ccdb:qc/too:many tokens", configWithDBs), std::invalid_argument);

  CHECK_NOTHROW(trigger_helpers::triggerFactory("foreachobject:qcdb:qc/asdf/vcxz", configWithDBs));
  CHECK_NOTHROW(trigger_helpers::triggerFactory("foreachobject:ccdb:qc/asdf/vcxz", configWithDBs));
  CHECK_NOTHROW(trigger_helpers::triggerFactory("foreachobject:QCDB:qc/asdf/vcxz", configWithDBs));
  CHECK_NOTHROW(trigger_helpers::triggerFactory("foreachobject:CCDB:qc/asdf/vcxz", configWithDBs));
  CHECK_NOTHROW(trigger_helpers::triggerFactory("ForEachObject:QcDb:qc/asdf/vcxz", configWithDBs));
  CHECK_THROWS_AS(trigger_helpers::triggerFactory("foreachobject", configWithDBs), std::invalid_argument);
  CHECK_THROWS_AS(trigger_helpers::triggerFactory("foreachobject:", configWithDBs), std::invalid_argument);
  CHECK_THROWS_AS(trigger_helpers::triggerFactory("foreachobject::", configWithDBs), std::invalid_argument);
  CHECK_THROWS_AS(trigger_helpers::triggerFactory("foreachobject::qc/no/db/specified", configWithDBs), std::invalid_argument);
  CHECK_THROWS_AS(trigger_helpers::triggerFactory("foreachobject:nodb:qc/incorrect/db/speficied", configWithDBs), std::invalid_argument);
  CHECK_THROWS_AS(trigger_helpers::triggerFactory("foreachobject:ccdb:qc/too:many tokens", configWithDBs), std::invalid_argument);

  CHECK_NOTHROW(trigger_helpers::triggerFactory("foreachlatest:qcdb:qc/asdf/vcxz", configWithDBs));
  CHECK_NOTHROW(trigger_helpers::triggerFactory("foreachlatest:ccdb:qc/asdf/vcxz", configWithDBs));
  CHECK_NOTHROW(trigger_helpers::triggerFactory("foreachlatest:QCDB:qc/asdf/vcxz", configWithDBs));
  CHECK_NOTHROW(trigger_helpers::triggerFactory("foreachlatest:CCDB:qc/asdf/vcxz", configWithDBs));
  CHECK_NOTHROW(trigger_helpers::triggerFactory("ForEachLatest:QcDb:qc/asdf/vcxz", configWithDBs));
  CHECK_THROWS_AS(trigger_helpers::triggerFactory("foreachlatest", configWithDBs), std::invalid_argument);
  CHECK_THROWS_AS(trigger_helpers::triggerFactory("foreachlatest:", configWithDBs), std::invalid_argument);
  CHECK_THROWS_AS(trigger_helpers::triggerFactory("foreachlatest::", configWithDBs), std::invalid_argument);
  CHECK_THROWS_AS(trigger_helpers::triggerFactory("foreachlatest::qc/no/db/specified", configWithDBs), std::invalid_argument);
  CHECK_THROWS_AS(trigger_helpers::triggerFactory("foreachlatest:nodb:qc/incorrect/db/speficied", configWithDBs), std::invalid_argument);
  CHECK_THROWS_AS(trigger_helpers::triggerFactory("foreachlatest:ccdb:qc/too:many tokens", configWithDBs), std::invalid_argument);

  // check if config string is propagated
  CHECK(trigger_helpers::triggerFactory("10sec", dummyConfig)().config == "10sec");
  CHECK(trigger_helpers::triggerFactory("newobject:qcdb:qc/asdf/vcxz", configWithDBs)().config == "newobject:qcdb:qc/asdf/vcxz");
  CHECK(trigger_helpers::triggerFactory("foreachobject:qcdb:qc/asdf/vcxz", configWithDBs)().config == "foreachobject:qcdb:qc/asdf/vcxz");
  CHECK(trigger_helpers::triggerFactory("foreachlatest:qcdb:qc/asdf/vcxz", configWithDBs)().config == "foreachlatest:qcdb:qc/asdf/vcxz");

  // fixme: this is treated as "123 seconds", do we want to be so defensive?
  CHECK_NOTHROW(trigger_helpers::triggerFactory("123 secure code", dummyConfig));
}

TEST_CASE("test_create_trigger")
{
  PostProcessingConfig dummyConfig;

  CHECK(trigger_helpers::createTriggers({}, dummyConfig).size() == 0);
  CHECK(trigger_helpers::createTriggers({ "once", "always" }, dummyConfig).size() == 2);
}

TEST_CASE("test_try_triggers")
{
  PostProcessingConfig dummyConfig;
  {
    auto triggers = trigger_helpers::createTriggers({}, dummyConfig);
    CHECK(!trigger_helpers::tryTrigger(triggers));
    CHECK(!trigger_helpers::tryTrigger(triggers));
    CHECK(!trigger_helpers::tryTrigger(triggers));
    CHECK(!trigger_helpers::tryTrigger(triggers));
    CHECK(!trigger_helpers::tryTrigger(triggers));
  }

  {
    auto triggers = trigger_helpers::createTriggers({ "once" }, dummyConfig);
    CHECK(trigger_helpers::tryTrigger(triggers));
    CHECK(!trigger_helpers::tryTrigger(triggers));
    CHECK(!trigger_helpers::tryTrigger(triggers));
    CHECK(!trigger_helpers::tryTrigger(triggers));
    CHECK(!trigger_helpers::tryTrigger(triggers));
  }

  {
    auto triggers = trigger_helpers::createTriggers({ "once", "once", "once" }, dummyConfig);
    CHECK(trigger_helpers::tryTrigger(triggers));
    CHECK(trigger_helpers::tryTrigger(triggers));
    CHECK(trigger_helpers::tryTrigger(triggers));
    CHECK(!trigger_helpers::tryTrigger(triggers));
    CHECK(!trigger_helpers::tryTrigger(triggers));
    CHECK(!trigger_helpers::tryTrigger(triggers));
  }
}
