///
/// \file   CheckerConfig.h
/// \author Barthelemy von Haller
///

#ifndef QC_CORE_CHECKERCONFIG_H
#define QC_CORE_CHECKERCONFIG_H

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
} // namespace quality_control
} // namespace o2

#endif // QC_CORE_CHECKERCONFIG_H
