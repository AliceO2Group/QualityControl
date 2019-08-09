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
/// \file    testInfrastructureGenerator.cxx
/// \author  Piotr Konopka
///

#define BOOST_TEST_MODULE InfrastructureGenerator test
#define BOOST_TEST_MAIN
#define BOOST_TEST_DYN_LINK

#include <boost/test/unit_test.hpp>

#include "QualityControl/InfrastructureGenerator.h"
#include "getTestDataDirectory.h"

#include <Framework/DataSpecUtils.h>

using namespace o2::quality_control::core;
using namespace o2::framework;

BOOST_AUTO_TEST_CASE(qc_factory_local_test)
{
  std::string configFilePath = std::string("json://") + getTestDataDirectory() + "testSharedConfig.json";

  {
    auto workflow = InfrastructureGenerator::generateLocalInfrastructure(configFilePath, "o2flp1");

    BOOST_REQUIRE_EQUAL(workflow.size(), 1);

    BOOST_CHECK_EQUAL(workflow[0].name, "QC-TASK-RUNNER-skeletonTask");
    BOOST_CHECK_EQUAL(workflow[0].inputs.size(), 2);
    BOOST_CHECK_EQUAL(workflow[0].outputs.size(), 1);
    BOOST_CHECK_EQUAL(DataSpecUtils::getOptionalSubSpec(workflow[0].outputs[0]).value_or(-1), 1);
  }

  {
    auto workflow = InfrastructureGenerator::generateLocalInfrastructure(configFilePath, "o2flp2");

    BOOST_REQUIRE_EQUAL(workflow.size(), 1);

    BOOST_CHECK_EQUAL(workflow[0].name, "QC-TASK-RUNNER-skeletonTask");
    BOOST_CHECK_EQUAL(workflow[0].inputs.size(), 2);
    BOOST_CHECK_EQUAL(workflow[0].outputs.size(), 1);
    BOOST_CHECK_EQUAL(DataSpecUtils::getOptionalSubSpec(workflow[0].outputs[0]).value_or(-1), 2);
  }

  {
    auto workflow = InfrastructureGenerator::generateLocalInfrastructure(configFilePath, "o2flp3");

    BOOST_REQUIRE_EQUAL(workflow.size(), 0);
  }
}

BOOST_AUTO_TEST_CASE(qc_factory_remote_test)
{
  std::string configFilePath = std::string("json://") + getTestDataDirectory() + "testSharedConfig.json";
  auto workflow = InfrastructureGenerator::generateRemoteInfrastructure(configFilePath);

  // the infrastructure should consist of a merger and checker for the 'skeletonTask' (its taskRunner is declared to be
  // local) and also taskRunner and checker for the 'abcTask'.
  BOOST_REQUIRE_EQUAL(workflow.size(), 4);

  auto mergerSkeletonTask = std::find_if(
    workflow.begin(), workflow.end(),
    [](const DataProcessorSpec& d) {
      auto concreteInput0 = DataSpecUtils::asConcreteDataMatcher(d.inputs[0]);
      auto concreteInput1 = DataSpecUtils::asConcreteDataMatcher(d.inputs[1]);
      return d.name == "skeletonTask-merger" &&
             d.inputs.size() == 2 && concreteInput0.subSpec == 1 && concreteInput1.subSpec == 2 &&
             d.outputs.size() == 1 && DataSpecUtils::getOptionalSubSpec(d.outputs[0]).value_or(-1) == 0;
    });
  BOOST_CHECK(mergerSkeletonTask != workflow.end());

  auto checkerSkeletonTask = std::find_if(
    workflow.begin(), workflow.end(),
    [](const DataProcessorSpec& d) {
      return d.name == "skeletonTask-checker" &&
             d.inputs.size() == 1 &&
             d.outputs.size() == 1;
    });
  BOOST_CHECK(checkerSkeletonTask != workflow.end());

  auto taskRunnerAbcTask = std::find_if(
    workflow.begin(), workflow.end(),
    [](const DataProcessorSpec& d) {
      return d.name == "QC-TASK-RUNNER-abcTask" &&
             d.inputs.size() == 2 &&
             d.outputs.size() == 1;
    });
  BOOST_CHECK(taskRunnerAbcTask != workflow.end());

  auto checkerAbcTask = std::find_if(
    workflow.begin(), workflow.end(),
    [](const DataProcessorSpec& d) {
      return d.name == "abcTask-checker" &&
             d.inputs.size() == 1 &&
             d.outputs.size() == 1;
    });
  BOOST_CHECK(checkerAbcTask != workflow.end());
}
