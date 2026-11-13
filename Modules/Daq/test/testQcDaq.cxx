///
/// \file   testQcDaq.cxx
/// \author Barthelemy von Haller
///

#include "Daq/DaqTask.h"
//#include "QualityControl/TaskFactory.h"
//#include <TSystem.h>

//#include <TH1.h>
#include <catch_amalgamated.hpp>

using namespace std;

namespace o2::quality_control_modules::daq
{

struct Config {
  std::string taskName = "test";
  std::string detectorName = "TST";
  std::string consulUrl = "invalid";
};

TEST_CASE("instantiate_task")
{
  DaqTask task;
  Config config;
  config.consulUrl = "";
  config.taskName = "qcDaqTest";
  config.detectorName = "DAQ";
  auto manager = make_shared<ObjectsManager>(config.taskName, "DaqTask", config.detectorName, 0);
  task.setObjectsManager(manager);
  //  o2::framework::InitContext ctx;
  //  task.initialize(ctx); // TODO

  //  CHECK(manager->getMonitorObject("payloadSize")->getObject() != nullptr);

  //  Activity activity;
  //  task.startOfActivity(activity);
  //  task.startOfCycle();
  //  auto producer = AliceO2::DataSampling::DataBlockProducer(false, 1024);
  //  DataSetReference dataSet = producer.getDataSet();
  //  task.monitorDataBlock(dataSet);// TODO

  //  TH1F* histo = (TH1F*)manager->getMonitorObject("payloadSize")->getObject();
  //  CHECK(histo->GetEntries() == 1);

  //  task.endOfCycle();
  //  task.endOfActivity(activity);
}

} // namespace o2::quality_control_modules::daq
