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
#include <Configuration/ConfigurationFactory.h>

using namespace o2::quality_control::core;
using namespace o2::framework;
using namespace o2::configuration;

BOOST_AUTO_TEST_CASE(qc_factory_local_test)
{
  std::string configFilePath = std::string("json://") + getTestDataDirectory() + "testSharedConfig.json";
  std::cout << configFilePath << std::endl;
  auto configInterface = ConfigurationFactory::getConfiguration(configFilePath);
  auto configTree = configInterface->getRecursive();
  {
    auto workflow = InfrastructureGenerator::generateLocalInfrastructure(configTree, "o2flp1");

    BOOST_REQUIRE_EQUAL(workflow.size(), 5);

    BOOST_CHECK_EQUAL(workflow[0].name, "qc-task-TST-skeletonTask");
    BOOST_CHECK_EQUAL(workflow[0].inputs.size(), 2);
    BOOST_CHECK_EQUAL(workflow[0].outputs.size(), 1);
    BOOST_CHECK_EQUAL(DataSpecUtils::getOptionalSubSpec(workflow[0].outputs[0]).value_or(-1), 1);

    BOOST_CHECK_EQUAL(workflow[1].name, "TST-skeletonTask-proxy");
    BOOST_CHECK_EQUAL(workflow[1].inputs.size(), 1);
    BOOST_CHECK_EQUAL(DataSpecUtils::getOptionalSubSpec(workflow[1].inputs[0]).value_or(-1), 1);
    BOOST_CHECK_EQUAL(workflow[1].outputs.size(), 0);

    BOOST_CHECK_EQUAL(workflow[2].name, "qc-task-GLO-recoTask");
    BOOST_CHECK_EQUAL(workflow[2].inputs.size(), 13);
    BOOST_CHECK_EQUAL(workflow[2].outputs.size(), 1);

    BOOST_CHECK_EQUAL(workflow[3].name, "GLO-recoTask-proxy");
    BOOST_CHECK_EQUAL(workflow[3].inputs.size(), 1);
    BOOST_CHECK_EQUAL(workflow[3].outputs.size(), 0);

    BOOST_CHECK_EQUAL(workflow[4].name, "tpcclust-proxy");
    BOOST_CHECK_EQUAL(workflow[4].inputs.size(), 1);
    BOOST_CHECK_EQUAL(workflow[4].outputs.size(), 0);
  }

  {
    auto workflow = InfrastructureGenerator::generateLocalInfrastructure(configTree, "o2flp2");

    BOOST_REQUIRE_EQUAL(workflow.size(), 2);

    BOOST_CHECK_EQUAL(workflow[0].name, "qc-task-TST-skeletonTask");
    BOOST_CHECK_EQUAL(workflow[0].inputs.size(), 2);
    BOOST_CHECK_EQUAL(workflow[0].outputs.size(), 1);
    BOOST_CHECK_EQUAL(DataSpecUtils::getOptionalSubSpec(workflow[0].outputs[0]).value_or(-1), 2);

    BOOST_CHECK_EQUAL(workflow[1].name, "TST-skeletonTask-proxy");
    BOOST_CHECK_EQUAL(workflow[1].inputs.size(), 1);
    BOOST_CHECK_EQUAL(DataSpecUtils::getOptionalSubSpec(workflow[1].inputs[0]).value_or(-1), 2);
    BOOST_CHECK_EQUAL(workflow[1].outputs.size(), 0);
  }

  {
    auto workflow = InfrastructureGenerator::generateLocalInfrastructure(configTree, "o2flp3");

    BOOST_REQUIRE_EQUAL(workflow.size(), 0);
  }
}

