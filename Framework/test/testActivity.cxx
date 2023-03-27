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
#include "QualityControl/ActivityHelpers.h"

#define BOOST_TEST_MODULE Activity test
#define BOOST_TEST_MAIN
#define BOOST_TEST_DYN_LINK

#include <boost/test/unit_test.hpp>

using namespace o2::quality_control::core;

BOOST_AUTO_TEST_CASE(test_matching)
{
  {
    // the default Activity has the widest match (provenance always has to match)
    Activity matcher{};

    BOOST_CHECK(matcher.matches({ 300000, 1, "LHC22a", "spass", "qc", { 1, 10 }, "pp" }));
    BOOST_CHECK(matcher.matches({ 0, 0, "", "", "qc", { 1, 10 }, "" }));
    BOOST_CHECK(!matcher.matches({ 0, 0, "", "", "qc_mc", { 1, 10 }, "" }));
    BOOST_CHECK(matcher.matches({}));
    BOOST_CHECK(Activity().matches(matcher));
  }
  {
    // the most concrete matcher
    // it also should not match any less concrete Activity
    Activity matcher{ 300000, 1, "LHC22a", "spass", "qc", { 1, 10 }, "pp" };

    // should match only the same but with equal or contained validity
    BOOST_CHECK(matcher.matches({ 300000, 1, "LHC22a", "spass", "qc", { 1, 10 }, "pp" }));
    BOOST_CHECK(matcher.matches({ 300000, 1, "LHC22a", "spass", "qc", { 5, 7 }, "pp" }));
    BOOST_CHECK(matcher.matches({ 300000, 1, "LHC22a", "spass", "qc", { 5, 15 }, "pp" })); // we support this until we indicate correct validity of our objects
    BOOST_CHECK(!matcher.matches({ 300000, 1, "LHC22a", "spass", "qc", { 15, 25 }, "pp" }));

    // should not match if any other parameter is different
    BOOST_CHECK(!matcher.matches({ 300001, 1, "LHC22a", "spass", "qc", { 1, 10 }, "pp" }));
    BOOST_CHECK(!matcher.matches({ 300000, 2, "LHC22a", "spass", "qc", { 1, 10 }, "pp" }));
    BOOST_CHECK(!matcher.matches({ 300000, 1, "LHC22b", "apass", "qc", { 1, 10 }, "pp" }));
    BOOST_CHECK(!matcher.matches({ 300000, 1, "LHC22a", "spass", "qc_mc", { 1, 10 }, "pp" }));
    BOOST_CHECK(!matcher.matches({ 300000, 1, "LHC22a", "spass", "qc", { 1, 10 }, "PbPb" }));

    // should not match any less concrete field
    BOOST_CHECK(!matcher.matches({ 0, 1, "LHC22a", "spass", "qc", { 1, 10 } }));
    BOOST_CHECK(!matcher.matches({ 300000, 0, "LHC22a", "spass", "qc", { 1, 10 } }));
    BOOST_CHECK(!matcher.matches({ 300000, 1, "", "spass", "qc", { 1, 10 } }));
    BOOST_CHECK(!matcher.matches({ 300000, 1, "LHC22a", "", "qc", { 1, 10 } }));
    BOOST_CHECK(!matcher.matches({ 300000, 1, "LHC22a", "spass", "qc", { 0, 1000000 } }));
    BOOST_CHECK(!matcher.matches({ 300000, 1, "LHC22a", "spass", "qc", { 1, 10 }, "" }));
  }
}

BOOST_AUTO_TEST_CASE(test_same)
{
  {
    // Activity::same should return true if the other one is has the same field, but the validity can be different
    Activity activity{ 300000, 1, "LHC22a", "spass", "qc", { 1, 10 } };

    BOOST_CHECK(activity.same({ 300000, 1, "LHC22a", "spass", "qc", { 1, 10 } }));
    BOOST_CHECK(activity.same({ 300000, 1, "LHC22a", "spass", "qc", { 2, 5 } }));
    BOOST_CHECK(activity.same({ 300000, 1, "LHC22a", "spass", "qc", { 432, 54334 } }));

    BOOST_CHECK(!activity.same({ 300001, 1, "LHC22a", "spass", "qc", { 1, 10 } }));
    BOOST_CHECK(!activity.same({ 300000, 2, "LHC22a", "spass", "qc", { 1, 10 } }));
    BOOST_CHECK(!activity.same({ 300000, 1, "LHC22b", "spass", "qc", { 1, 10 } }));
    BOOST_CHECK(!activity.same({ 300000, 1, "LHC22a", "apass", "qc", { 1, 10 } }));
    BOOST_CHECK(!activity.same({ 300000, 1, "LHC22a", "spass", "qc_mc", { 1, 10 } }));
  }
}

BOOST_AUTO_TEST_CASE(test_minimalMatchingActivity)
{
  {
    // providing a map accessor + everything being the same except the validity
    std::map<int, Activity> m{
      { 1, { 300000, 1, "LHC22a", "spass", "qc", { 1, 10 }, "pp" } },
      { 2, { 300000, 1, "LHC22a", "spass", "qc", { 10, 20 }, "pp" } },
      { 4, { 300000, 1, "LHC22a", "spass", "qc", { 20, 30 }, "pp" } },
      { 3, { 300000, 1, "LHC22a", "spass", "qc", { 30, 40 }, "pp" } }
    };
    auto result = ActivityHelpers::strictestMatchingActivity(m.begin(), m.end(), [](const auto& item) { return item.second; });
    Activity expectation{ 300000, 1, "LHC22a", "spass", "qc", { 1, 40 }, "pp" };
    BOOST_CHECK_EQUAL(result, expectation);
  }
  {
    // providing a vector (default accessor) + different run numbers and validities
    std::vector<Activity> m{
      { 300000, 1, "LHC22a", "spass", "qc", { 1, 10 }, "pp" },
      { 300001, 1, "LHC22a", "spass", "qc", { 20, 30 }, "pp" }
    };
    auto result = ActivityHelpers::strictestMatchingActivity(m.begin(), m.end());
    Activity expectation{ 0, 1, "LHC22a", "spass", "qc", { 1, 30 }, "pp" };
    BOOST_CHECK_EQUAL(result, expectation);
  }
  {
    // providing a vector (custom accessor) + different everything
    std::vector<Activity> m{
      { 300000, 1, "LHC22a", "spass", "qc", { 1, 10 }, "pp" },
      { 300001, 2, "LHC22b", "apass2", "qc_mc", { 20, 30 }, "PbPb" }
    };
    auto result = ActivityHelpers::strictestMatchingActivity(m.begin(), m.end(), [](const auto& a) { return a; });
    Activity expectation{ 0, 0, "", "", "qc", { 1, 30 }, "" };
    BOOST_CHECK_EQUAL(result, expectation);
  }
}
