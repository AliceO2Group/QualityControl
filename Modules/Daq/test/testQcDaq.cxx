///
/// \file   testQcDaq.cxx
/// \author Barthelemy von Haller
///

#include "Daq/DaqTask.h"
//#include "QualityControl/TaskFactory.h"
//#include <TSystem.h>

#define BOOST_TEST_MODULE Publisher test
#define BOOST_TEST_MAIN
#define BOOST_TEST_DYN_LINK

//#include <TH1.h>
#include <boost/test/unit_test.hpp>

using namespace std;

namespace o2::quality_control_modules::daq
{

struct Config {
  std::string taskName = "test";
  std::string detectorName = "TST";
  std::string consulUrl = "invalid";
};

BOOST_AUTO_TEST_CASE(instantiate_task)
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

  //  BOOST_CHECK(manager->getMonitorObject("payloadSize")->getObject() != nullptr);

  //  Activity activity;
  //  task.startOfActivity(activity);
  //  task.startOfCycle();
  //  auto producer = AliceO2::DataSampling::DataBlockProducer(false, 1024);
  //  DataSetReference dataSet = producer.getDataSet();
  //  task.monitorDataBlock(dataSet);// TODO

  //  TH1F* histo = (TH1F*)manager->getMonitorObject("payloadSize")->getObject();
  //  BOOST_CHECK(histo->GetEntries() == 1);

  //  task.endOfCycle();
  //  task.endOfActivity(activity);
}

} // namespace o2::quality_control_modules::daq
