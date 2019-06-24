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
/// \file    testTaskRunner.cxx
/// \author  Piotr Konopka
///

#include "QualityControl/TaskRunnerFactory.h"
#include "QualityControl/TaskRunner.h"
#include <Framework/DataSampling.h>
#include <Framework/DataSpecUtils.h>

#define BOOST_TEST_MODULE TaskRunner test
#define BOOST_TEST_MAIN
#define BOOST_TEST_DYN_LINK

#include <boost/test/unit_test.hpp>
#include <iostream>

using namespace o2::quality_control::core;
using namespace std;
using namespace o2::framework;
using namespace o2::header;

BOOST_AUTO_TEST_CASE(test_factory)
{
  std::string configFilePath =
    std::string("json:/") + getenv("QUALITYCONTROL_ROOT") + "/tests/testSharedConfig.json";

  TaskRunnerFactory taskRunnerFactory;
  DataProcessorSpec taskRunner = taskRunnerFactory.create("abcTask", configFilePath, 123);

  BOOST_CHECK_EQUAL(taskRunner.name, "QC-TASK-RUNNER-abcTask");

  BOOST_REQUIRE_EQUAL(taskRunner.inputs.size(), 2);
  BOOST_CHECK_EQUAL(taskRunner.inputs[0], DataSampling::InputSpecsForPolicy(configFilePath, "tpcclust").at(0));
  BOOST_CHECK(taskRunner.inputs[1].lifetime == Lifetime::Timer);

  BOOST_REQUIRE_EQUAL(taskRunner.outputs.size(), 1);
  BOOST_CHECK_EQUAL(taskRunner.outputs[0], (OutputSpec{ { "mo" }, "QC", "abcTask-mo", 123 }));

  BOOST_CHECK(taskRunner.algorithm.onInit != nullptr);

  BOOST_REQUIRE_EQUAL(taskRunner.options.size(), 1);
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
  std::string configFilePath =
    std::string("json:/") + getenv("QUALITYCONTROL_ROOT") + "/tests/testSharedConfig.json";

  TaskRunner qcTask{ "abcTask", configFilePath, 0 };

  BOOST_CHECK_EQUAL(qcTask.getDeviceName(), "QC-TASK-RUNNER-abcTask");

  BOOST_REQUIRE_EQUAL(qcTask.getInputsSpecs().size(), 2);
  BOOST_CHECK_EQUAL(qcTask.getInputsSpecs()[0], DataSampling::InputSpecsForPolicy(configFilePath, "tpcclust").at(0));
  BOOST_CHECK(qcTask.getInputsSpecs()[1].lifetime == Lifetime::Timer);

  BOOST_CHECK_EQUAL(qcTask.getOutputSpec(), (OutputSpec{ { "mo" }, "QC", "abcTask-mo", 0 }));

  BOOST_REQUIRE_EQUAL(qcTask.getOptions().size(), 1);
  BOOST_CHECK_EQUAL(qcTask.getOptions()[0].name, "period-timer-cycle");

  // This is maximum that we can do until we are able to test the DPL algorithms in isolation.
  // When it is possible, we should try calling run() and init()
}