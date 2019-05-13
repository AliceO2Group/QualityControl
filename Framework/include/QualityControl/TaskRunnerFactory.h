///
/// \file   TaskRunnerFactory.h
/// \author Piotr Konopka
///

#ifndef QC_CORE_TASKFACTORY_H
#define QC_CORE_TASKFACTORY_H

#include "Framework/DataProcessorSpec.h"

namespace o2::quality_control::core
{

/// \brief Factory in charge of creating DataProcessorSpec of QC task
class TaskRunnerFactory
{
 public:
  TaskRunnerFactory() = default;
  virtual ~TaskRunnerFactory() = default;

  /// \brief Creator of tasks
  ///
  /// \param taskName - name of the task, which exists in tasks list in the configuration file
  /// \param configurationSource - absolute path to configuration file, preceded with backend (f.e. "json://")
  /// \param id - subSpecification for taskRunner's OutputSpec, useful to avoid outputs collisions one more complex topologies
  /// \param resetAfterPublish - should taskRunner reset the user's task after each MO publication
  o2::framework::DataProcessorSpec
  create(std::string taskName, std::string configurationSource, size_t id = 0, bool resetAfterPublish = false);
};

} // namespace o2::quality_control::core

#endif // QC_CORE_TASKFACTORY_H
