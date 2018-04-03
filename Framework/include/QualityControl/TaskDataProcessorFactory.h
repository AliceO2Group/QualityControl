///
/// \file   TaskDataProcessorFactory.h
/// \author Piotr Konopka
///

#ifndef PROJECT_TASKDATAPROCESSORFACTORY_H
#define PROJECT_TASKDATAPROCESSORFACTORY_H

#include "Framework/DataProcessorSpec.h"

namespace o2 {
namespace quality_control {
namespace core {

/// \brief Factory in charge of creating DataProcessorSpec of QC task
class TaskDataProcessorFactory
{
 public:
  TaskDataProcessorFactory();
  virtual ~TaskDataProcessorFactory();

  o2::framework::DataProcessorSpec create(std::string taskName, std::string configurationSource);
};

} // namespace core
} // namespace QualityControl
} // namespace o2

#endif //PROJECT_TASKDATAPROCESSORFACTORY_H
