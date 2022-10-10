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
/// \file    testCheckRunner.cxx
/// \author  Piotr Konopka
///

#include "QualityControl/CheckRunnerFactory.h"
#include "QualityControl/CheckRunner.h"
#include "QualityControl/CommonSpec.h"

#define BOOST_TEST_MODULE CheckRunner test
#define BOOST_TEST_MAIN
#define BOOST_TEST_DYN_LINK

#include <boost/test/unit_test.hpp>

using namespace o2::quality_control::checker;
using namespace std;
using namespace o2::framework;
using namespace o2::header;

BOOST_AUTO_TEST_CASE(test_check_runner_static)
{
  // facility name
  BOOST_CHECK(CheckRunner::createCheckRunnerFacility(CheckRunner::createCheckRunnerIdString() + "-test") == "check/test");
  BOOST_CHECK(CheckRunner::createCheckRunnerFacility(CheckRunner::createCheckRunnerIdString() + "-abcdefghijklmnopqrstuvwxyz") == "check/abcdefghijklmnopqrstuvwxyz");
  BOOST_CHECK(CheckRunner::createCheckRunnerFacility(CheckRunner::createCheckRunnerIdString() + "-abcdefghijklmnopqrstuvwxyz123456789") == "check/abcdefghijklmnopqrstuvwxyz");
}

BOOST_AUTO_TEST_CASE(test_getDetector)
{
  CheckConfig config;
  config.detectorName = "TST";

  vector<CheckConfig> checks;
  BOOST_CHECK_EQUAL(CheckRunner::getDetectorName(checks), "");
  checks.push_back(config);
  BOOST_CHECK_EQUAL(CheckRunner::getDetectorName(checks), "TST");
  checks.push_back(config);
  BOOST_CHECK_EQUAL(CheckRunner::getDetectorName(checks), "TST");
  config.detectorName = "EMC";
  checks.push_back(config);
  BOOST_CHECK_EQUAL(CheckRunner::getDetectorName(checks), "MANY");
}