///
/// \file   testNonEmpty.cxx
/// \author Barthelemy von Haller
///

#include "QualityControl/ObjectsManager.h"
#include "QualityControl/TaskFactory.h"
#include <Common/Exceptions.h>
#include <TSystem.h>
#include <boost/exception/diagnostic_information.hpp>

#include <catch_amalgamated.hpp>

using namespace std;
using namespace o2::quality_control::core;

namespace o2::quality_control_modules::example
{

TEST_CASE("Task_Factory")
{
  TaskFactory factory;
  TaskRunnerConfig config;
  config.name = "task";
  config.moduleName = "QcCommon";
  config.className = "o2::quality_control_modules::example::ExampleTask";
  config.detectorName = "DAQ";
  config.ccdbUrl = "something";
  auto manager = make_shared<ObjectsManager>(config.name, config.className, config.detectorName, 0);
  try {
    gSystem->AddDynamicPath("lib:../../lib:../../../lib:.:"); // add local  paths for the test
    factory.create(config, manager);
  } catch (...) {
    FAIL(boost::current_exception_diagnostic_information());
  }
}

// Catch2 runs the test cases sequentially in the order of declaration, so it is never run at the same time as "Task_Factory"
TEST_CASE("Task_Factory_failures")
{
  TaskFactory factory;
  TaskRunnerConfig config;
  auto manager = make_shared<ObjectsManager>(config.name, config.className, config.detectorName, 0);

  config.name = "task";
  config.moduleName = "WRONGNAME";
  config.className = "o2::quality_control_modules::example::ExampleTask";
  CHECK_THROWS_AS(factory.create(config, manager), AliceO2::Common::FatalException);

  gSystem->AddDynamicPath("lib:../../lib:../../../lib:"); // add local paths for the test
  config.name = "task";
  config.moduleName = "QcCommon";
  config.className = "WRONGCLASS";
  CHECK_THROWS_AS(factory.create(config, manager), AliceO2::Common::FatalException);
}

} // namespace o2::quality_control_modules::example
