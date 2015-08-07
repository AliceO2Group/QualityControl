///
/// \file   TaskControl.h
/// \author Barthelemy von Haller
///

#ifndef QUALITY_CONTROL_TASKCONTROL_H
#define QUALITY_CONTROL_TASKCONTROL_H

#include <memory>
#include <DataSampling/Sampler.h>
#include "Core/Publisher.h"
#include "TaskInterface.h"
#include "Configuration.h"

namespace AliceO2 {
namespace QualityControl {
namespace Core {

class TaskControl
{

  public:

    TaskControl(std::string taskName, std::string configurationSource);
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
    AliceO2::DataSampling::Sampler *sampler;
};

}
}
}

#endif //QUALITY_CONTROL_TASKCONTROL_H
