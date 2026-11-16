// Copyright 2019-2020 CERN and copyright holders of ALICE O2.
// See https://alice-o2.web.cern.ch/copyright for details of the copyright holders.
// All rights not expressly granted are reserved.
//
// This software is distributed under the terms of the GNU General Public
// License v3 (GPL Version 3), copied verbatim in the file "COPYING".
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.

///
/// \file   testQcSkeleton.cxx
/// \author Barthelemy von Haller
/// \author Piotr Konopka
///

#include "Skeleton/SkeletonTask.h"

//#include "Framework/InitContext.h"
#include "QualityControl/TaskFactory.h"
#include <catch_amalgamated.hpp>

using namespace std;

namespace o2::quality_control_modules::skeleton
{

TEST_CASE("instantiate_task")
{
  SkeletonTask task;
  TaskRunnerConfig config;
  config.consulUrl = "";
  config.name = "qcSkeletonTest";
  config.detectorName = "TST";
  auto manager = make_shared<ObjectsManager>(config.name, "SkeletonTask", config.detectorName, 0);
  task.setObjectsManager(manager);
  //  o2::framework::InitContext ctx;
  //  task.initialize(ctx);

  //  CHECK(manager->getMonitorObject("example")->getObject() != nullptr);
}

} // namespace o2::quality_control_modules::skeleton
