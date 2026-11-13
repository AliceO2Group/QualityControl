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
/// \file    testPostProcessingConfig.cxx
/// \author  Piotr Konopka
///

#include "getTestDataDirectory.h"
#include "QualityControl/PostProcessingConfig.h"
#include "Configuration/ConfigurationFactory.h"
#include <catch_amalgamated.hpp>

using namespace o2::quality_control::postprocessing;
using namespace o2::configuration;

TEST_CASE("test_configuration_read")
{
  std::string configFilePath = std::string("json://") + getTestDataDirectory() + "testSharedConfig.json";

  auto configFile = ConfigurationFactory::getConfiguration(configFilePath);
  PostProcessingConfig ppconfig("SkeletonPostProcessing", configFile->getRecursive());

  CHECK(ppconfig.taskName == "SkeletonPostProcessing");
  CHECK(ppconfig.detectorName == "TST");
  CHECK(ppconfig.moduleName == "QcSkeleton");
  CHECK(ppconfig.className == "o2::quality_control_modules::skeleton::SkeletonPostProcessing");

  REQUIRE(ppconfig.initTriggers.size() == 3);
  CHECK(ppconfig.initTriggers[0] == "SOR");
  CHECK(ppconfig.initTriggers[1] == "EOR");
  CHECK(ppconfig.initTriggers[2] == "once");

  REQUIRE(ppconfig.updateTriggers.size() == 1);
  CHECK(ppconfig.updateTriggers[0] == "once");

  REQUIRE(ppconfig.stopTriggers.size() == 1);
  CHECK(ppconfig.stopTriggers[0] == "once");

  CHECK(ppconfig.customParameters.size() == 4);

  CHECK(ppconfig.ccdbUrl == "localhost:8083");
}