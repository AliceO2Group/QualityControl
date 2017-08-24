///
/// \file   testQcSkeleton.cxx
/// \author Barthelemy von Haller
///

#include "Skeleton/SkeletonTask.h"
#include "QualityControl/TaskFactory.h"
#include <TSystem.h>

#define BOOST_TEST_MODULE Publisher test
#define BOOST_TEST_MAIN
#define BOOST_TEST_DYN_LINK

#include <boost/test/unit_test.hpp>

#include <TH1.h>

using namespace std;

namespace AliceO2 {
namespace QualityControlModules {
namespace Skeleton {

BOOST_AUTO_TEST_CASE(instantiate_task)
{
  SkeletonTask task;
  TaskConfig config;
  auto manager = make_shared<ObjectsManager>(config);
  task.setObjectsManager(manager);
  task.initialize();

  BOOST_CHECK(manager->getMonitorObject("example")->getObject() != nullptr);
}

} // namespace Skeleton
} // namespace QualityControlModules
} // namespace AliceO2
