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
/// \file    testRunnerUtils.cxx
/// \author  Barthelemy von Haller
///

#include "QualityControl/runnerUtils.h"

#include <Framework/ConfigParamRegistry.h>
#include <Framework/DataProcessorSpec.h>
#include <catch_amalgamated.hpp>

using namespace o2::quality_control::core;
using namespace std;

TEST_CASE("test_computeActivity")
{
  string runType;
  CHECK(translateIntegerRunType("0") == "NONE");
  CHECK(translateIntegerRunType("1") == "PHYSICS");
  CHECK(translateIntegerRunType("18") == "CALIBRATION_VRESETD");
  CHECK(translateIntegerRunType("34") == "NONE");   // unknown
  CHECK(translateIntegerRunType("-134") == "-134"); // not a uint
  CHECK(translateIntegerRunType("NONE") == "NONE");
  CHECK(translateIntegerRunType("bla") == "bla");
}