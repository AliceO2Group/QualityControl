///
/// \file   TaskDataProcessorFactory.h
/// \author Piotr Konopka
///

#ifndef QC_CORE_TASKDATAPROCESSORFACTORY_H
#define QC_CORE_TASKDATAPROCESSORFACTORY_H

#include "Framework/DataProcessorSpec.h"

namespace o2
{
namespace quality_control
{
namespace core
{

/// \brief Factory in charge of creating DataProcessorSpec of QC task
class TaskDataProcessorFactory
{
 public:
  TaskDataProcessorFactory();
  virtual ~TaskDataProcessorFactory();

  o2::framework::DataProcessorSpec create(std::string taskName, std::string configurationSource);
};

} // namespace core
} // namespace quality_control
} // namespace o2

#endif // QC_CORE_TASKDATAPROCESSORFACTORY_H
