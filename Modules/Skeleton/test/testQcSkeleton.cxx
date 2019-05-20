// Copyright CERN and copyright holders of ALICE O2. This software is
// distributed under the terms of the GNU General Public License v3 (GPL
// Version 3), copied verbatim in the file "COPYING".
//
// See http://alice-o2.web.cern.ch/license for full licensing information.
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.

///
/// \file   testQcSkeleton.cxx
/// \author Barthelemy von Haller
/// \author Piotr Konopka
///

#include "Framework/InitContext.h"
#include "QualityControl/TaskFactory.h"
#include "Skeleton/SkeletonTask.h"
#include <TSystem.h>

#define BOOST_TEST_MODULE Publisher test
#define BOOST_TEST_MAIN
#define BOOST_TEST_DYN_LINK

#include <boost/test/unit_test.hpp>

#include <TH1.h>

using namespace std;

namespace o2::quality_control_modules::skeleton
{

BOOST_AUTO_TEST_CASE(instantiate_task)
{
  SkeletonTask task;
  TaskConfig config;
  auto manager = make_shared<ObjectsManager>(config);
  task.setObjectsManager(manager);
  //  o2::framework::InitContext ctx;
  //  task.initialize(ctx);

  //  BOOST_CHECK(manager->getMonitorObject("example")->getObject() != nullptr);
}

} // namespace o2::quality_control_modules::skeleton
