///
/// \file   TaskRunnerFactory.h
/// \author Piotr Konopka
///

#ifndef QC_CORE_TASKFACTORY_H
#define QC_CORE_TASKFACTORY_H

#include "Framework/DataProcessorSpec.h"

namespace o2
{
namespace quality_control
{
namespace core
{

/// \brief Factory in charge of creating DataProcessorSpec of QC task
class TaskRunnerFactory
{
 public:
  TaskRunnerFactory();
  virtual ~TaskRunnerFactory();

  o2::framework::DataProcessorSpec create(std::string taskName, std::string configurationSource, size_t id = 0);
};

} // namespace core
} // namespace quality_control
} // namespace o2

#endif // QC_CORE_TASKFACTORY_H
