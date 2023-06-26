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
/// \file    testTimekeeper.cxx
/// \author  Piotr Konopka
///

#include "QualityControl/Timekeeper.h"
#include "QualityControl/TimekeeperSynchronous.h"
#include "QualityControl/TimekeeperAsynchronous.h"
#include "QualityControl/ActivityHelpers.h"
#include <Framework/TimingInfo.h>
#include <CommonUtils/ConfigurableParam.h>

#include <catch_amalgamated.hpp>

using namespace o2::quality_control::core;
using namespace o2::framework;

TEST_CASE("timekeeper_synchronous")
{
  SECTION("defaults")
  {
    auto tk = std::make_shared<TimekeeperSynchronous>();
    CHECK(tk->getValidity() == gInvalidValidityInterval);
    CHECK(tk->getSampleTimespan() == gInvalidValidityInterval);
    CHECK(tk->getTimerangeIdRange() == gInvalidTimeframeIdRange);

    tk->reset();
    CHECK(tk->getValidity() == gInvalidValidityInterval);
    CHECK(tk->getSampleTimespan() == gInvalidValidityInterval);
    CHECK(tk->getTimerangeIdRange() == gInvalidTimeframeIdRange);
  }

  SECTION("one_data_point_no_timer")
  {
    auto tk = std::make_shared<TimekeeperSynchronous>();
    tk->updateByTimeFrameID(5, 32);

    CHECK(tk->getValidity() == gInvalidValidityInterval);
    CHECK(tk->getSampleTimespan() == gInvalidValidityInterval);
    CHECK(tk->getTimerangeIdRange() == TimeframeIdRange{ 5, 5 });

    tk->reset();
    CHECK(tk->getValidity() == gInvalidValidityInterval);
    CHECK(tk->getSampleTimespan() == gInvalidValidityInterval);
    CHECK(tk->getTimerangeIdRange() == gInvalidTimeframeIdRange);
  }

  SECTION("no_data_one_timer")
  {
    auto tk = std::make_shared<TimekeeperSynchronous>();
    tk->updateByCurrentTimestamp(1653000000000);

    CHECK(tk->getValidity() == ValidityInterval{ 1653000000000, 1653000000000 });
    CHECK(tk->getSampleTimespan() == gInvalidValidityInterval);
    CHECK(tk->getTimerangeIdRange() == gInvalidTimeframeIdRange);

    tk->reset();
    CHECK(tk->getValidity() == ValidityInterval{ 1653000000000, 1653000000000 });
    CHECK(tk->getSampleTimespan() == gInvalidValidityInterval);
    CHECK(tk->getTimerangeIdRange() == gInvalidTimeframeIdRange);
  }

  SECTION("one_data_point_sor_timer")
  {
    auto tk = std::make_shared<TimekeeperSynchronous>();
    tk->setActivityDuration(ValidityInterval{ 1653000000000, 1653000000000 });
    tk->updateByTimeFrameID(5, 32);

    CHECK(tk->getValidity() == gInvalidValidityInterval);
    // we need at least one update with timestamp for a valid validity
    tk->updateByCurrentTimestamp(1653000000000);
    CHECK(tk->getValidity() == ValidityInterval{ 1653000000000, 1653000000000 });
    CHECK(tk->getSampleTimespan() == ValidityInterval{ 1653000000011, 1653000000013 });
    CHECK(tk->getTimerangeIdRange() == TimeframeIdRange{ 5, 5 });
  }

  SECTION("one_data_point_one_timer")
  {
    auto tk = std::make_shared<TimekeeperSynchronous>();
    tk->updateByCurrentTimestamp(1653000000000);
    tk->updateByTimeFrameID(5, 32);

    CHECK(tk->getValidity() == ValidityInterval{ 1653000000000, 1653000000000 });
    CHECK(tk->getSampleTimespan() == ValidityInterval{ 1653000000011, 1653000000013 });
    CHECK(tk->getTimerangeIdRange() == TimeframeIdRange{ 5, 5 });

    tk->reset();
    CHECK(tk->getValidity() == ValidityInterval{ 1653000000000, 1653000000000 });
    CHECK(tk->getSampleTimespan() == gInvalidValidityInterval);
    CHECK(tk->getTimerangeIdRange() == gInvalidTimeframeIdRange);
  }

  SECTION("no_data_many_timers")
  {
    auto tk = std::make_shared<TimekeeperSynchronous>();
    tk->updateByCurrentTimestamp(1655000000000);
    tk->updateByCurrentTimestamp(1656000000000);
    tk->updateByCurrentTimestamp(1654000000000); // a timer from the past is rather unexpected, but it should not break anything

    CHECK(tk->getValidity() == ValidityInterval{ 1654000000000, 1656000000000 });
    CHECK(tk->getSampleTimespan() == gInvalidValidityInterval);
    CHECK(tk->getTimerangeIdRange() == gInvalidTimeframeIdRange);

    tk->reset();
    tk->updateByCurrentTimestamp(1655000000000); // again, we try with a timestamp which is before the beginning of this window
    CHECK(tk->getValidity() == ValidityInterval{ 1655000000000, 1656000000000 });
    CHECK(tk->getSampleTimespan() == gInvalidValidityInterval);
    CHECK(tk->getTimerangeIdRange() == gInvalidTimeframeIdRange);
  }

  SECTION("many_data_points_many_timers")
  {
    auto tk = std::make_shared<TimekeeperSynchronous>();
    tk->updateByCurrentTimestamp(1653000000000);
    tk->updateByTimeFrameID(5, 32);
    tk->updateByTimeFrameID(7, 32);
    tk->updateByTimeFrameID(3, 32);
    tk->updateByTimeFrameID(10, 32);
    tk->updateByCurrentTimestamp(1653500000000);

    CHECK(tk->getValidity() == ValidityInterval{ 1653000000000, 1653500000000 });
    CHECK(tk->getSampleTimespan() == ValidityInterval{ 1653000000005, 1653000000027 });
    CHECK(tk->getTimerangeIdRange() == TimeframeIdRange{ 3, 10 });

    tk->reset();
    CHECK(tk->getValidity() == ValidityInterval{ 1653500000000, 1653500000000 });
    CHECK(tk->getSampleTimespan() == gInvalidValidityInterval);
    CHECK(tk->getTimerangeIdRange() == gInvalidTimeframeIdRange);

    tk->updateByTimeFrameID(12, 32);
    tk->updateByTimeFrameID(54, 32);
    tk->updateByCurrentTimestamp(1653600000000);

    CHECK(tk->getValidity() == ValidityInterval{ 1653500000000, 1653600000000 });
    CHECK(tk->getSampleTimespan() == ValidityInterval{ 1653000000031, 1653000000152 });
    CHECK(tk->getTimerangeIdRange() == TimeframeIdRange{ 12, 54 });
  }

  SECTION("boundary_selection")
  {
    auto tk = std::make_shared<TimekeeperSynchronous>();

    // ECS first
    tk->setStartOfActivity(1, 2, 3);
    tk->setEndOfActivity(4, 5, 6);
    CHECK(tk->getActivityDuration().getMin() == 1);
    CHECK(tk->getActivityDuration().getMax() == 4);

    // current timestamp second
    tk->setStartOfActivity(0, 2, 3);
    tk->setEndOfActivity(0, 5, 6);
    CHECK(tk->getActivityDuration().getMin() == 3);
    CHECK(tk->getActivityDuration().getMax() == 6);
    tk->setStartOfActivity(-1, 2, 3);
    tk->setEndOfActivity(-1, 5, 6);
    CHECK(tk->getActivityDuration().getMin() == 3);
    CHECK(tk->getActivityDuration().getMax() == 6);

    // config as the last resort
    tk->setStartOfActivity(0, 2, 0);
    tk->setEndOfActivity(0, 5, 0);
    CHECK(tk->getActivityDuration().getMin() == 2);
    CHECK(tk->getActivityDuration().getMax() == 5);
    tk->setStartOfActivity(-1, 2, 0);
    tk->setEndOfActivity(-1, 5, 0);
    CHECK(tk->getActivityDuration().getMin() == 2);
    CHECK(tk->getActivityDuration().getMax() == 5);
  }
}

