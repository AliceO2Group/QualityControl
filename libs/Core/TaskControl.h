///
/// \file   TaskControl.h
/// \author Barthelemy von Haller
///

#ifndef QUALITY_CONTROL_TASKCONTROL_H
#define QUALITY_CONTROL_TASKCONTROL_H

#include <memory>
#include "Core/Publisher.h"
#include "QCTask.h"

namespace AliceO2 {
namespace QualityControl {
namespace Core {

class TaskControl
{

  public:

    TaskControl();
    virtual ~TaskControl();

    void initialize();
    void configure();
    void start();
    void execute();
    void stop();

  private:
    Publisher* mPublisher;
    QCTask* mTask;
};

}
}
}

#endif //QUALITY_CONTROL_TASKCONTROL_H
