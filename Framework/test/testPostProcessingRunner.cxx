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

#define BOOST_TEST_MODULE PostProcessingRunner test
#define BOOST_TEST_MAIN
#define BOOST_TEST_DYN_LINK

#include <boost/test/unit_test.hpp>

using namespace o2::quality_control::postprocessing;
using namespace o2::configuration;

BOOST_AUTO_TEST_CASE(test_factory)
{
  std::string configFilePath = std::string("json://") + getTestDataDirectory() + "testSharedConfig.json";
  auto config = ConfigurationFactory::getConfiguration(configFilePath);

  PostProcessingRunner runner("SkeletonPostProcessing");

  // todo: this initializes database. should we have an option not to do it, so we don't fail test randomly?
  BOOST_CHECK_NO_THROW(runner.init(config->getRecursive()));
  BOOST_CHECK_NO_THROW(runner.run());
}