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
/// \file   TaskRunnerFactory.cxx
/// \author Piotr Konopka
///

#include <Framework/DeviceSpec.h>
#include "QualityControl/TaskRunnerFactory.h"
#include "QualityControl/TaskRunner.h"

namespace o2::quality_control::core
{

using namespace o2::framework;

o2::framework::DataProcessorSpec
TaskRunnerFactory::create(std::string taskName, std::string configurationSource, size_t id, bool resetAfterPublish)
{
  TaskRunner qcTask{ taskName, configurationSource, id };
  qcTask.setResetAfterPublish(resetAfterPublish);

  DataProcessorSpec newTask{
    qcTask.getDeviceName(),
    qcTask.getInputsSpecs(),
    Outputs{ qcTask.getOutputSpec() },
    AlgorithmSpec{},
    qcTask.getOptions()
  };
  // this needs to be moved at the end
  newTask.algorithm = adaptFromTask<TaskRunner>(std::move(qcTask));

  return newTask;
}

void TaskRunnerFactory::customizeInfrastructure(std::vector<framework::CompletionPolicy>& policies)
{
  auto matcher = [](framework::DeviceSpec const& device) {
    return device.name.find(TaskRunner::createTaskRunnerIdString()) != std::string::npos;
  };
  auto callback = TaskRunner::completionPolicyCallback;

  framework::CompletionPolicy taskRunnerCompletionPolicy{ "taskRunnerCompletionPolicy", matcher, callback };
  policies.push_back(taskRunnerCompletionPolicy);
}

} // namespace o2::quality_control::core
