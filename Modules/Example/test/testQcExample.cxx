///
/// \file   testQcExample.cxx
/// \author Barthelemy von Haller
///

#include "Example/ExampleTask.h"
#include "QualityControl/TaskFactory.h"
#include <TSystem.h>

#define BOOST_TEST_MODULE QcExample test
#define BOOST_TEST_MAIN
#define BOOST_TEST_DYN_LINK

#include <boost/test/unit_test.hpp>

#include <TH1.h>

using namespace std;

namespace o2::quality_control_modules::example
{

BOOST_AUTO_TEST_CASE(insantiate_task)
{
  ExampleTask task;
  TaskRunnerConfig config;
  config.consulUrl = "";
  config.name = "qcExampleTest";
  config.detectorName = "TST";
  auto manager = make_shared<ObjectsManager>(config.name, "ExampleTask", config.detectorName, 0);
  task.setObjectsManager(manager);
  //  task.initialize();// TODO

  //  BOOST_CHECK(task.getHisto1() != nullptr);
  //  BOOST_CHECK(task.getHisto2() != nullptr);

  Activity activity;
  task.startOfActivity(activity);
  //  task.startOfCycle();
  //  auto producer = AliceO2::DataSampling::DataBlockProducer(false, 1024);
  //  DataSetReference dataSet = producer.getDataSet();
  //  task.monitorDataBlock(dataSet);// TODO

  //  BOOST_CHECK(task.getHisto1()->GetEntries() > 0);

  //  task.endOfCycle();
  task.endOfActivity(activity);
  task.startOfActivity(activity);

  //  BOOST_CHECK(task.getHisto1()->GetEntries() == 0);

  task.endOfActivity(activity);
}

} // namespace o2::quality_control_modules::example
