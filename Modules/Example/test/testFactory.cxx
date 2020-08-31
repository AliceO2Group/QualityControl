///
/// \file   testNonEmpty.cxx
/// \author Barthelemy von Haller
///

#include "Example/ExampleTask.h"
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

namespace o2::quality_control_modules::example
{

BOOST_AUTO_TEST_CASE(Task_Factory)
{
  TaskFactory factory;
  TaskConfig config;
  config.taskName = "task";
  config.moduleName = "QcCommon";
  config.className = "o2::quality_control_modules::example::ExampleTask";
  auto manager = make_shared<ObjectsManager>(config);
  try {
    gSystem->AddDynamicPath("lib:../../lib:../../../lib:.:"); // add local paths for the test
    factory.create(config, manager);
  } catch (...) {
    BOOST_TEST_FAIL(boost::current_exception_diagnostic_information());
  }
}

bool is_critical(AliceO2::Common::FatalException const&) { return true; }

BOOST_AUTO_TEST_CASE(Task_Factory_failures)
{
  TaskFactory factory;
  TaskConfig config;
  auto manager = make_shared<ObjectsManager>(config);

  config.taskName = "task";
  config.moduleName = "WRONGNAME";
  config.className = "o2::quality_control_modules::example::ExampleTask";
  BOOST_CHECK_EXCEPTION(factory.create(config, manager), AliceO2::Common::FatalException, is_critical);

  std::string addition = "lib:../../lib:../../../lib:";
  const char *ldPath = gSystem->Getenv("LD_LIBRARY_PATH");
  string ldPathString = ldPath == nullptr ? "" : ldPath;
  gSystem->Setenv("LD_LIBRARY_PATH", (addition + ldPathString).c_str());
  config.taskName = "task";
  config.moduleName = "QcCommon";
  config.className = "WRONGCLASS";
  BOOST_CHECK_EXCEPTION(factory.create(config, manager), AliceO2::Common::FatalException, is_critical);
}

} // namespace o2::quality_control_modules::example
