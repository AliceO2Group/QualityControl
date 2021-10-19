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
/// \file    testTaskRunner.cxx
/// \author  Piotr Konopka
///

#include "getTestDataDirectory.h"
#include "QualityControl/TaskRunnerFactory.h"
#include "QualityControl/TaskRunner.h"
#include <Framework/DataSpecUtils.h>
#include <DataSampling/DataSampling.h>
#include "QualityControl/InfrastructureSpecReader.h"
#include "Configuration/ConfigurationFactory.h"
#include "Configuration/ConfigurationInterface.h"

#define BOOST_TEST_MODULE TaskRunner test
#define BOOST_TEST_MAIN
#define BOOST_TEST_DYN_LINK

#include <boost/test/unit_test.hpp>

using namespace o2::quality_control::core;
using namespace std;
using namespace o2::framework;
using namespace o2::header;
using namespace o2::utilities;
using namespace o2::configuration;

TaskRunnerConfig getTaskConfig(const std::string& configFilePath, const std::string& taskName, size_t id)
{
  auto config = ConfigurationFactory::getConfiguration(configFilePath);
  auto infrastructureSpec = InfrastructureSpecReader::readInfrastructureSpec(config->getRecursive());

  auto taskSpec = std::find_if(infrastructureSpec.tasks.begin(), infrastructureSpec.tasks.end(), [&taskName](const auto& taskSpec) {
    return taskSpec.taskName == taskName;
  });
  if (taskSpec != infrastructureSpec.tasks.end()) {
    return TaskRunnerFactory::extractConfig(infrastructureSpec.common, *taskSpec, id);
  } else {
    throw std::runtime_error("task " + taskName + " not found in the config file");
  }
}

BOOST_AUTO_TEST_CASE(test_factory)
{
  std::string configFilePath = std::string("json://") + getTestDataDirectory() + "testSharedConfig.json";

  DataProcessorSpec taskRunner = TaskRunnerFactory::create(getTaskConfig(configFilePath, "abcTask", 123));

  BOOST_CHECK_EQUAL(taskRunner.name, "QC-TASK-RUNNER-abcTask");

  auto dataSamplingTree = ConfigurationFactory::getConfiguration(configFilePath)->getRecursive("dataSamplingPolicies");
  BOOST_REQUIRE_EQUAL(taskRunner.inputs.size(), 2);
  BOOST_CHECK_EQUAL(taskRunner.inputs[0], DataSampling::InputSpecsForPolicy(dataSamplingTree, "tpcclust").at(0));
  BOOST_CHECK(taskRunner.inputs[1].lifetime == Lifetime::Timer);

  BOOST_REQUIRE_EQUAL(taskRunner.outputs.size(), 1);
  BOOST_CHECK_EQUAL(taskRunner.outputs[0], (OutputSpec{ { "mo" }, "QC", "abcTask-mo", 123 }));

  BOOST_CHECK(taskRunner.algorithm.onInit != nullptr);

  BOOST_REQUIRE_EQUAL(taskRunner.options.size(), 2);
  BOOST_CHECK_EQUAL(taskRunner.options[0].name, "period-timer-cycle");
}

BOOST_AUTO_TEST_CASE(test_task_runner_static)
{
  BOOST_CHECK_EQUAL(TaskRunner::createTaskDataOrigin(), DataOrigin("QC"));
  BOOST_CHECK(TaskRunner::createTaskDataDescription("qwertyuiop") == DataDescription("qwertyuiop-mo"));
  BOOST_CHECK(TaskRunner::createTaskDataDescription("012345678901234567890") == DataDescription("0123456789012-mo"));
  BOOST_CHECK_THROW(TaskRunner::createTaskDataDescription(""), AliceO2::Common::FatalException);
  BOOST_CHECK_EQUAL(TaskRunner::createTaskRunnerIdString(), "QC-TASK-RUNNER");
}

BOOST_AUTO_TEST_CASE(test_task_runner)
{
  std::string configFilePath = std::string("json://") + getTestDataDirectory() + "testSharedConfig.json";
  TaskRunner qcTask{ getTaskConfig(configFilePath, "abcTask", 0) };

  BOOST_CHECK_EQUAL(qcTask.getDeviceName(), "QC-TASK-RUNNER-abcTask");

  auto dataSamplingTree = ConfigurationFactory::getConfiguration(configFilePath)->getRecursive("dataSamplingPolicies");
  BOOST_REQUIRE_EQUAL(qcTask.getInputsSpecs().size(), 2);
  BOOST_CHECK_EQUAL(qcTask.getInputsSpecs()[0], DataSampling::InputSpecsForPolicy(dataSamplingTree, "tpcclust").at(0));
  BOOST_CHECK(qcTask.getInputsSpecs()[1].lifetime == Lifetime::Timer);

  BOOST_CHECK_EQUAL(qcTask.getOutputSpec(), (OutputSpec{ { "mo" }, "QC", "abcTask-mo", 0 }));

  BOOST_REQUIRE_EQUAL(qcTask.getOptions().size(), 2);
  BOOST_CHECK_EQUAL(qcTask.getOptions()[0].name, "period-timer-cycle");

  // This is maximum that we can do until we are able to test the DPL algorithms in isolation.
  // TODO: When it is possible, we should try calling run() and init()
}

BOOST_AUTO_TEST_CASE(test_task_wrong_detector_name)
{
  std::string configFilePath = std::string("json://") + getTestDataDirectory() + "testSharedConfig.json";

  DataProcessorSpec taskRunner = TaskRunnerFactory::create(getTaskConfig(configFilePath, "abcTask", 0));
  //  cout << "It should print an error message" << endl;
}

BOOST_AUTO_TEST_CASE(test_task_good_detector_name)
{
  std::string configFilePath = std::string("json://") + getTestDataDirectory() + "testSharedConfig.json";

  DataProcessorSpec taskRunner = TaskRunnerFactory::create(getTaskConfig(configFilePath, "xyzTask", 0));

  //  cout << "no error message" << endl;
}