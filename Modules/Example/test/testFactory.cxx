///
/// \file   testNonEmpty.cxx
/// \author Barthelemy von Haller
///

#include "QualityControl/ObjectsManager.h"
#include "QualityControl/TaskFactory.h"
#include <Common/Exceptions.h>
#include <TSystem.h>
#include <boost/exception/diagnostic_information.hpp>

#define BOOST_TEST_MODULE Publisher test
#define BOOST_TEST_MAIN
#define BOOST_TEST_DYN_LINK

#include <boost/test/unit_test.hpp>

using namespace std;
namespace utf = boost::unit_test;
using namespace o2::quality_control::core;

namespace o2::quality_control_modules::example
{

BOOST_AUTO_TEST_CASE(Task_Factory)
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
    BOOST_TEST_FAIL(boost::current_exception_diagnostic_information());
  }
}

bool is_critical(AliceO2::Common::FatalException const&) { return true; }

BOOST_AUTO_TEST_CASE(Task_Factory_failures, *utf::depends_on("Task_Factory") /* make sure we don't run both tests at the same time */)
{
  TaskFactory factory;
  TaskRunnerConfig config;
  auto manager = make_shared<ObjectsManager>(config.name, config.className, config.detectorName, 0);

  config.name = "task";
  config.moduleName = "WRONGNAME";
  config.className = "o2::quality_control_modules::example::ExampleTask";
  BOOST_CHECK_EXCEPTION(factory.create(config, manager), AliceO2::Common::FatalException, is_critical);

  gSystem->AddDynamicPath("lib:../../lib:../../../lib:"); // add local paths for the test
  config.name = "task";
  config.moduleName = "QcCommon";
  config.className = "WRONGCLASS";
  BOOST_CHECK_EXCEPTION(factory.create(config, manager), AliceO2::Common::FatalException, is_critical);
}

} // namespace o2::quality_control_modules::example
