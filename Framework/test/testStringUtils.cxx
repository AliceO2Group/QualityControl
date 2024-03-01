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

#include "QualityControl/stringUtils.h"

#define BOOST_TEST_MODULE Triggers test
#define BOOST_TEST_MAIN
#define BOOST_TEST_DYN_LINK

#include <boost/test/unit_test.hpp>

using namespace o2::quality_control::core;

BOOST_AUTO_TEST_CASE(test_is_number)
{
  BOOST_CHECK_EQUAL(isOnlyDigits("1"), true);
  BOOST_CHECK_EQUAL(isOnlyDigits("-1"), false);
  BOOST_CHECK_EQUAL(isOnlyDigits("1000000"), true);
  BOOST_CHECK_EQUAL(isOnlyDigits("0.1"), false);
  BOOST_CHECK_EQUAL(isOnlyDigits(".2"), false);
  BOOST_CHECK_EQUAL(isOnlyDigits("x"), false);
  BOOST_CHECK_EQUAL(isOnlyDigits("1x"), false);
  BOOST_CHECK_EQUAL(isOnlyDigits("......"), false);
}