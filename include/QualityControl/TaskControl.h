///
/// \file   TaskControl.h
/// \author Barthelemy von Haller
///

#ifndef QUALITY_CONTROL_TASKCONTROL_H
#define QUALITY_CONTROL_TASKCONTROL_H

#include <DataSampling/SamplerInterface.h>
#include "QualityControl/ObjectsManager.h"
#include "QualityControl/TaskInterface.h"
#include "Configuration/Configuration.h"
#include "Monitoring/Collector.h"
#include "QualityControl/TaskConfig.h"
#include "Monitoring/ProcessMonitor.h"
#include "Common/Timer.h"
#include <boost/accumulators/accumulators.hpp>
#include <boost/accumulators/statistics.hpp>

namespace ba = boost::accumulators;

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
    /// This corresponds to a cycle here.
    void execute();
    void stop();

    int getTotalNumberObjectsPublished(){return mTotalNumberObjectsPublished;}

  private:
    void populateConfig(std::string taskName);

    ObjectsManager *mObjectsManager;
    TaskInterface *mTask;
    ConfigFile mConfigFile;
    TaskConfig mTaskConfig;
    AliceO2::DataSampling::SamplerInterface *mSampler;
    std::shared_ptr<AliceO2::Monitoring::Core::Collector> mCollector;
    std::unique_ptr<Monitoring::Core::ProcessMonitor> mMonitor;

    // stats
    int mTotalNumberObjectsPublished;
    AliceO2::Common::Timer timerTotalDurationActivity;
    ba::accumulator_set<double, ba::features<ba::tag::mean, ba::tag::variance>> pcpus;
    ba::accumulator_set<double, ba::features<ba::tag::mean, ba::tag::variance>> pmems;
};

}
}
}

#endif //QUALITY_CONTROL_TASKCONTROL_H
