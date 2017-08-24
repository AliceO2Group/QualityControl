///
/// \file   TaskDevice.h
/// \author Barthelemy von Haller
///

#ifndef QUALITYCONTROL_TASKDEVICE_H
#define QUALITYCONTROL_TASKDEVICE_H

// fairroot
#include <fairmq/FairMQDevice.h>
// boost (should be first but then it makes errors in fairmq)
#include <boost/serialization/array_wrapper.hpp>
#include <boost/accumulators/accumulators.hpp>
#include <boost/accumulators/statistics.hpp>
// O2
#include <Common/Timer.h>
#include <Configuration/Configuration.h>
#include <Configuration/ConfigurationInterface.h>
#include <Monitoring/Collector.h>
#include <DataSampling/SamplerInterface.h>
// QC
#include "QualityControl/TaskConfig.h"
#include "QualityControl/ObjectsManager.h"

namespace ba = boost::accumulators;

namespace AliceO2 {
namespace QualityControl {
namespace Core {

class TaskInterface;

/// \brief The device driving the execution of a QC task.
///
/// TaskDevice drives the execution of the task by implementing the Control interface (not yet!).
/// It is responsible for retrieving details about the task via the Configuration system and
/// instantiating the DataSampler and the Task (indirectly). It then steers the execution
/// of the task and provides it with the DataBlocks coming from the data sampler. It finally
/// publishes the MonitorObjects owned and filled by the QC task and managed by the ObjectsManager.
///
/// \author Barthelemy von Haller
class TaskDevice : public FairMQDevice
{
  public:
    TaskDevice(std::string taskName, std::string configurationSource);
    virtual ~TaskDevice();

  protected:
    virtual void InitTask() override;
    virtual void Run() override;
    virtual void Reset() override;

  private:
    void populateConfig(std::string taskName);
    void startOfActivity();
    void endOfActivity();
    void monitorCycle();
    unsigned long publish();
    static void CustomCleanupTMessage(void *data, void *object);

  private:
    std::string mTaskName;
    TaskConfig mTaskConfig;
    std::unique_ptr<AliceO2::Configuration::ConfigurationInterface> mConfigFile;
    std::unique_ptr<AliceO2::Monitoring::Collector> mCollector;
    std::unique_ptr<AliceO2::DataSampling::SamplerInterface> mSampler;
    AliceO2::QualityControl::Core::TaskInterface *mTask;
    std::shared_ptr<ObjectsManager> mObjectsManager;

    // stats
    int mTotalNumberObjectsPublished;
    AliceO2::Common::Timer timerTotalDurationActivity;
    ba::accumulator_set<double, ba::features<ba::tag::mean, ba::tag::variance>> pcpus;
    ba::accumulator_set<double, ba::features<ba::tag::mean, ba::tag::variance>> pmems;
};

}
}
}

#endif //QUALITYCONTROL_TASKDEVICE_H
