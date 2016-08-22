///
/// \file   TaskConfig.h
/// \author Barthelemy von Haller
///

#ifndef QUALITYCONTROL_CORE_TASKCONFIG_H_
#define QUALITYCONTROL_CORE_TASKCONFIG_H_

#include <string>

namespace AliceO2 {
namespace QualityControl {
namespace Core {

/// \brief  Container for the configuration of a Task
///
/// \author Barthelemy von Haller
struct TaskConfig
{
    std::string taskName;
    std::string moduleName;
    std::string className;
    std::string address;
    int numberHistos;
    int numberChecks;
    std::string typeOfChecks;
    int cycleDurationSeconds;
    std::string publisherClassName;
};

} // namespace Core
} // namespace QualityControl
} // namespace AliceO2

#endif // QUALITYCONTROL_CORE_TASKCONFIG_H_
