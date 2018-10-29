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

#include "QualityControl/TaskRunnerFactory.h"
#include "QualityControl/TaskRunner.h"

namespace o2
{
namespace quality_control
{
namespace core
{

using namespace o2::framework;

TaskRunnerFactory::TaskRunnerFactory() {}

TaskRunnerFactory::~TaskRunnerFactory() {}

DataProcessorSpec TaskRunnerFactory::create(std::string taskName, std::string configurationSource)
{
  auto qcTask = std::make_shared<TaskRunner>(taskName, configurationSource);

  DataProcessorSpec newTask{
    taskName,
    qcTask->getInputsSpecs(),
    Outputs{ qcTask->getOutputSpec() },
    AlgorithmSpec{
      (AlgorithmSpec::InitCallback) [qcTask = std::move(qcTask)](InitContext& initContext) {

        qcTask->initCallback(initContext);

        return (AlgorithmSpec::ProcessCallback) [qcTask = std::move(qcTask)] (ProcessingContext &processingContext) {
          qcTask->processCallback(processingContext);
        };
      }
    }
  };

  return std::move(newTask);
}

} // namespace core
} // namespace quality_control
} // namespace o2
