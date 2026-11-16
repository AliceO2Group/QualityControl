///
/// \file   testQcExample.cxx
/// \author Barthelemy von Haller
///

#include "Example/ExampleTask.h"
#include "QualityControl/TaskFactory.h"
#include <TSystem.h>

#include <TH1.h>
#include <catch_amalgamated.hpp>

using namespace std;

namespace o2::quality_control_modules::example
{

TEST_CASE("insantiate_task")
{
  ExampleTask task;
  TaskRunnerConfig config;
  config.consulUrl = "";
  config.name = "qcExampleTest";
  config.detectorName = "TST";
  auto manager = make_shared<ObjectsManager>(config.name, "ExampleTask", config.detectorName, 0);
  task.setObjectsManager(manager);
  //  task.initialize();// TODO

  //  CHECK(task.getHisto1() != nullptr);
  //  CHECK(task.getHisto2() != nullptr);

  Activity activity;
  task.startOfActivity(activity);
  //  task.startOfCycle();
  //  auto producer = AliceO2::DataSampling::DataBlockProducer(false, 1024);
  //  DataSetReference dataSet = producer.getDataSet();
  //  task.monitorDataBlock(dataSet);// TODO

  //  CHECK(task.getHisto1()->GetEntries() > 0);

  //  task.endOfCycle();
  task.endOfActivity(activity);
  task.startOfActivity(activity);

  //  CHECK(task.getHisto1()->GetEntries() == 0);

  task.endOfActivity(activity);
}

} // namespace o2::quality_control_modules::example
