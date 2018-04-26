///
/// \file   TaskDataProcessorFactory.cxx
/// \author Piotr Konopka
///

#include "QualityControl/TaskDataProcessorFactory.h"
#include "QualityControl/TaskDataProcessor.h"

namespace o2 {
namespace quality_control {
namespace core {

using namespace o2::framework;

TaskDataProcessorFactory::TaskDataProcessorFactory()
{
}

TaskDataProcessorFactory::~TaskDataProcessorFactory()
{
}

DataProcessorSpec TaskDataProcessorFactory::create(std::string taskName, std::string configurationSource)
{
  auto qcTask = std::make_shared<TaskDataProcessor>(taskName, configurationSource);

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
