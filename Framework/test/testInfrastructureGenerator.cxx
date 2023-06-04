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

#include <catch_amalgamated.hpp>
#include <iostream>

#include "QualityControl/InfrastructureGenerator.h"
#include "getTestDataDirectory.h"

#include <Framework/DataSpecUtils.h>
#include <Configuration/ConfigurationFactory.h>

using namespace o2::quality_control::core;
using namespace o2::framework;
using namespace o2::configuration;

TEST_CASE("qc_factory_local_test")
{
  std::string configFilePath = std::string("json://") + getTestDataDirectory() + "testSharedConfig.json";
  std::cout << configFilePath << std::endl;
  auto configInterface = ConfigurationFactory::getConfiguration(configFilePath);
  auto configTree = configInterface->getRecursive();
  {
    auto workflow = InfrastructureGenerator::generateLocalInfrastructure(configTree, "o2flp1");

    REQUIRE(workflow.size() == 5);

    CHECK(workflow[0].name == "qc-task-TST-skeletonTask");
    CHECK(workflow[0].inputs.size() == 2);
    CHECK(workflow[0].outputs.size() == 1);
    CHECK(DataSpecUtils::getOptionalSubSpec(workflow[0].outputs[0]).value_or(-1) == 1);

    CHECK(workflow[1].name == "TST-skeletonTask-proxy");
    CHECK(workflow[1].inputs.size() == 1);
    CHECK(DataSpecUtils::getOptionalSubSpec(workflow[1].inputs[0]).value_or(-1) == 1);
    CHECK(workflow[1].outputs.size() == 0);

    CHECK(workflow[2].name == "qc-task-GLO-recoTask");
    CHECK(workflow[2].inputs.size() == 13);
    CHECK(workflow[2].outputs.size() == 1);

    CHECK(workflow[3].name == "GLO-recoTask-proxy");
    CHECK(workflow[3].inputs.size() == 1);
    CHECK(workflow[3].outputs.size() == 0);

    CHECK(workflow[4].name == "tpcclust-proxy");
    CHECK(workflow[4].inputs.size() == 1);
    CHECK(workflow[4].outputs.size() == 0);
  }

  {
    auto workflow = InfrastructureGenerator::generateLocalInfrastructure(configTree, "o2flp2");

    REQUIRE(workflow.size() == 2);

    CHECK(workflow[0].name == "qc-task-TST-skeletonTask");
    CHECK(workflow[0].inputs.size() == 2);
    CHECK(workflow[0].outputs.size() == 1);
    CHECK(DataSpecUtils::getOptionalSubSpec(workflow[0].outputs[0]).value_or(-1) == 2);

    CHECK(workflow[1].name == "TST-skeletonTask-proxy");
    CHECK(workflow[1].inputs.size() == 1);
    CHECK(DataSpecUtils::getOptionalSubSpec(workflow[1].inputs[0]).value_or(-1) == 2);
    CHECK(workflow[1].outputs.size() == 0);
  }

  {
    auto workflow = InfrastructureGenerator::generateLocalInfrastructure(configTree, "o2flp3");

    REQUIRE(workflow.size() == 0);
  }
}

