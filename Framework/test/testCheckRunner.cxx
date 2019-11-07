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
/// \file    testCheckRunner.cxx
/// \author  Piotr Konopka
///

#include "QualityControl/CheckRunnerFactory.h"
#include "QualityControl/CheckRunner.h"
#include <Framework/DataSampling.h>
#include "getTestDataDirectory.h"

#define BOOST_TEST_MODULE CheckRunner test
#define BOOST_TEST_MAIN
#define BOOST_TEST_DYN_LINK

#include <boost/test/unit_test.hpp>

using namespace o2::quality_control::checker;
using namespace std;
using namespace o2::framework;
using namespace o2::header;

BOOST_AUTO_TEST_CASE(test_check_runner_factory)
{
  std::string configFilePath = std::string("json://") + getTestDataDirectory() + "testSharedConfig.json";

  CheckRunnerFactory checkerFactory;
  Check check("singleCheck", configFilePath);
  DataProcessorSpec checker = checkerFactory.create(check, configFilePath);

  BOOST_REQUIRE_EQUAL(checker.inputs.size(), 1);
  BOOST_CHECK_EQUAL(checker.inputs[0], (InputSpec{ { "mo" }, "QC", "abcTask-mo", 0 }));

  BOOST_CHECK(checker.algorithm.onInit != nullptr);
}

BOOST_AUTO_TEST_CASE(test_check_runner_static)
{
  BOOST_CHECK(CheckRunner::createCheckRunnerDataDescription("qwertyuiop") == DataDescription("qwertyuiop-chk"));
  BOOST_CHECK(CheckRunner::createCheckRunnerDataDescription("012345678901234567890") == DataDescription("012345678901-chk"));
  BOOST_CHECK_THROW(CheckRunner::createCheckRunnerDataDescription(""), AliceO2::Common::FatalException);
}

BOOST_AUTO_TEST_CASE(test_check_runner)
{
  std::string configFilePath = std::string("json://") + getTestDataDirectory() + "testSharedConfig.json";

  Check check("singleCheck", configFilePath);
  CheckRunner checker{ check, configFilePath };

  BOOST_CHECK_EQUAL(checker.getInputs()[0], (InputSpec{ { "mo" }, "QC", "abcTask-mo", 0 }));
  BOOST_CHECK_EQUAL(checker.getOutputs()[0], (OutputSpec{ "QC", "singleCheck-chk", 0 }));

  // This is maximum that we can do until we are able to test the DPL algorithms in isolation.
  // TODO: When it is possible, we should try calling run() and init()
}
