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
/// \file    testFlagHelpers.cxx
/// \author  Piotr Konopka
///

#include "QualityControl/FlagHelpers.h"
#include "QualityControl/ValidityInterval.h"

#include <catch_amalgamated.hpp>

using namespace o2::quality_control;
using namespace o2::quality_control::core;
using namespace o2::quality_control::core::flag_helpers;

TEST_CASE("intervalsConnect")
{
  SECTION("Valid intervals that are adjacent")
  {
    ValidityInterval interval1{ 1, 10 };
    ValidityInterval interval2{ 10, 20 };
    REQUIRE(intervalsConnect(interval1, interval2));
    REQUIRE(intervalsConnect(interval2, interval1));
  }

  SECTION("Valid intervals that are not adjacent")
  {
    ValidityInterval interval1{ 1, 10 };
    ValidityInterval interval2{ 11, 20 };
    REQUIRE_FALSE(intervalsConnect(interval1, interval2));
    REQUIRE_FALSE(intervalsConnect(interval2, interval1));
  }

  SECTION("Overlapping intervals")
  {
    ValidityInterval interval1{ 1, 15 };
    ValidityInterval interval2{ 10, 20 };
    REQUIRE(intervalsConnect(interval1, interval2));
    REQUIRE(intervalsConnect(interval2, interval1));
  }

  SECTION("Invalid intervals")
  {
    ValidityInterval invalidInterval{ 10, 5 }; // max < min
    ValidityInterval validInterval{ 1, 10 };
    REQUIRE_FALSE(intervalsConnect(invalidInterval, validInterval));
    REQUIRE_FALSE(intervalsConnect(validInterval, invalidInterval));
  }

  SECTION("Intervals with same start and end")
  {
    ValidityInterval interval{ 10, 10 };
    REQUIRE(intervalsConnect(interval, interval));
  }
}

TEST_CASE("intervalsOverlap")
{
  SECTION("Valid intervals that are adjacent")
  {
    ValidityInterval interval1{ 1, 10 };
    ValidityInterval interval2{ 10, 20 };
    REQUIRE_FALSE(intervalsOverlap(interval1, interval2));
    REQUIRE_FALSE(intervalsOverlap(interval2, interval1));
  }

  SECTION("Overlapping intervals")
  {
    ValidityInterval interval1{ 1, 15 };
    ValidityInterval interval2{ 10, 20 };
    REQUIRE(intervalsOverlap(interval1, interval2));
    REQUIRE(intervalsOverlap(interval2, interval1));
  }

  SECTION("Invalid intervals")
  {
    ValidityInterval invalidInterval{ 10, 5 }; // max < min
    ValidityInterval validInterval{ 1, 10 };
    REQUIRE_FALSE(intervalsOverlap(invalidInterval, validInterval));
    REQUIRE_FALSE(intervalsOverlap(validInterval, invalidInterval));
  }

  // fixme: This now returns false due to < and > comparators being used.
  //  However, the true reason for returning false should be the invalidity of the two intervals,
  //  which are now considered valid, since we use o2::math_utils::detail::Bracket::isValid().
  //  This could be refactored, so an interval [x, x) is consistently treated as invalid.
  SECTION("Intervals with same start and end")
  {
    ValidityInterval interval{ 10, 10 };
    REQUIRE_FALSE(intervalsOverlap(interval, interval));
  }
}

