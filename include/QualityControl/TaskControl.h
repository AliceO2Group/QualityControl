///
/// \file   TaskControl.h
/// \author Barthelemy von Haller
///

#ifndef QUALITY_CONTROL_TASKCONTROL_H
#define QUALITY_CONTROL_TASKCONTROL_H

#include <DataSampling/SamplerInterface.h>
#include "QualityControl/Publisher.h"
#include "QualityControl/TaskInterface.h"
#include "Configuration/Configuration.h"

namespace AliceO2 {
namespace QualityControl {
namespace Core {

/// \brief  TaskControl drives the execution of the task.
///
/// TaskControl drives the execution of the task by implementing the Control interface (not yet!).
/// It is responsible for retrieving details about the task via the Configuration system and
/// instantiating the DataSampler, the Publisher and the Task (indirectly). It then steers the execution
/// of the task and provides it with the DataBlocks coming from the data sampler.
///
/// \author Barthelemy von Haller
class TaskControl
{
  public:

    /// Constructor
    TaskControl(std::string taskName, std::string configurationSource);
    /// Destructor
    virtual ~TaskControl();

    void initialize();
    void configure();
    void start();
    void execute();
    void stop();

  private:
    Publisher *mPublisher;
    TaskInterface *mTask;
    ConfigFile mConfiguration;
    AliceO2::DataSampling::SamplerInterface *sampler;
};

}
}
}

#endif //QUALITY_CONTROL_TASKCONTROL_H
