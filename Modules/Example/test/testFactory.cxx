///
/// \file   testNonEmpty.cxx
/// \author Barthelemy von Haller
///

#include "../include/Example/ExampleTask.h"
#include "QualityControl/TaskFactory.h"
#include "QualityControl/ObjectsManager.h"
#include <boost/exception/diagnostic_information.hpp>
#include <TSystem.h>
#include "Common/Exceptions.h"

#define BOOST_TEST_MODULE Publisher test
#define BOOST_TEST_MAIN
#define BOOST_TEST_DYN_LINK

#include <boost/test/unit_test.hpp>
#include <cassert>

#include <TH1.h>

//using namespace AliceO2::Common;
using namespace std;

namespace AliceO2 {
namespace QualityControlModules {
namespace Common {

BOOST_AUTO_TEST_CASE(Task_Factory)
{
  TaskFactory factory;
  TaskConfig config;
  config.taskName = "task";
  config.moduleName = "QcCommon";
  config.className = "AliceO2::QualityControlModules::Example::ExampleTask";
  auto manager = make_shared<ObjectsManager>(config);
  try {
    gSystem->AddDynamicPath("lib:../../lib:../../../lib:.:"); // add local paths for the test
    factory.create(config, manager);
  } catch (...) {
    BOOST_TEST_FAIL(boost::current_exception_diagnostic_information());
  }
}

bool is_critical(AliceO2::Common::FatalException const &ex)
{
  return true;
}

BOOST_AUTO_TEST_CASE(Task_Factory_failures)
{
  TaskFactory factory;
  TaskConfig config;
  auto manager = make_shared<ObjectsManager>(config);

  config.taskName = "task";
  config.moduleName = "WRONGNAME";
  config.className = "AliceO2::QualityControlModules::Example::ExampleTask";
  BOOST_CHECK_EXCEPTION(
    factory.create(config, manager),
    AliceO2::Common::FatalException, is_critical);

  std::string addition = "lib:../../lib:../../../lib:";
  gSystem->Setenv("LD_LIBRARY_PATH", (addition + gSystem->Getenv("LD_LIBRARY_PATH")).c_str());
  config.taskName = "task";
  config.moduleName = "QcCommon";
  config.className = "WRONGCLASS";
  BOOST_CHECK_EXCEPTION(factory.create(config, manager), AliceO2::Common::FatalException, is_critical);

}

} // namespace Checker
} // namespace QualityControl
} // namespace AliceO2