BOOST_AUTO_TEST_CASE(qc_factory_remote_test)
{
  std::string configFilePath = std::string("json://") + getTestDataDirectory() + "testSharedConfig.json";
  auto configInterface = ConfigurationFactory::getConfiguration(configFilePath);
  auto configTree = configInterface->getRecursive();
  auto workflow = InfrastructureGenerator::generateRemoteInfrastructure(configTree);

  // the infrastructure should consist of two proxies, mergers and checkers for 'skeletonTask' and 'recoTask'
  // (their taskRunner are declared to be local) and also taskRunner and checker for the 'abcTask' and 'xyzTask'.
  // Post processing adds one process for the task and one for checks.
  BOOST_REQUIRE_EQUAL(workflow.size(), 14);

  auto tcpclustProxy = std::find_if(
    workflow.begin(), workflow.end(),
    [](const DataProcessorSpec& d) {
      return d.name == "tpcclust" &&
             d.inputs.size() == 0 &&
             d.outputs.size() == 1;
    });
  BOOST_CHECK(tcpclustProxy != workflow.end());

  auto skeletonTaskProxy = std::find_if(
    workflow.begin(), workflow.end(),
    [](const DataProcessorSpec& d) {
      return d.name == "TST-skeletonTask-proxy" &&
             d.inputs.size() == 0 &&
             d.outputs.size() == 2;
    });
  BOOST_CHECK(skeletonTaskProxy != workflow.end());

  auto recoTaskProxy = std::find_if(
    workflow.begin(), workflow.end(),
    [](const DataProcessorSpec& d) {
      return d.name == "GLO-recoTask-proxy" &&
             d.inputs.size() == 0 &&
             d.outputs.size() == 1;
    });
  BOOST_CHECK(skeletonTaskProxy != workflow.end());

  auto mergerSkeletonTask = std::find_if(
    workflow.begin(), workflow.end(),
    [](const DataProcessorSpec& d) {
      return d.name.find("TST-MERGER-skeletonTask") != std::string::npos &&
             d.inputs.size() == 3 &&
             d.outputs.size() == 1 && DataSpecUtils::getOptionalSubSpec(d.outputs[0]).value_or(-1) == 0;
    });
  BOOST_CHECK(mergerSkeletonTask != workflow.end());

  auto mergerRecoTask = std::find_if(
    workflow.begin(), workflow.end(),
    [](const DataProcessorSpec& d) {
      return d.name.find("GLO-MERGER-recoTask") != std::string::npos &&
             d.inputs.size() == 2 &&
             d.outputs.size() == 1 && DataSpecUtils::getOptionalSubSpec(d.outputs[0]).value_or(-1) == 0;
    });
  BOOST_CHECK(mergerRecoTask != workflow.end());

  auto taskRunnerAbcTask = std::find_if(
    workflow.begin(), workflow.end(),
    [](const DataProcessorSpec& d) {
      return d.name == "qc-task-MISC-abcTask" &&
             d.inputs.size() == 2 &&
             d.outputs.size() == 1;
    });
  BOOST_CHECK(taskRunnerAbcTask != workflow.end());

  auto taskRunnerXyzTask = std::find_if(
    workflow.begin(), workflow.end(),
    [](const DataProcessorSpec& d) {
      return d.name == "qc-task-ITS-xyzTask" &&
             d.inputs.size() == 2 &&
             d.outputs.size() == 1;
    });
  BOOST_CHECK(taskRunnerXyzTask != workflow.end());

  // This task shouldn't be generated here - it is local
  auto taskRunnerSkeletonTask = std::find_if(
    workflow.begin(), workflow.end(),
    [](const DataProcessorSpec& d) {
      return d.name == "qc-task-TST-skeletonTask";
    });
  BOOST_CHECK(taskRunnerSkeletonTask == workflow.end());

  auto checkRunnerCount = std::count_if(
    workflow.begin(), workflow.end(),
    [](const DataProcessorSpec& d) {
      return d.name.find("qc-check") != std::string::npos &&
             d.inputs.size() == 1;
    });
  BOOST_REQUIRE_EQUAL(checkRunnerCount, 5);

  auto postprocessingTask = std::find_if(
    workflow.begin(), workflow.end(),
    [](const DataProcessorSpec& d) {
      return d.name == "PP-TASK-RUNNER-SkeletonPostProcessing" &&
             d.inputs.size() == 1 &&
             d.outputs.size() == 1;
    });
  BOOST_CHECK(postprocessingTask != workflow.end());

  auto aggregator = std::find_if(
    workflow.begin(), workflow.end(),
    [](const DataProcessorSpec& d) {
      return d.name == "QC-AGGREGATOR-RUNNER" &&
             d.inputs.size() == 4 &&
             d.outputs.size() == 0;
    });
  BOOST_CHECK(aggregator != workflow.end());
}

