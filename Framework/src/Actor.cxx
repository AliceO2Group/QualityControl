//
// Created by pkonopka on 24/11/2025.
//


#include "QualityControl/Actor.h"

#include "QualityControl/Bookkeeping.h"
#include <Monitoring/Monitoring.h>
#include <Monitoring/MonitoringFactory.h>


namespace o2::quality_control::core
{


namespace impl
{
std::shared_ptr<monitoring::Monitoring> initMonitoring(std::string_view url, std::string_view detector)
{
  auto monitoring = monitoring::MonitoringFactory::Get(std::string{url});
  monitoring->addGlobalTag(monitoring::tags::Key::Subsystem, monitoring::tags::Value::QC);
  // mMonitoring->addGlobalTag("TaskName", mTaskConfig.taskName);
  if (!detector.empty()) {
    monitoring->addGlobalTag("DetectorName", detector);
  }

  return std::move(monitoring);
}

void startMonitoring(monitoring::Monitoring& monitoring, int runNumber)
{
  monitoring.setRunNumber(runNumber);
}

void initBookkeeping(std::string_view url)
{
  Bookkeeping::getInstance().init(url.data());
}

void startBookkeeping(int runNumber, std::string_view actorName, std::string_view detectorName, const o2::bkp::DplProcessType& processType, std::string_view args)
{
  Bookkeeping::getInstance().registerProcess(runNumber, actorName.data(), detectorName.data(), processType, args.data());
}

Bookkeeping& getBookkeeping()
{
  return Bookkeeping::getInstance();
}

}

}