TEST_CASE("qc_factory_remote_test")
{
  std::string configFilePath = std::string("json://") + getTestDataDirectory() + "testSharedConfig.json";
  auto configInterface = ConfigurationFactory::getConfiguration(configFilePath);
  auto configTree = configInterface->getRecursive();
  auto workflow = InfrastructureGenerator::generateRemoteInfrastructure(configTree);

  // the infrastructure should consist of two proxies, mergers and checkers for 'skeletonTask' and 'recoTask'
  // (their taskRunner are declared to be local) and also taskRunner and checker for the 'abcTask' and 'xyzTask'.
  // Post processing adds one process for the task and one for checks.
  REQUIRE(workflow.size() == 14);

  auto tcpclustProxy = std::find_if(
    workflow.begin(), workflow.end(),
    [](const DataProcessorSpec& d) {
      return d.name == "tpcclust" &&
             d.inputs.size() == 0 &&
             d.outputs.size() == 1;
    });
  CHECK(tcpclustProxy != workflow.end());

  auto skeletonTaskProxy = std::find_if(
    workflow.begin(), workflow.end(),
    [](const DataProcessorSpec& d) {
      return d.name == "TST-skeletonTask-proxy" &&
             d.inputs.size() == 0 &&
             d.outputs.size() == 2;
    });
  CHECK(skeletonTaskProxy != workflow.end());

  auto recoTaskProxy = std::find_if(
    workflow.begin(), workflow.end(),
    [](const DataProcessorSpec& d) {
      return d.name == "GLO-recoTask-proxy" &&
             d.inputs.size() == 0 &&
             d.outputs.size() == 1;
    });
  CHECK(skeletonTaskProxy != workflow.end());

  auto mergerSkeletonTask = std::find_if(
    workflow.begin(), workflow.end(),
    [](const DataProcessorSpec& d) {
      return d.name.find("TST-MERGER-skeletonTask") != std::string::npos &&
             d.inputs.size() == 3 &&
             d.outputs.size() == 1 && DataSpecUtils::getOptionalSubSpec(d.outputs[0]).value_or(-1) == 0;
    });
  CHECK(mergerSkeletonTask != workflow.end());

  auto mergerRecoTask = std::find_if(
    workflow.begin(), workflow.end(),
    [](const DataProcessorSpec& d) {
      return d.name.find("GLO-MERGER-recoTask") != std::string::npos &&
             d.inputs.size() == 2 &&
             d.outputs.size() == 1 && DataSpecUtils::getOptionalSubSpec(d.outputs[0]).value_or(-1) == 0;
    });
  CHECK(mergerRecoTask != workflow.end());

  auto taskRunnerAbcTask = std::find_if(
    workflow.begin(), workflow.end(),
    [](const DataProcessorSpec& d) {
      return d.name == "qc-task-MISC-abcTask" &&
             d.inputs.size() == 2 &&
             d.outputs.size() == 1;
    });
  CHECK(taskRunnerAbcTask != workflow.end());

  auto taskRunnerXyzTask = std::find_if(
    workflow.begin(), workflow.end(),
    [](const DataProcessorSpec& d) {
      return d.name == "qc-task-ITS-xyzTask" &&
             d.inputs.size() == 2 &&
             d.outputs.size() == 1;
    });
  CHECK(taskRunnerXyzTask != workflow.end());

  // This task shouldn't be generated here - it is local
  auto taskRunnerSkeletonTask = std::find_if(
    workflow.begin(), workflow.end(),
    [](const DataProcessorSpec& d) {
      return d.name == "qc-task-TST-skeletonTask";
    });
  CHECK(taskRunnerSkeletonTask == workflow.end());

  auto checkRunnerCount = std::count_if(
    workflow.begin(), workflow.end(),
    [](const DataProcessorSpec& d) {
      return d.name.find("qc-check") != std::string::npos &&
             d.inputs.size() == 1;
    });
  REQUIRE(checkRunnerCount == 5);

  auto postprocessingTask = std::find_if(
    workflow.begin(), workflow.end(),
    [](const DataProcessorSpec& d) {
      return d.name == "qc-pp-TST-SkeletonPostProcessing" &&
             d.inputs.size() == 1 &&
             d.outputs.size() == 1;
    });
  CHECK(postprocessingTask != workflow.end());

  auto aggregator = std::find_if(
    workflow.begin(), workflow.end(),
    [](const DataProcessorSpec& d) {
      return d.name == "qc-aggregator" &&
             d.inputs.size() == 4 &&
             d.outputs.size() == 0;
    });
  CHECK(aggregator != workflow.end());
}

