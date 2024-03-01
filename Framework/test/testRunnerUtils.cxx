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

#define BOOST_TEST_MODULE RunnerUtils test
#define BOOST_TEST_MAIN
#define BOOST_TEST_DYN_LINK

#include <Framework/ConfigParamRegistry.h>
#include <Framework/DataProcessorSpec.h>
#include <boost/test/unit_test.hpp>

using namespace o2::quality_control::core;
using namespace std;

BOOST_AUTO_TEST_CASE(test_computeActivity)
{
  string runType;
  BOOST_CHECK_EQUAL(translateIntegerRunType("0"), "NONE");
  BOOST_CHECK_EQUAL(translateIntegerRunType("1"), "PHYSICS");
  BOOST_CHECK_EQUAL(translateIntegerRunType("18"), "CALIBRATION_VRESETD");
  BOOST_CHECK_EQUAL(translateIntegerRunType("34"), "NONE");
  BOOST_CHECK_EQUAL(translateIntegerRunType("-134"), "NONE");
}