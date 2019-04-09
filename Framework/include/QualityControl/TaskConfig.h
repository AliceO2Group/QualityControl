///
/// \file   TaskConfig.h
/// \author Barthelemy von Haller
///

#ifndef QC_CORE_TASKCONFIG_H
#define QC_CORE_TASKCONFIG_H

#include <string>

namespace o2::quality_control::core
{

/// \brief  Container for the configuration of a Task
///
/// \author Barthelemy von Haller
struct TaskConfig {
  std::string taskName;
  std::string moduleName;
  std::string className;
  int cycleDurationSeconds;
  int maxNumberCycles;
};

} // namespace o2::quality_control::core

#endif // QC_CORE_TASKCONFIG_H