TEST_CASE("timekeeper_asynchronous")
{
  SECTION("defaults")
  {
    auto tk = std::make_shared<TimekeeperAsynchronous>();
    CHECK(tk->getValidity() == gInvalidValidityInterval);
    CHECK(tk->getSampleTimespan() == gInvalidValidityInterval);
    CHECK(tk->getTimerangeIdRange() == gInvalidTimeframeIdRange);

    tk->reset();
    CHECK(tk->getValidity() == gInvalidValidityInterval);
    CHECK(tk->getSampleTimespan() == gInvalidValidityInterval);
    CHECK(tk->getTimerangeIdRange() == gInvalidTimeframeIdRange);
  }

  SECTION("timers_have_no_effect")
  {
    auto tk = std::make_shared<TimekeeperAsynchronous>();
    tk->setActivityDuration(ValidityInterval{ 1653000000000, 1655000000000 });
    CHECK(tk->getValidity() == gInvalidValidityInterval);
    tk->updateByCurrentTimestamp(1654000000000);
    CHECK(tk->getValidity() == gInvalidValidityInterval);
  }

  SECTION("sor_eor_not_set")
  {
    auto tk = std::make_shared<TimekeeperAsynchronous>();
    // duration not set
    tk->updateByTimeFrameID(1234, 32);
    CHECK(tk->getValidity() == gInvalidValidityInterval);
    CHECK(tk->getSampleTimespan() == gInvalidValidityInterval);
    CHECK(tk->getTimerangeIdRange() == gInvalidTimeframeIdRange);

    // sor set, not eor - not enough
    tk->setActivityDuration(ValidityInterval{ 1653000000000, 0 });
    tk->updateByTimeFrameID(1234, 32);
    CHECK(tk->getValidity() == gInvalidValidityInterval);
    CHECK(tk->getSampleTimespan() == gInvalidValidityInterval);
    CHECK(tk->getTimerangeIdRange() == gInvalidTimeframeIdRange);

    // make sure nothing weird happens after reset
    tk->reset();
    CHECK(tk->getValidity() == gInvalidValidityInterval);
    CHECK(tk->getSampleTimespan() == gInvalidValidityInterval);
    CHECK(tk->getTimerangeIdRange() == gInvalidTimeframeIdRange);
  }

  SECTION("data_no_moving_window")
  {
    auto tk = std::make_shared<TimekeeperAsynchronous>();
    tk->setActivityDuration(ValidityInterval{ 1653000000000, 1655000000000 });

    tk->updateByTimeFrameID(3, 32);
    tk->updateByTimeFrameID(10, 32);
    CHECK(tk->getValidity() == ValidityInterval{ 1653000000000, 1655000000000 });
    CHECK(tk->getSampleTimespan() == ValidityInterval{ 1653000000005, 1653000000027 });
    CHECK(tk->getTimerangeIdRange() == TimeframeIdRange{ 3, 10 });

    tk->reset();
    CHECK(tk->getValidity() == gInvalidValidityInterval);
    CHECK(tk->getSampleTimespan() == gInvalidValidityInterval);
    CHECK(tk->getTimerangeIdRange() == gInvalidTimeframeIdRange);

    tk->updateByTimeFrameID(12, 32);
    tk->updateByTimeFrameID(54, 32);
    CHECK(tk->getValidity() == ValidityInterval{ 1653000000000, 1655000000000 });
    CHECK(tk->getSampleTimespan() == ValidityInterval{ 1653000000031, 1653000000152 });
    CHECK(tk->getTimerangeIdRange() == TimeframeIdRange{ 12, 54 });
  }

  SECTION("data_moving_window")
  {
    // for "simplicity" assuming TF length of 11246 orbits, which gives us 1.0005 second TF duration
    const auto nOrbitPerTF = 11246;
    auto tk = std::make_shared<TimekeeperAsynchronous>(30 * 1000);
    tk->setActivityDuration(ValidityInterval{ 1653000000000, 1653000095000 }); // 95 seconds: 0-30, 30-60, 60-95

    // hitting only the 1st window
    tk->updateByTimeFrameID(1, nOrbitPerTF);
    tk->updateByTimeFrameID(10, nOrbitPerTF);
    CHECK(tk->getValidity() == ValidityInterval{ 1653000000000, 1653000030000 });
    CHECK(tk->getSampleTimespan() == ValidityInterval{ 1653000000000, 1653000009999 });
    CHECK(tk->getTimerangeIdRange() == TimeframeIdRange{ 1, 10 });

    // hitting the 1st and 2nd window
    tk->reset();
    tk->updateByTimeFrameID(1, nOrbitPerTF);
    tk->updateByTimeFrameID(55, nOrbitPerTF);
    CHECK(tk->getValidity() == ValidityInterval{ 1653000000000, 1653000060000 });
    CHECK(tk->getSampleTimespan() == ValidityInterval{ 1653000000000, 1653000055001 });
    CHECK(tk->getTimerangeIdRange() == TimeframeIdRange{ 1, 55 });

    // hitting the 3rd, extended window in the main part.
    // there is no 4th window, since we merge the last two to avoid having the last one with too little statistics
    tk->reset();
    tk->updateByTimeFrameID(80, nOrbitPerTF);
    CHECK(tk->getValidity() == ValidityInterval{ 1653000060000, 1653000095000 });
    CHECK(tk->getSampleTimespan() == ValidityInterval{ 1653000079003, 1653000080002 });
    CHECK(tk->getTimerangeIdRange() == TimeframeIdRange{ 80, 80 });

    // hitting the 3rd window with a sample which is in the extended part.
    tk->reset();
    tk->updateByTimeFrameID(93, nOrbitPerTF);
    CHECK(tk->getValidity() == ValidityInterval{ 1653000060000, 1653000095000 });
    CHECK(tk->getSampleTimespan() == ValidityInterval{ 1653000092004, 1653000093003 });
    CHECK(tk->getTimerangeIdRange() == TimeframeIdRange{ 93, 93 });
  }

  SECTION("boundary_selection")
  {
    auto tk = std::make_shared<TimekeeperAsynchronous>();

    o2::conf::ConfigurableParam::updateFromString("NameConf.mCCDBServer=http://ccdb-test.cern.ch:8080");

    // CCDB RCT first
    tk->setStartOfActivity(1, 2, 3, activity_helpers::getCcdbSorTimeAccessor(300000));
    tk->setEndOfActivity(4, 5, 6, activity_helpers::getCcdbEorTimeAccessor(300000));
    CHECK(tk->getActivityDuration().getMin() > 100);
    CHECK(tk->getActivityDuration().getMax() > 100);

    // ECS second
    tk->setStartOfActivity(1, 2, 3);
    tk->setEndOfActivity(4, 5, 6);
    CHECK(tk->getActivityDuration().getMin() == 1);
    CHECK(tk->getActivityDuration().getMax() == 4);

    // config as the last resort
    tk->setStartOfActivity(0, 2, 0);
    tk->setEndOfActivity(0, 5, 0);
    CHECK(tk->getActivityDuration().getMin() == 2);
    CHECK(tk->getActivityDuration().getMax() == 5);
    tk->setStartOfActivity(-1, 2, 0);
    tk->setEndOfActivity(-1, 5, 0);
    CHECK(tk->getActivityDuration().getMin() == 2);
    CHECK(tk->getActivityDuration().getMax() == 5);
  }
}