///
/// \file   TaskFactory.h
/// \author Barthelemy von Haller
///

#ifndef QUALITYCONTROL_LIBS_CORE_TASKFACTORY_H_
#define QUALITYCONTROL_LIBS_CORE_TASKFACTORY_H_

#include <iostream>

namespace AliceO2 {
namespace QualityControl {
namespace Core {

class TaskInterface;
class Publisher;

class TaskFactory
{
  public:
    /// Default constructor
    TaskFactory();
    /// Destructor
    virtual ~TaskFactory();

    /// \brief Create a new instance of a TaskInterface.
    /// The TaskInterface actual class is decided based on the parameters passed.
    /// \author Barthelemy von Haller
    TaskInterface *create(std::string taskName, std::string moduleName, std::string className, Publisher* publisher);
};

} // namespace Core
} // namespace QualityControl
} // namespace AliceO2

#endif // QUALITYCONTROL_LIBS_CORE_TASKFACTORY_H_
