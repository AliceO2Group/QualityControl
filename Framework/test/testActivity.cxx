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
/// \file    testActivity.cxx
/// \author  Piotr Konopka
///

#include "QualityControl/Activity.h"
#include <catch_amalgamated.hpp>

using namespace o2::quality_control::core;

TEST_CASE("test_matching")
{
  {
    // the default Activity has the widest match (provenance always has to match)
    Activity matcher{};

    CHECK(matcher.matches({ 300000, 1, "LHC22a", "spass", "qc", { 1, 10 }, "pp" }));
    CHECK(matcher.matches({ 0, 0, "", "", "qc", { 1, 10 }, "" }));
    CHECK(!matcher.matches({ 0, 0, "", "", "qc_mc", { 1, 10 }, "" }));
    CHECK(matcher.matches({}));
    CHECK(Activity().matches(matcher));
  }
  {
    // the most concrete matcher
    // it also should not match any less concrete Activity
    Activity matcher{ 300000, 1, "LHC22a", "spass", "qc", { 1, 10 }, "pp" };

    // should match only the same but with equal or contained validity
    CHECK(matcher.matches({ 300000, 1, "LHC22a", "spass", "qc", { 1, 10 }, "pp" }));
    CHECK(matcher.matches({ 300000, 1, "LHC22a", "spass", "qc", { 5, 7 }, "pp" }));
    CHECK(matcher.matches({ 300000, 1, "LHC22a", "spass", "qc", { 5, 15 }, "pp" })); // we support this until we indicate correct validity of our objects
    CHECK(!matcher.matches({ 300000, 1, "LHC22a", "spass", "qc", { 15, 25 }, "pp" }));

    // should not match if any other parameter is different
    CHECK(!matcher.matches({ 300001, 1, "LHC22a", "spass", "qc", { 1, 10 }, "pp" }));
    CHECK(!matcher.matches({ 300000, 2, "LHC22a", "spass", "qc", { 1, 10 }, "pp" }));
    CHECK(!matcher.matches({ 300000, 1, "LHC22b", "apass", "qc", { 1, 10 }, "pp" }));
    CHECK(!matcher.matches({ 300000, 1, "LHC22a", "spass", "qc_mc", { 1, 10 }, "pp" }));
    CHECK(!matcher.matches({ 300000, 1, "LHC22a", "spass", "qc", { 1, 10 }, "PbPb" }));

    // should not match any less concrete field
    CHECK(!matcher.matches({ 0, 1, "LHC22a", "spass", "qc", { 1, 10 } }));
    CHECK(!matcher.matches({ 300000, 0, "LHC22a", "spass", "qc", { 1, 10 } }));
    CHECK(!matcher.matches({ 300000, 1, "", "spass", "qc", { 1, 10 } }));
    CHECK(!matcher.matches({ 300000, 1, "LHC22a", "", "qc", { 1, 10 } }));
    CHECK(!matcher.matches({ 300000, 1, "LHC22a", "spass", "qc", { 0, 1000000 } }));
    CHECK(!matcher.matches({ 300000, 1, "LHC22a", "spass", "qc", { 1, 10 }, "" }));
  }
}

TEST_CASE("test_same")
{
  {
    // Activity::same should return true if the other one is has the same field, but the validity can be different
    Activity activity{ 300000, 1, "LHC22a", "spass", "qc", { 1, 10 } };

    CHECK(activity.same({ 300000, 1, "LHC22a", "spass", "qc", { 1, 10 } }));
    CHECK(activity.same({ 300000, 1, "LHC22a", "spass", "qc", { 2, 5 } }));
    CHECK(activity.same({ 300000, 1, "LHC22a", "spass", "qc", { 432, 54334 } }));

    CHECK(!activity.same({ 300001, 1, "LHC22a", "spass", "qc", { 1, 10 } }));
    CHECK(!activity.same({ 300000, 2, "LHC22a", "spass", "qc", { 1, 10 } }));
    CHECK(!activity.same({ 300000, 1, "LHC22b", "spass", "qc", { 1, 10 } }));
    CHECK(!activity.same({ 300000, 1, "LHC22a", "apass", "qc", { 1, 10 } }));
    CHECK(!activity.same({ 300000, 1, "LHC22a", "spass", "qc_mc", { 1, 10 } }));
  }
}