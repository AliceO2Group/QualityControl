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
#include <catch_amalgamated.hpp>

using namespace o2::quality_control::checker;
using namespace std;
using namespace o2::framework;
using namespace o2::header;

TEST_CASE("test_check_runner_static")
{
  // facility name
  CHECK(CheckRunner::createCheckRunnerFacility(CheckRunner::createCheckRunnerIdString() + "-test") == "check/test");
  CHECK(CheckRunner::createCheckRunnerFacility(CheckRunner::createCheckRunnerIdString() + "-abcdefghijklmnopqrstuvwxyz") == "check/abcdefghijklmnopqrstuvwxyz");
  CHECK(CheckRunner::createCheckRunnerFacility(CheckRunner::createCheckRunnerIdString() + "-abcdefghijklmnopqrstuvwxyz123456789") == "check/abcdefghijklmnopqrstuvwxyz");
}

TEST_CASE("test_checkRunner_getDetector")
{
  CheckConfig config;
  config.detectorName = "TST";

  vector<CheckConfig> checks;
  CHECK(CheckRunner::getDetectorName(checks) == "");
  checks.push_back(config);
  CHECK(CheckRunner::getDetectorName(checks) == "TST");
  checks.push_back(config);
  CHECK(CheckRunner::getDetectorName(checks) == "TST");
  config.detectorName = "EMC";
  checks.push_back(config);
  CHECK(CheckRunner::getDetectorName(checks) == "MANY");
}