TEST_CASE("excludeInterval")
{
  SECTION("Empty vector when interval fully covers the flag's interval")
  {
    QualityControlFlag qcFlag{ 10, 20, FlagTypeFactory::BadTracking() };
    ValidityInterval interval{ 5, 25 };

    auto result = excludeInterval(qcFlag, interval);

    REQUIRE(result.empty());
  }

  SECTION("One flag when interval covers the start of the flag's interval")
  {
    QualityControlFlag qcFlag{ 10, 20, FlagTypeFactory::BadTracking() };
    ValidityInterval interval{ 5, 15 };

    auto result = excludeInterval(qcFlag, interval);

    REQUIRE(result.size() == 1);
    REQUIRE(result[0].getStart() == 15);
    REQUIRE(result[0].getEnd() == 20);
    REQUIRE(result[0].getFlag() == qcFlag.getFlag());
    REQUIRE(result[0].getComment() == qcFlag.getComment());
    REQUIRE(result[0].getSource() == qcFlag.getSource());
  }

  SECTION("One flag when interval covers the end of the flag's interval")
  {
    QualityControlFlag qcFlag{ 10, 20, FlagTypeFactory::BadTracking() };
    ValidityInterval interval{ 15, 25 };

    auto result = excludeInterval(qcFlag, interval);

    REQUIRE(result.size() == 1);
    REQUIRE(result[0].getStart() == 10);
    REQUIRE(result[0].getEnd() == 15);
    REQUIRE(result[0].getFlag() == qcFlag.getFlag());
    REQUIRE(result[0].getComment() == qcFlag.getComment());
    REQUIRE(result[0].getSource() == qcFlag.getSource());
  }

  SECTION("Two flags when interval is fully contained within the flag's interval")
  {
    QualityControlFlag qcFlag{ 10, 30, FlagTypeFactory::BadTracking() };
    ValidityInterval interval{ 15, 25 };

    auto result = excludeInterval(qcFlag, interval);

    REQUIRE(result.size() == 2);
    REQUIRE(result[0].getStart() == 10);
    REQUIRE(result[0].getEnd() == 15);
    REQUIRE(result[0].getFlag() == qcFlag.getFlag());
    REQUIRE(result[0].getComment() == qcFlag.getComment());
    REQUIRE(result[0].getSource() == qcFlag.getSource());
    REQUIRE(result[1].getStart() == 25);
    REQUIRE(result[1].getEnd() == 30);
    REQUIRE(result[1].getFlag() == qcFlag.getFlag());
    REQUIRE(result[1].getComment() == qcFlag.getComment());
    REQUIRE(result[1].getSource() == qcFlag.getSource());
  }

  SECTION("No exclusion when interval is before the flag's interval")
  {
    QualityControlFlag qcFlag{ 10, 20, FlagTypeFactory::BadTracking() };
    ValidityInterval interval{ 0, 5 };

    auto result = excludeInterval(qcFlag, interval);

    REQUIRE(result.size() == 1);
    REQUIRE(result[0].getStart() == 10);
    REQUIRE(result[0].getEnd() == 20);
    REQUIRE(result[0].getFlag() == qcFlag.getFlag());
    REQUIRE(result[0].getComment() == qcFlag.getComment());
    REQUIRE(result[0].getSource() == qcFlag.getSource());
  }

  SECTION("No exclusion when interval is after the flag's interval")
  {
    QualityControlFlag qcFlag{ 10, 20, FlagTypeFactory::BadTracking() };
    ValidityInterval interval{ 25, 30 };

    auto result = excludeInterval(qcFlag, interval);

    REQUIRE(result.size() == 1);
    REQUIRE(result[0].getStart() == 10);
    REQUIRE(result[0].getEnd() == 20);
    REQUIRE(result[0].getFlag() == qcFlag.getFlag());
    REQUIRE(result[0].getComment() == qcFlag.getComment());
    REQUIRE(result[0].getSource() == qcFlag.getSource());
  }

  SECTION("Invalid flag interval returns the same flag")
  {
    QualityControlFlag qcFlag{ 10, 10, FlagTypeFactory::BadTracking() }; // Zero-length interval
    ValidityInterval interval{ 5, 15 };

    auto result = excludeInterval(qcFlag, interval);

    REQUIRE(result.size() == 1);
    REQUIRE(result[0].getStart() == 10);
    REQUIRE(result[0].getEnd() == 10);
    REQUIRE(result[0].getFlag() == qcFlag.getFlag());
    REQUIRE(result[0].getComment() == qcFlag.getComment());
    REQUIRE(result[0].getSource() == qcFlag.getSource());
  }

  SECTION("Zero-length overlap returns original flag")
  {
    QualityControlFlag qcFlag{ 10, 20, FlagTypeFactory::BadTracking() };
    ValidityInterval interval{ 15, 15 }; // Zero-length interval

    auto result = excludeInterval(qcFlag, interval);

    REQUIRE(result.size() == 1);
    REQUIRE(result[0].getStart() == 10);
    REQUIRE(result[0].getEnd() == 20);
    REQUIRE(result[0].getFlag() == qcFlag.getFlag());
    REQUIRE(result[0].getComment() == qcFlag.getComment());
    REQUIRE(result[0].getSource() == qcFlag.getSource());
  }
}

TEST_CASE("intersection")
{
  SECTION("Returns original flag when interval is invalid")
  {
    QualityControlFlag qcFlag{ 10, 20, FlagTypeFactory::BadTracking() };
    ValidityInterval interval{ 10, 5 };

    auto result = intersection(qcFlag, interval);

    REQUIRE(result.has_value());
    REQUIRE(result->getStart() == 10);
    REQUIRE(result->getEnd() == 20);
    REQUIRE(result->getFlag() == qcFlag.getFlag());
    REQUIRE(result->getComment() == qcFlag.getComment());
    REQUIRE(result->getSource() == qcFlag.getSource());
  }

  SECTION("Returns nullopt when there is no overlap")
  {
    QualityControlFlag qcFlag{ 10, 20, FlagTypeFactory::BadTracking() };
    ValidityInterval interval{ 25, 30 };

    auto result = intersection(qcFlag, interval);

    REQUIRE(!result.has_value());
  }

  SECTION("Returns intersected flag when intervals partially overlap")
  {
    QualityControlFlag qcFlag{ 10, 20, FlagTypeFactory::BadTracking(), "comment", "source" };
    ValidityInterval interval{ 15, 25 };

    auto result = intersection(qcFlag, interval);

    REQUIRE(result.has_value());
    REQUIRE(result->getStart() == 15);
    REQUIRE(result->getEnd() == 20);
    REQUIRE(result->getFlag() == qcFlag.getFlag());
    REQUIRE(result->getComment() == qcFlag.getComment());
    REQUIRE(result->getSource() == qcFlag.getSource());
  }

  SECTION("Returns intersected flag when intervals fully overlap")
  {
    QualityControlFlag qcFlag{ 10, 30, FlagTypeFactory::BadTracking(), "comment", "source" };
    ValidityInterval interval{ 15, 25 };

    auto result = intersection(qcFlag, interval);

    REQUIRE(result.has_value());
    REQUIRE(result->getStart() == 15);
    REQUIRE(result->getEnd() == 25);
    REQUIRE(result->getFlag() == qcFlag.getFlag());
    REQUIRE(result->getComment() == qcFlag.getComment());
    REQUIRE(result->getSource() == qcFlag.getSource());
  }

  SECTION("Returns intersected flag when flag's interval is fully within the given interval")
  {
    QualityControlFlag qcFlag{ 15, 25, FlagTypeFactory::BadTracking(), "comment", "source" };
    ValidityInterval interval{ 10, 30 };

    auto result = intersection(qcFlag, interval);

    REQUIRE(result.has_value());
    REQUIRE(result->getStart() == 15);
    REQUIRE(result->getEnd() == 25);
    REQUIRE(result->getFlag() == qcFlag.getFlag());
    REQUIRE(result->getComment() == qcFlag.getComment());
    REQUIRE(result->getSource() == qcFlag.getSource());
  }
}