BOOST_AUTO_TEST_CASE(qc_factory_standalone_test)
{
  std::string configFilePath = std::string("json://") + getTestDataDirectory() + "testSharedConfig.json";
  auto configInterface = ConfigurationFactory::getConfiguration(configFilePath);
  auto configTree = configInterface->getRecursive();
  auto workflow = InfrastructureGenerator::generateStandaloneInfrastructure(configTree);

  // the infrastructure should consist of 4 TaskRunners, 1 PostProcessingRunner, 5 CheckRunners (including one for PP), 1 AggregatorRunner
  BOOST_REQUIRE_EQUAL(workflow.size(), 11);

  auto taskRunnerSkeleton = std::find_if(
    workflow.begin(), workflow.end(),
    [](const DataProcessorSpec& d) {
      return d.name == "qc-task-TST-skeletonTask" &&
             d.inputs.size() == 2 &&
             d.outputs.size() == 1;
    });
  BOOST_CHECK(taskRunnerSkeleton != workflow.end());

  auto taskRunnerAbcTask = std::find_if(
    workflow.begin(), workflow.end(),
    [](const DataProcessorSpec& d) {
      return d.name == "qc-task-MISC-abcTask" &&
             d.inputs.size() == 2 &&
             d.outputs.size() == 1;
    });
  BOOST_CHECK(taskRunnerAbcTask != workflow.end());

  auto taskRunnerXyzTask = std::find_if(
    workflow.begin(), workflow.end(),
    [](const DataProcessorSpec& d) {
      return d.name == "qc-task-ITS-xyzTask" &&
             d.inputs.size() == 2 &&
             d.outputs.size() == 1;
    });
  BOOST_CHECK(taskRunnerXyzTask != workflow.end());

  auto taskRunnerRecoTask = std::find_if(
    workflow.begin(), workflow.end(),
    [](const DataProcessorSpec& d) {
      return d.name == "qc-task-GLO-recoTask" &&
             d.inputs.size() == 13 &&
             d.outputs.size() == 1;
    });
  BOOST_CHECK(taskRunnerRecoTask != workflow.end());

  auto checkRunnerCount = std::count_if(
    workflow.begin(), workflow.end(),
    [](const DataProcessorSpec& d) {
      return d.name.find("qc-check") != std::string::npos &&
             d.inputs.size() == 1;
    });
  BOOST_REQUIRE_EQUAL(checkRunnerCount, 5);

  auto postprocessingTask = std::find_if(
    workflow.begin(), workflow.end(),
    [](const DataProcessorSpec& d) {
      return d.name == "PP-TASK-RUNNER-SkeletonPostProcessing" &&
             d.inputs.size() == 1 &&
             d.outputs.size() == 1;
    });
  BOOST_CHECK(postprocessingTask != workflow.end());

  auto aggregator = std::find_if(
    workflow.begin(), workflow.end(),
    [](const DataProcessorSpec& d) {
      return d.name == "QC-AGGREGATOR-RUNNER" &&
             d.inputs.size() == 4 &&
             d.outputs.size() == 0;
    });
  BOOST_CHECK(aggregator != workflow.end());
}

BOOST_AUTO_TEST_CASE(qc_factory_empty_config)
{
  std::string configFilePath = std::string("json://") + getTestDataDirectory() + "testEmptyConfig.json";
  auto configInterface = ConfigurationFactory::getConfiguration(configFilePath);
  auto configTree = configInterface->getRecursive();
  {
    WorkflowSpec workflow;
    BOOST_REQUIRE_NO_THROW(InfrastructureGenerator::generateStandaloneInfrastructure(workflow, configTree));
    BOOST_CHECK_EQUAL(workflow.size(), 0);
  }
  {
    WorkflowSpec workflow;
    BOOST_REQUIRE_NO_THROW(InfrastructureGenerator::generateLocalInfrastructure(workflow, configTree, "asdf"));
    BOOST_CHECK_EQUAL(workflow.size(), 0);
  }
  {
    WorkflowSpec workflow;
    BOOST_REQUIRE_NO_THROW(InfrastructureGenerator::generateRemoteInfrastructure(workflow, configTree));
    BOOST_CHECK_EQUAL(workflow.size(), 0);
  }
  {
    WorkflowSpec workflow;
    BOOST_REQUIRE_NO_THROW(InfrastructureGenerator::generateLocalBatchInfrastructure(workflow, configTree, "file.root"));
    BOOST_CHECK_EQUAL(workflow.size(), 0);
  }
  {
    WorkflowSpec workflow;
    BOOST_REQUIRE_NO_THROW(InfrastructureGenerator::generateRemoteBatchInfrastructure(workflow, configTree, "file.root"));
    BOOST_CHECK_EQUAL(workflow.size(), 0);
  }
}

