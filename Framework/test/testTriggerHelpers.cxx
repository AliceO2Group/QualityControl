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

#define BOOST_TEST_MODULE TriggerHelpers test
#define BOOST_TEST_MAIN
#define BOOST_TEST_DYN_LINK

#include <boost/test/unit_test.hpp>

using namespace o2::quality_control::postprocessing;

BOOST_AUTO_TEST_CASE(test_factory)
{
  // check if it ignores letter case
  BOOST_CHECK_NO_THROW(trigger_helpers::triggerFactory("once"));
  BOOST_CHECK_NO_THROW(trigger_helpers::triggerFactory("Once"));
  BOOST_CHECK_NO_THROW(trigger_helpers::triggerFactory("ONCE"));

  // check if it creates correct triggers
  auto once = trigger_helpers::triggerFactory("once");
  BOOST_CHECK_EQUAL(once(), Trigger::Once);
  BOOST_CHECK_EQUAL(once(), Trigger::No);
  BOOST_CHECK_EQUAL(once(), Trigger::No);
  BOOST_CHECK_EQUAL(once(), Trigger::No);
  BOOST_CHECK_EQUAL(once(), Trigger::No);
  auto always = trigger_helpers::triggerFactory("always");
  BOOST_CHECK_EQUAL(always(), Trigger::Always);
  BOOST_CHECK_EQUAL(always(), Trigger::Always);
  BOOST_CHECK_EQUAL(always(), Trigger::Always);
  BOOST_CHECK_EQUAL(always(), Trigger::Always);
  BOOST_CHECK_EQUAL(always(), Trigger::Always);

  // unknown trigger
  BOOST_CHECK_THROW(trigger_helpers::triggerFactory("adsfzxcvadsf"), std::invalid_argument);
  BOOST_CHECK_THROW(trigger_helpers::triggerFactory(""), std::invalid_argument);

  // generating periodic trigger
  BOOST_CHECK_NO_THROW(trigger_helpers::triggerFactory("1sec"));
  BOOST_CHECK_NO_THROW(trigger_helpers::triggerFactory("1.23sec"));
  BOOST_CHECK_NO_THROW(trigger_helpers::triggerFactory("123 seconds"));
  BOOST_CHECK_NO_THROW(trigger_helpers::triggerFactory("2min"));
  BOOST_CHECK_NO_THROW(trigger_helpers::triggerFactory("2mins"));
  BOOST_CHECK_NO_THROW(trigger_helpers::triggerFactory("2mins"));
  BOOST_CHECK_NO_THROW(trigger_helpers::triggerFactory("2minutes"));
  BOOST_CHECK_NO_THROW(trigger_helpers::triggerFactory("3hour"));
  BOOST_CHECK_NO_THROW(trigger_helpers::triggerFactory("3hours"));
  BOOST_CHECK_THROW(trigger_helpers::triggerFactory("-1sec"), std::invalid_argument);
  BOOST_CHECK_THROW(trigger_helpers::triggerFactory("sec"), std::invalid_argument);
  BOOST_CHECK_THROW(trigger_helpers::triggerFactory("asec"), std::invalid_argument);

  // fixme: this is treated as "123 seconds", do we want to be so defensive?
  BOOST_CHECK_NO_THROW(trigger_helpers::triggerFactory("123 secure code"));
}

BOOST_AUTO_TEST_CASE(test_create_trigger)
{
  BOOST_CHECK_EQUAL(trigger_helpers::createTriggers({}).size(), 0);
  BOOST_CHECK_EQUAL(trigger_helpers::createTriggers({ "once", "always" }).size(), 2);
}

BOOST_AUTO_TEST_CASE(test_try_triggers)
{
  {
    auto triggers = trigger_helpers::createTriggers({});
    BOOST_CHECK(!trigger_helpers::tryTrigger(triggers));
    BOOST_CHECK(!trigger_helpers::tryTrigger(triggers));
    BOOST_CHECK(!trigger_helpers::tryTrigger(triggers));
    BOOST_CHECK(!trigger_helpers::tryTrigger(triggers));
    BOOST_CHECK(!trigger_helpers::tryTrigger(triggers));
  }

  {
    auto triggers = trigger_helpers::createTriggers({ "once" });
    BOOST_CHECK(trigger_helpers::tryTrigger(triggers));
    BOOST_CHECK(!trigger_helpers::tryTrigger(triggers));
    BOOST_CHECK(!trigger_helpers::tryTrigger(triggers));
    BOOST_CHECK(!trigger_helpers::tryTrigger(triggers));
    BOOST_CHECK(!trigger_helpers::tryTrigger(triggers));
  }

  {
    auto triggers = trigger_helpers::createTriggers({ "once", "once", "once" });
    BOOST_CHECK(trigger_helpers::tryTrigger(triggers));
    BOOST_CHECK(trigger_helpers::tryTrigger(triggers));
    BOOST_CHECK(trigger_helpers::tryTrigger(triggers));
    BOOST_CHECK(!trigger_helpers::tryTrigger(triggers));
    BOOST_CHECK(!trigger_helpers::tryTrigger(triggers));
    BOOST_CHECK(!trigger_helpers::tryTrigger(triggers));
  }
}