TEST_CASE("qc_factory_standalone_test")
{
  std::string configFilePath = std::string("json://") + getTestDataDirectory() + "testSharedConfig.json";
  auto configInterface = ConfigurationFactory::getConfiguration(configFilePath);
  auto configTree = configInterface->getRecursive();
  auto workflow = InfrastructureGenerator::generateStandaloneInfrastructure(configTree);

  // the infrastructure should consist of 4 TaskRunners, 1 PostProcessingRunner, 5 CheckRunners (including one for PP), 1 AggregatorRunner
  REQUIRE(workflow.size() == 11);

  auto taskRunnerSkeleton = std::find_if(
    workflow.begin(), workflow.end(),
    [](const DataProcessorSpec& d) {
      return d.name == "qc-task-TST-skeletonTask" &&
             d.inputs.size() == 2 &&
             d.outputs.size() == 1;
    });
  CHECK(taskRunnerSkeleton != workflow.end());

  auto taskRunnerAbcTask = std::find_if(
    workflow.begin(), workflow.end(),
    [](const DataProcessorSpec& d) {
      return d.name == "qc-task-MISC-abcTask" &&
             d.inputs.size() == 2 &&
             d.outputs.size() == 1;
    });
  CHECK(taskRunnerAbcTask != workflow.end());

  auto taskRunnerXyzTask = std::find_if(
    workflow.begin(), workflow.end(),
    [](const DataProcessorSpec& d) {
      return d.name == "qc-task-ITS-xyzTask" &&
             d.inputs.size() == 2 &&
             d.outputs.size() == 1;
    });
  CHECK(taskRunnerXyzTask != workflow.end());

  auto taskRunnerRecoTask = std::find_if(
    workflow.begin(), workflow.end(),
    [](const DataProcessorSpec& d) {
      return d.name == "qc-task-GLO-recoTask" &&
             d.inputs.size() == 13 &&
             d.outputs.size() == 1;
    });
  CHECK(taskRunnerRecoTask != workflow.end());

  auto checkRunnerCount = std::count_if(
    workflow.begin(), workflow.end(),
    [](const DataProcessorSpec& d) {
      return d.name.find("qc-check") != std::string::npos &&
             d.inputs.size() == 1;
    });
  REQUIRE(checkRunnerCount == 5);

  auto postprocessingTask = std::find_if(
    workflow.begin(), workflow.end(),
    [](const DataProcessorSpec& d) {
      return d.name == "qc-pp-TST-SkeletonPostProcessing" &&
             d.inputs.size() == 1 &&
             d.outputs.size() == 1;
    });
  CHECK(postprocessingTask != workflow.end());

  auto aggregator = std::find_if(
    workflow.begin(), workflow.end(),
    [](const DataProcessorSpec& d) {
      return d.name == "qc-aggregator" &&
             d.inputs.size() == 4 &&
             d.outputs.size() == 0;
    });
  CHECK(aggregator != workflow.end());
}

TEST_CASE("qc_factory_empty_config")
{
  std::string configFilePath = std::string("json://") + getTestDataDirectory() + "testEmptyConfig.json";
  auto configInterface = ConfigurationFactory::getConfiguration(configFilePath);
  auto configTree = configInterface->getRecursive();
  {
    WorkflowSpec workflow;
    REQUIRE_NOTHROW(InfrastructureGenerator::generateStandaloneInfrastructure(workflow, configTree));
    CHECK(workflow.size() == 0);
  }
  {
    WorkflowSpec workflow;
    REQUIRE_NOTHROW(InfrastructureGenerator::generateLocalInfrastructure(workflow, configTree, "asdf"));
    CHECK(workflow.size() == 0);
  }
  {
    WorkflowSpec workflow;
    REQUIRE_NOTHROW(InfrastructureGenerator::generateRemoteInfrastructure(workflow, configTree));
    CHECK(workflow.size() == 0);
  }
  {
    WorkflowSpec workflow;
    REQUIRE_NOTHROW(InfrastructureGenerator::generateLocalBatchInfrastructure(workflow, configTree, "file.root"));
    CHECK(workflow.size() == 0);
  }
  {
    WorkflowSpec workflow;
    REQUIRE_NOTHROW(InfrastructureGenerator::generateRemoteBatchInfrastructure(workflow, configTree, "file.root"));
    CHECK(workflow.size() == 0);
  }
}