BOOST_AUTO_TEST_CASE(qc_infrastructure_local_batch_test)
{
  std::string configFilePath = std::string("json://") + getTestDataDirectory() + "testSharedConfig.json";
  std::cout << configFilePath << std::endl;
  auto configInterface = ConfigurationFactory::getConfiguration(configFilePath);
  auto configTree = configInterface->getRecursive();
  {
    auto workflow = InfrastructureGenerator::generateLocalBatchInfrastructure(configTree, "file.root");

    BOOST_REQUIRE_EQUAL(workflow.size(), 5);

    auto taskRunnerSkeleton = std::find_if(
      workflow.begin(), workflow.end(),
      [](const DataProcessorSpec& d) {
        return d.name == "qc-task-TST-skeletonTask" &&
               d.inputs.size() == 2 &&
               d.outputs.size() == 1;
      });
    BOOST_CHECK(taskRunnerSkeleton != workflow.end());

    auto taskRunnerReco = std::find_if(
      workflow.begin(), workflow.end(),
      [](const DataProcessorSpec& d) {
        return d.name == "qc-task-GLO-recoTask" &&
               d.inputs.size() == 13 &&
               d.outputs.size() == 1;
      });
    BOOST_CHECK(taskRunnerReco != workflow.end());

    auto taskRunnerAbcTask = std::find_if(
      workflow.begin(), workflow.end(),
      [](const DataProcessorSpec& d) {
        return d.name == "qc-task-MISC-abcTask" &&
               d.inputs.size() == 2 &&
               d.outputs.size() == 1;
      });
    BOOST_CHECK(taskRunnerAbcTask != workflow.end());

    auto taskRunnerXyzTask = std::find_if(
      workflow.begin(), workflow.end(),
      [](const DataProcessorSpec& d) {
        return d.name == "qc-task-ITS-xyzTask" &&
               d.inputs.size() == 2 &&
               d.outputs.size() == 1;
      });
    BOOST_CHECK(taskRunnerXyzTask != workflow.end());

    BOOST_CHECK_EQUAL(workflow[4].name, "qc-root-file-sink");
    BOOST_CHECK_EQUAL(workflow[4].inputs.size(), 4);
    BOOST_CHECK_EQUAL(workflow[4].outputs.size(), 0);
  }
}

BOOST_AUTO_TEST_CASE(qc_infrastructure_remote_batch_test)
{
  std::string configFilePath = std::string("json://") + getTestDataDirectory() + "testSharedConfig.json";
  auto configInterface = ConfigurationFactory::getConfiguration(configFilePath);
  auto configTree = configInterface->getRecursive();
  auto workflow = InfrastructureGenerator::generateRemoteBatchInfrastructure(configTree, "file.root");

  BOOST_REQUIRE_EQUAL(workflow.size(), 8);

  auto fileReader = std::find_if(
    workflow.begin(), workflow.end(),
    [](const DataProcessorSpec& d) {
      return d.name == "qc-root-file-source" &&
             d.inputs.size() == 0 &&
             d.outputs.size() == 4;
    });
  BOOST_CHECK(fileReader != workflow.end());

  auto checkRunnerCount = std::count_if(
    workflow.begin(), workflow.end(),
    [](const DataProcessorSpec& d) {
      return d.name.find("qc-check") != std::string::npos &&
             d.inputs.size() == 1;
    });
  BOOST_REQUIRE_EQUAL(checkRunnerCount, 5);

  auto postprocessingTask = std::find_if(
    workflow.begin(), workflow.end(),
    [](const DataProcessorSpec& d) {
      return d.name == "PP-TASK-RUNNER-SkeletonPostProcessing" &&
             d.inputs.size() == 1 &&
             d.outputs.size() == 1;
    });
  BOOST_CHECK(postprocessingTask != workflow.end());

  auto aggregator = std::find_if(
    workflow.begin(), workflow.end(),
    [](const DataProcessorSpec& d) {
      return d.name == "QC-AGGREGATOR-RUNNER" &&
             d.inputs.size() == 4 &&
             d.outputs.size() == 0;
    });
  BOOST_CHECK(aggregator != workflow.end());
}