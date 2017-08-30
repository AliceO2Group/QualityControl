///
/// \file   TaskConfig.h
/// \author Barthelemy von Haller
///

#ifndef QUALITYCONTROL_CORE_TASKCONFIG_H_
#define QUALITYCONTROL_CORE_TASKCONFIG_H_

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
} // namespace QualityControl
} // namespace o2

#endif // QUALITYCONTROL_CORE_TASKCONFIG_H_
