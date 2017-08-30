///
/// \file   CheckerConfig.h
/// \author Barthelemy von Haller
///

#ifndef QUALITYCONTROL_CORE_CHECKERCONFIG_H_
#define QUALITYCONTROL_CORE_CHECKERCONFIG_H_

#include <string>

namespace o2 {
namespace quality_control {
namespace core {

/// \brief  Container for the configuration of a Task
///
/// \author Barthelemy von Haller
struct CheckerConfig
{
    // Specific to this checker
    std::string checkerName;
    bool broadcast;
    std::string broadcastAddress;
    int id;
    // Shared amongst checkers
    int numberCheckers;
    int numberTasks;
    std::string tasksAddresses;
};

} // namespace core
} // namespace QualityControl
} // namespace o2

#endif // QUALITYCONTROL_CORE_CHECKERCONFIG_H_
