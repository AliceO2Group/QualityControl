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
/// \file    testTriggers.cxx
/// \author  Piotr Konopka
///

#include "QualityControl/Triggers.h"

#define BOOST_TEST_MODULE Triggers test
#define BOOST_TEST_MAIN
#define BOOST_TEST_DYN_LINK

#include <boost/test/unit_test.hpp>

#include <chrono>
using namespace std::chrono;

using namespace o2::quality_control::postprocessing;

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
  Trigger t1(TriggerType::Once, 123);
  BOOST_CHECK_EQUAL(t1.triggerType, TriggerType::Once);
  BOOST_CHECK_EQUAL(t1.timestamp, 123);

  auto nowMs = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
  Trigger t2(TriggerType::Once);
  BOOST_CHECK_EQUAL(t2.triggerType, TriggerType::Once);
  BOOST_CHECK_CLOSE(t2.timestamp, nowMs, 100000); // 100 seconds of max. difference should be fine.
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