///
/// \file   TaskLauncher.h
/// \author Barthelemy von Haller
///

#ifndef QUALITYCONTROL_LIBS_PUBLISHER_TASKLAUNCHER_H_
#define QUALITYCONTROL_LIBS_PUBLISHER_TASKLAUNCHER_H_

namespace AliceO2 {
namespace QualityControl {
namespace Publisher {

class TaskLauncher
{
  public:
    /// Default constructor
    TaskLauncher();
    /// Destructor
    virtual ~TaskLauncher();

    void configure();
};

} /* namespace Publisher */
} /* namespace QualityControl */
} /* namespace AliceO2 */

#endif /* QUALITYCONTROL_LIBS_PUBLISHER_TASKLAUNCHER_H_ */
