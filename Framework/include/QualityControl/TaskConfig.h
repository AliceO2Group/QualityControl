///
/// \file   TaskConfig.h
/// \author Barthelemy von Haller
///

#ifndef QC_CORE_TASKCONFIG_H
#define QC_CORE_TASKCONFIG_H

#include <string>

namespace o2 {
namespace quality_control {
namespace core {

/// \brief  Container for the configuration of a Task
///
/// \author Barthelemy von Haller
struct TaskConfig
{
    std::string taskName;
    std::string moduleName;
    std::string className;
//    int numberHistos;
//    int numberChecks;
//    std::string typeOfChecks;
    int cycleDurationSeconds;
    int maxNumberCycles;
};

} // namespace core
} // namespace quality_control
} // namespace o2

#endif // QC_CORE_TASKCONFIG_H
