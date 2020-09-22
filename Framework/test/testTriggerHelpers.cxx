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
/// \file    testTriggerHelpers.cxx
/// \author  Piotr Konopka
///

#include "QualityControl/TriggerHelpers.h"
#include "QualityControl/PostProcessingConfig.h"

#define BOOST_TEST_MODULE TriggerHelpers test
#define BOOST_TEST_MAIN
#define BOOST_TEST_DYN_LINK

#include <boost/test/unit_test.hpp>

using namespace o2::quality_control::postprocessing;
const std::string CCDB_ENDPOINT = "ccdb-test.cern.ch:8080";

BOOST_AUTO_TEST_CASE(test_factory)
{
  PostProcessingConfig dummyConfig;
  // check if it ignores letter case
  BOOST_CHECK_NO_THROW(trigger_helpers::triggerFactory("once", dummyConfig));
  BOOST_CHECK_NO_THROW(trigger_helpers::triggerFactory("Once", dummyConfig));
  BOOST_CHECK_NO_THROW(trigger_helpers::triggerFactory("ONCE", dummyConfig));

  // check if it creates correct triggers
  auto once = trigger_helpers::triggerFactory("once", dummyConfig);
  BOOST_CHECK_EQUAL(once(), TriggerType::Once);
  BOOST_CHECK_EQUAL(once(), TriggerType::No);
  BOOST_CHECK_EQUAL(once(), TriggerType::No);
  BOOST_CHECK_EQUAL(once(), TriggerType::No);
  BOOST_CHECK_EQUAL(once(), TriggerType::No);
  auto always = trigger_helpers::triggerFactory("always", dummyConfig);
  BOOST_CHECK_EQUAL(always(), TriggerType::Always);
  BOOST_CHECK_EQUAL(always(), TriggerType::Always);
  BOOST_CHECK_EQUAL(always(), TriggerType::Always);
  BOOST_CHECK_EQUAL(always(), TriggerType::Always);
  BOOST_CHECK_EQUAL(always(), TriggerType::Always);

  // unknown trigger
  BOOST_CHECK_THROW(trigger_helpers::triggerFactory("adsfzxcvadsf", dummyConfig), std::invalid_argument);
  BOOST_CHECK_THROW(trigger_helpers::triggerFactory("", dummyConfig), std::invalid_argument);

  // generating periodic trigger
  BOOST_CHECK_NO_THROW(trigger_helpers::triggerFactory("1sec", dummyConfig));
  BOOST_CHECK_NO_THROW(trigger_helpers::triggerFactory("1.23sec", dummyConfig));
  BOOST_CHECK_NO_THROW(trigger_helpers::triggerFactory("123 seconds", dummyConfig));
  BOOST_CHECK_NO_THROW(trigger_helpers::triggerFactory("2min", dummyConfig));
  BOOST_CHECK_NO_THROW(trigger_helpers::triggerFactory("2mins", dummyConfig));
  BOOST_CHECK_NO_THROW(trigger_helpers::triggerFactory("2mins", dummyConfig));
  BOOST_CHECK_NO_THROW(trigger_helpers::triggerFactory("2minutes", dummyConfig));
  BOOST_CHECK_NO_THROW(trigger_helpers::triggerFactory("3hour", dummyConfig));
  BOOST_CHECK_NO_THROW(trigger_helpers::triggerFactory("3hours", dummyConfig));
  BOOST_CHECK_THROW(trigger_helpers::triggerFactory("-1sec", dummyConfig), std::invalid_argument);
  BOOST_CHECK_THROW(trigger_helpers::triggerFactory("sec", dummyConfig), std::invalid_argument);
  BOOST_CHECK_THROW(trigger_helpers::triggerFactory("asec", dummyConfig), std::invalid_argument);

  // generating new object trigger
  PostProcessingConfig configWithDBs;
  configWithDBs.qcdbUrl = CCDB_ENDPOINT;
  configWithDBs.ccdbUrl = CCDB_ENDPOINT;
  BOOST_CHECK_NO_THROW(trigger_helpers::triggerFactory("newobject:qcdb:qc/asdf/vcxz", configWithDBs));
  BOOST_CHECK_NO_THROW(trigger_helpers::triggerFactory("newobject:ccdb:qc/asdf/vcxz", configWithDBs));
  BOOST_CHECK_NO_THROW(trigger_helpers::triggerFactory("newobject:QCDB:qc/asdf/vcxz", configWithDBs));
  BOOST_CHECK_NO_THROW(trigger_helpers::triggerFactory("newobject:CCDB:qc/asdf/vcxz", configWithDBs));
  BOOST_CHECK_NO_THROW(trigger_helpers::triggerFactory("NewObject:QcDb:qc/asdf/vcxz", configWithDBs));
  BOOST_CHECK_THROW(trigger_helpers::triggerFactory("newobject", configWithDBs), std::invalid_argument);
  BOOST_CHECK_THROW(trigger_helpers::triggerFactory("newobject:", configWithDBs), std::invalid_argument);
  BOOST_CHECK_THROW(trigger_helpers::triggerFactory("newobject::", configWithDBs), std::invalid_argument);
  BOOST_CHECK_THROW(trigger_helpers::triggerFactory("newobject::qc/no/db/specified", configWithDBs), std::invalid_argument);
  BOOST_CHECK_THROW(trigger_helpers::triggerFactory("newobject:nodb:qc/incorrect/db/speficied", configWithDBs), std::invalid_argument);
  BOOST_CHECK_THROW(trigger_helpers::triggerFactory("newobject:ccdb:qc/too:many tokens", configWithDBs), std::invalid_argument);

  // fixme: this is treated as "123 seconds", do we want to be so defensive?
  BOOST_CHECK_NO_THROW(trigger_helpers::triggerFactory("123 secure code", dummyConfig));
}

