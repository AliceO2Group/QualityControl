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
/// \file   testQuality.cxx
/// \author Barthelemy von Haller
///

#include "QualityControl/Quality.h"
#include "QualityControl/QcInfoLogger.h"

#define BOOST_TEST_MODULE Quality test
#define BOOST_TEST_MAIN
#define BOOST_TEST_DYN_LINK
#include <boost/test/unit_test.hpp>

using namespace std;

namespace o2::quality_control::core
{

BOOST_AUTO_TEST_CASE(quality_test)
{
  Quality asdf(123, "asdf");
  Quality myQuality = Quality::Bad;
  BOOST_CHECK_EQUAL(myQuality.getLevel(), 3);
  BOOST_CHECK_EQUAL(myQuality.getName(), "Bad");
  myQuality = Quality::Good;
  BOOST_CHECK_EQUAL(myQuality.getLevel(), 1);
  BOOST_CHECK_EQUAL(myQuality.getName(), "Good");
  myQuality = Quality::Medium;
  BOOST_CHECK_EQUAL(myQuality.getLevel(), 2);
  BOOST_CHECK_EQUAL(myQuality.getName(), "Medium");
  myQuality = Quality::Null;
  BOOST_CHECK_EQUAL(myQuality.getLevel(), Quality::NullLevel);
  BOOST_CHECK_EQUAL(myQuality.getName(), "Null");

  ILOG(Info, Support) << "test quality output : " << myQuality << ENDM;

  BOOST_CHECK(Quality::Bad.isWorseThan(Quality::Medium));
  BOOST_CHECK(Quality::Bad.isWorseThan(Quality::Good));
  BOOST_CHECK(!Quality::Bad.isWorseThan(Quality::Null));
  BOOST_CHECK(!Quality::Bad.isWorseThan(Quality::Bad));

  BOOST_CHECK(Quality::Good.isBetterThan(Quality::Medium));
  BOOST_CHECK(Quality::Good.isBetterThan(Quality::Bad));
  BOOST_CHECK(Quality::Good.isBetterThan(Quality::Null));
  BOOST_CHECK(!Quality::Good.isBetterThan(Quality::Good));
}

BOOST_AUTO_TEST_CASE(quality_reasons)
{
  Quality myQuality = Quality::Bad;
  myQuality.addReason(FlagReasonFactory::ProcessingError(), "exception in x");
  myQuality.addReason(FlagReasonFactory::ProcessingError(), "exception in y");
  myQuality.addReason(FlagReasonFactory::LimitedAcceptance(), "sector C off");

  auto myReasons = myQuality.getReasons();
  BOOST_CHECK_EQUAL(myReasons[0].first, FlagReasonFactory::ProcessingError());
  BOOST_CHECK_EQUAL(myReasons[0].second, "exception in x");
  BOOST_CHECK_EQUAL(myReasons[1].first, FlagReasonFactory::ProcessingError());
  BOOST_CHECK_EQUAL(myReasons[1].second, "exception in y");
  BOOST_CHECK_EQUAL(myReasons[2].first, FlagReasonFactory::LimitedAcceptance());
  BOOST_CHECK_EQUAL(myReasons[2].second, "sector C off");

  auto copyQuality = myQuality;
  auto copyReasons = copyQuality.getReasons();
  BOOST_CHECK_EQUAL(copyReasons[0].first, FlagReasonFactory::ProcessingError());
  BOOST_CHECK_EQUAL(copyReasons[0].second, "exception in x");
  BOOST_CHECK_EQUAL(copyReasons[1].first, FlagReasonFactory::ProcessingError());
  BOOST_CHECK_EQUAL(copyReasons[1].second, "exception in y");
  BOOST_CHECK_EQUAL(copyReasons[2].first, FlagReasonFactory::LimitedAcceptance());
  BOOST_CHECK_EQUAL(copyReasons[2].second, "sector C off");
}

} // namespace o2::quality_control::core
