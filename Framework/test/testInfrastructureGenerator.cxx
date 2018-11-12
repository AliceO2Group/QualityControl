///
/// \file    testQCFactory.cxx
/// \author  Piotr Konopka
///

#define BOOST_TEST_MODULE QCFactory test
#define BOOST_TEST_MAIN
#define BOOST_TEST_DYN_LINK

#include <boost/test/unit_test.hpp>
#include <boost/property_tree/json_parser.hpp>

#include "QualityControl/InfrastructureGenerator.h"

using namespace o2::quality_control::core;
using namespace o2::framework;

BOOST_AUTO_TEST_CASE(qc_factory_local_test)
{
  std::string configFilePath = std::string("json:/") + getenv("QUALITYCONTROL_ROOT") + "/test/testQCFactory.json";

  {
    auto workflow = InfrastructureGenerator::generateLocalInfrastructure(configFilePath, "o2flp1");

    BOOST_REQUIRE_EQUAL(workflow.size(), 1);

    BOOST_CHECK_EQUAL(workflow[0].name, "skeletonTask");
    BOOST_CHECK_EQUAL(workflow[0].inputs.size(), 1);
    BOOST_CHECK_EQUAL(workflow[0].outputs.size(), 1);
    BOOST_CHECK_EQUAL(workflow[0].outputs[0].subSpec, 1);
  }

  {
    auto workflow = InfrastructureGenerator::generateLocalInfrastructure(configFilePath, "o2flp2");

    BOOST_REQUIRE_EQUAL(workflow.size(), 1);

    BOOST_CHECK_EQUAL(workflow[0].name, "skeletonTask");
    BOOST_CHECK_EQUAL(workflow[0].inputs.size(), 1);
    BOOST_CHECK_EQUAL(workflow[0].outputs.size(), 1);
    BOOST_CHECK_EQUAL(workflow[0].outputs[0].subSpec, 2);
  }

  {
    auto workflow = InfrastructureGenerator::generateLocalInfrastructure(configFilePath, "o2flp3");

    BOOST_REQUIRE_EQUAL(workflow.size(), 0);
  }
}

BOOST_AUTO_TEST_CASE(qc_factory_remote_test)
{
  std::string configFilePath = std::string("json:/") + getenv("QUALITYCONTROL_ROOT") + "/test/testQCFactory.json";
  auto workflow = InfrastructureGenerator::generateRemoteInfrastructure(configFilePath);

  // the infrastructure should consist of a merger and checker for the 'skeletonTask' (its taskRunner is declared to be
  // local) and also taskRunner and checker for the 'abcTask'.
  BOOST_REQUIRE_EQUAL(workflow.size(), 4);

  auto mergerSkeletonTask = std::find_if(
    workflow.begin(), workflow.end(),
    [](const DataProcessorSpec& d) {
      return d.name == "skeletonTask-merger" &&
             d.inputs.size() == 2 && d.inputs[0].subSpec == 1 && d.inputs[1].subSpec == 2 &&
             d.outputs.size() == 1 && d.outputs[0].subSpec == 0;
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
      return d.name == "abcTask" &&
             d.inputs.size() == 1 &&
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