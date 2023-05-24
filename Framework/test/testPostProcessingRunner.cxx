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
/// \file    testPostProcessingRunner.cxx
/// \author  Piotr Konopka
///

#include "getTestDataDirectory.h"
#include "QualityControl/PostProcessingRunner.h"
#include <Configuration/ConfigurationFactory.h>
#include <catch_amalgamated.hpp>

using namespace o2::quality_control::postprocessing;
using namespace o2::configuration;

TEST_CASE("test_configurationfactory")
{
  std::string configFilePath = std::string("json://") + getTestDataDirectory() + "testSharedConfig.json";
  auto config = ConfigurationFactory::getConfiguration(configFilePath);

  PostProcessingRunner runner("SkeletonPostProcessing");

  // todo: this initializes database. should we have an option not to do it, so we don't fail test randomly?
  CHECK_NOTHROW(runner.init(config->getRecursive()));
  CHECK_NOTHROW(runner.run());
}
