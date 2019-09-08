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

#define BOOST_TEST_MODULE CheckRunner test
#define BOOST_TEST_MAIN
#define BOOST_TEST_DYN_LINK

#include <boost/test/unit_test.hpp>

using namespace o2::quality_control::checker;
using namespace std;
using namespace o2::framework;
using namespace o2::header;

BOOST_AUTO_TEST_CASE(test_checker_factory)
{
  std::string configFilePath{ "json://tests/testSharedConfig.json" };

  CheckRunnerFactory checkerFactory;
  Check check("abcCheck", configFilePath);
  DataProcessorSpec checker = checkerFactory.create(check, configFilePath);

  BOOST_REQUIRE_EQUAL(checker.inputs.size(), 1);
  BOOST_CHECK_EQUAL(checker.inputs[0], (InputSpec{ { "mo" }, "QC", "abcTask-mo", 0 }));

  //BOOST_REQUIRE_EQUAL(checker.outputs.size(), 1);
  //BOOST_CHECK_EQUAL(checker.outputs[0], (OutputSpec{ "QC", "abcCheck-chk", 0 }));

  BOOST_CHECK(checker.algorithm.onInit != nullptr);
}

BOOST_AUTO_TEST_CASE(test_checker_static)
{
  BOOST_CHECK(CheckRunner::createCheckRunnerDataDescription("qwertyuiop") == DataDescription("qwertyuiop-chk"));
  BOOST_CHECK(CheckRunner::createCheckRunnerDataDescription("012345678901234567890") == DataDescription("012345678901-chk"));
  BOOST_CHECK_THROW(CheckRunner::createCheckRunnerDataDescription(""), AliceO2::Common::FatalException);
}

BOOST_AUTO_TEST_CASE(test_checker)
{
  std::string configFilePath = { "json://tests/testSharedConfig.json" };

  Check check("abcCheck", configFilePath);
  CheckRunner checker{ check, configFilePath };

  BOOST_CHECK_EQUAL(checker.getInputs()[0], (InputSpec{ { "mo" }, "QC", "abcTask-mo", 0 }));
  //BOOST_CHECK_EQUAL(checker.getOutputSpec(), (OutputSpec{ "QC", "abcTask-chk", 0 }));

  // This is maximum that we can do until we are able to test the DPL algorithms in isolation.
  // TODO: When it is possible, we should try calling run() and init()
}