BOOST_AUTO_TEST_CASE(test_create_trigger)
{
  PostProcessingConfig dummyConfig;

  BOOST_CHECK_EQUAL(trigger_helpers::createTriggers({}, dummyConfig).size(), 0);
  BOOST_CHECK_EQUAL(trigger_helpers::createTriggers({ "once", "always" }, dummyConfig).size(), 2);
}

BOOST_AUTO_TEST_CASE(test_try_triggers)
{
  PostProcessingConfig dummyConfig;
  {
    auto triggers = trigger_helpers::createTriggers({}, dummyConfig);
    BOOST_CHECK(!trigger_helpers::tryTrigger(triggers));
    BOOST_CHECK(!trigger_helpers::tryTrigger(triggers));
    BOOST_CHECK(!trigger_helpers::tryTrigger(triggers));
    BOOST_CHECK(!trigger_helpers::tryTrigger(triggers));
    BOOST_CHECK(!trigger_helpers::tryTrigger(triggers));
  }

  {
    auto triggers = trigger_helpers::createTriggers({ "once" }, dummyConfig);
    BOOST_CHECK(trigger_helpers::tryTrigger(triggers));
    BOOST_CHECK(!trigger_helpers::tryTrigger(triggers));
    BOOST_CHECK(!trigger_helpers::tryTrigger(triggers));
    BOOST_CHECK(!trigger_helpers::tryTrigger(triggers));
    BOOST_CHECK(!trigger_helpers::tryTrigger(triggers));
  }

  {
    auto triggers = trigger_helpers::createTriggers({ "once", "once", "once" }, dummyConfig);
    BOOST_CHECK(trigger_helpers::tryTrigger(triggers));
    BOOST_CHECK(trigger_helpers::tryTrigger(triggers));
    BOOST_CHECK(trigger_helpers::tryTrigger(triggers));
    BOOST_CHECK(!trigger_helpers::tryTrigger(triggers));
    BOOST_CHECK(!trigger_helpers::tryTrigger(triggers));
    BOOST_CHECK(!trigger_helpers::tryTrigger(triggers));
  }
}