// Copyright CERN and copyright holders of ALICE O2. This software is
// distributed under the terms of the GNU General Public License v3 (GPL
// Version 3), copied verbatim in the file "COPYING".
//
// See http://alice-o2.web.cern.ch/license for full licensing information.
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.

///
/// \file    testChecker.cxx
/// \author  Piotr Konopka
///

#include "QualityControl/CheckerFactory.h"
#include "QualityControl/Checker.h"
#include <Framework/DataSampling.h>
#include <Framework/DataSpecUtils.h>

#define BOOST_TEST_MODULE Checker test
#define BOOST_TEST_MAIN
#define BOOST_TEST_DYN_LINK

#include <boost/test/unit_test.hpp>
#include <iostream>

using namespace o2::quality_control::checker;
using namespace std;
using namespace o2::framework;
using namespace o2::header;

BOOST_AUTO_TEST_CASE(test_checker_factory)
{
  std::string configFilePath =
    std::string("json:/") + getenv("QUALITYCONTROL_ROOT") + "/tests/testSharedConfig.json";

  CheckerFactory checkerFactory;
  DataProcessorSpec checker = checkerFactory.create("abcChecker", "abcTask", configFilePath);

  BOOST_CHECK_EQUAL(checker.name, "abcChecker");

  BOOST_REQUIRE_EQUAL(checker.inputs.size(), 1);
  BOOST_CHECK_EQUAL(checker.inputs[0], (InputSpec{ { "mo" }, "QC", "abcTask-mo", 0 }));

  BOOST_REQUIRE_EQUAL(checker.outputs.size(), 1);
  BOOST_CHECK_EQUAL(checker.outputs[0], (OutputSpec{ "QC", "abcTask-chk", 0 }));

  BOOST_CHECK(checker.algorithm.onInit != nullptr);
}

BOOST_AUTO_TEST_CASE(test_checker_static)
{
  BOOST_CHECK(Checker::createCheckerDataDescription("qwertyuiop") == DataDescription("qwertyuiop-chk"));
  BOOST_CHECK(Checker::createCheckerDataDescription("012345678901234567890") == DataDescription("012345678901-chk"));
  BOOST_CHECK_THROW(Checker::createCheckerDataDescription(""), AliceO2::Common::FatalException);
}

BOOST_AUTO_TEST_CASE(test_checker)
{
  std::string configFilePath =
    std::string("json:/") + getenv("QUALITYCONTROL_ROOT") + "/tests/testSharedConfig.json";

  Checker checker{ "abcChecker", "abcTask", configFilePath };

  BOOST_CHECK_EQUAL(checker.getInputSpec(), (InputSpec{ { "mo" }, "QC", "abcTask-mo", 0 }));
  BOOST_CHECK_EQUAL(checker.getOutputSpec(), (OutputSpec{ "QC", "abcTask-chk", 0 }));

  // This is maximum that we can do until we are able to test the DPL algorithms in isolation.
  // TODO: When it is possible, we should try calling run() and init()
}