TEST_CASE("qc_infrastructure_local_batch_test")
{
  std::string configFilePath = std::string("json://") + getTestDataDirectory() + "testSharedConfig.json";
  std::cout << configFilePath << std::endl;
  auto configInterface = ConfigurationFactory::getConfiguration(configFilePath);
  auto configTree = configInterface->getRecursive();
  {
    auto workflow = InfrastructureGenerator::generateLocalBatchInfrastructure(configTree, "file.root");

    REQUIRE(workflow.size() == 5);

    auto taskRunnerSkeleton = std::find_if(
      workflow.begin(), workflow.end(),
      [](const DataProcessorSpec& d) {
        return d.name == "qc-task-TST-skeletonTask" &&
               d.inputs.size() == 2 &&
               d.outputs.size() == 1;
      });
    CHECK(taskRunnerSkeleton != workflow.end());

    auto taskRunnerReco = std::find_if(
      workflow.begin(), workflow.end(),
      [](const DataProcessorSpec& d) {
        return d.name == "qc-task-GLO-recoTask" &&
               d.inputs.size() == 13 &&
               d.outputs.size() == 1;
      });
    CHECK(taskRunnerReco != workflow.end());

    auto taskRunnerAbcTask = std::find_if(
      workflow.begin(), workflow.end(),
      [](const DataProcessorSpec& d) {
        return d.name == "qc-task-MISC-abcTask" &&
               d.inputs.size() == 2 &&
               d.outputs.size() == 1;
      });
    CHECK(taskRunnerAbcTask != workflow.end());

    auto taskRunnerXyzTask = std::find_if(
      workflow.begin(), workflow.end(),
      [](const DataProcessorSpec& d) {
        return d.name == "qc-task-ITS-xyzTask" &&
               d.inputs.size() == 2 &&
               d.outputs.size() == 1;
      });
    CHECK(taskRunnerXyzTask != workflow.end());

    CHECK(workflow[4].name == "qc-root-file-sink");
    CHECK(workflow[4].inputs.size() == 4);
    CHECK(workflow[4].outputs.size() == 0);
  }
}

TEST_CASE("qc_infrastructure_remote_batch_test")
{
  std::string configFilePath = std::string("json://") + getTestDataDirectory() + "testSharedConfig.json";
  auto configInterface = ConfigurationFactory::getConfiguration(configFilePath);
  auto configTree = configInterface->getRecursive();
  auto workflow = InfrastructureGenerator::generateRemoteBatchInfrastructure(configTree, "file.root");

  REQUIRE(workflow.size() == 8);

  auto fileReader = std::find_if(
    workflow.begin(), workflow.end(),
    [](const DataProcessorSpec& d) {
      return d.name == "qc-root-file-source" &&
             d.inputs.size() == 0 &&
             d.outputs.size() == 4;
    });
  CHECK(fileReader != workflow.end());

  auto checkRunnerCount = std::count_if(
    workflow.begin(), workflow.end(),
    [](const DataProcessorSpec& d) {
      return d.name.find("qc-check") != std::string::npos &&
             d.inputs.size() == 1;
    });
  REQUIRE(checkRunnerCount == 5);

  auto postprocessingTask = std::find_if(
    workflow.begin(), workflow.end(),
    [](const DataProcessorSpec& d) {
      return d.name == "qc-pp-TST-SkeletonPostProcessing" &&
             d.inputs.size() == 1 &&
             d.outputs.size() == 1;
    });
  CHECK(postprocessingTask != workflow.end());

  auto aggregator = std::find_if(
    workflow.begin(), workflow.end(),
    [](const DataProcessorSpec& d) {
      return d.name == "qc-aggregator" &&
             d.inputs.size() == 4 &&
             d.outputs.size() == 0;
    });
  CHECK(aggregator != workflow.end());
}
