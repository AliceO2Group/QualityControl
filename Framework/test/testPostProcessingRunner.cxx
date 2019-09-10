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
/// \file    testPostProcessingRunner.cxx
/// \author  Piotr Konopka
///

//#include "getTestDataDirectory.h"
#include "QualityControl/PostProcessingRunner.h"

#define BOOST_TEST_MODULE PostProcessingRunner test
#define BOOST_TEST_MAIN
#define BOOST_TEST_DYN_LINK

#include <boost/test/unit_test.hpp>

using namespace o2::quality_control::postprocessing;

BOOST_AUTO_TEST_CASE(test_factory)
{
//  std::string configFilePath = std::string("json://") + getTestDataDirectory() + "testSharedConfig.json";


  PostProcessingRunner postProcessingRunner("", "");
}