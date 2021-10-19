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

#define BOOST_TEST_MODULE PostProcessingConfig test
#define BOOST_TEST_MAIN
#define BOOST_TEST_DYN_LINK

#include <boost/test/unit_test.hpp>
using namespace o2::quality_control::postprocessing;
using namespace o2::configuration;

BOOST_AUTO_TEST_CASE(test_configuration_read)
{
  std::string configFilePath = std::string("json://") + getTestDataDirectory() + "testSharedConfig.json";

  auto configFile = ConfigurationFactory::getConfiguration(configFilePath);
  PostProcessingConfig ppconfig("SkeletonPostProcessing", configFile->getRecursive());

  BOOST_CHECK_EQUAL(ppconfig.taskName, "SkeletonPostProcessing");
  BOOST_CHECK_EQUAL(ppconfig.detectorName, "TST");
  BOOST_CHECK_EQUAL(ppconfig.moduleName, "QcSkeleton");
  BOOST_CHECK_EQUAL(ppconfig.className, "o2::quality_control_modules::skeleton::SkeletonPostProcessing");

  BOOST_REQUIRE_EQUAL(ppconfig.initTriggers.size(), 3);
  BOOST_CHECK_EQUAL(ppconfig.initTriggers[0], "SOR");
  BOOST_CHECK_EQUAL(ppconfig.initTriggers[1], "EOR");
  BOOST_CHECK_EQUAL(ppconfig.initTriggers[2], "once");

  BOOST_REQUIRE_EQUAL(ppconfig.updateTriggers.size(), 1);
  BOOST_CHECK_EQUAL(ppconfig.updateTriggers[0], "once");

  BOOST_REQUIRE_EQUAL(ppconfig.stopTriggers.size(), 1);
  BOOST_CHECK_EQUAL(ppconfig.stopTriggers[0], "once");
}