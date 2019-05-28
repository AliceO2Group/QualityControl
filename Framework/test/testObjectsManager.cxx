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
/// \file   Publisher_test.cpp
/// \author Barthelemy von Haller
///

#include "QualityControl/ObjectsManager.h"

#define BOOST_TEST_MODULE Publisher test
#define BOOST_TEST_MAIN
#define BOOST_TEST_DYN_LINK
#include "../include/Common/Exceptions.h"
#include <TObjString.h>
#include <boost/test/unit_test.hpp>
#include <iostream>

using namespace std;
using namespace o2::quality_control::core;
using namespace AliceO2::Common;

namespace o2::quality_control::core
{

BOOST_AUTO_TEST_CASE(duplicate_object_test)
{
  TaskConfig config;
  config.taskName = "test";
  ObjectsManager objectsManager(config);
  TObjString s("content");
  objectsManager.startPublishing(&s);
  BOOST_CHECK_THROW(objectsManager.startPublishing(&s), o2::quality_control::core::DuplicateObjectError);
}

BOOST_AUTO_TEST_CASE(unpublish_test)
{
  TaskConfig config;
  config.taskName = "test";
  ObjectsManager objectsManager(config);
  TObjString s("content");
  objectsManager.startPublishing(&s);
  BOOST_CHECK_EQUAL(objectsManager.getNumberPublishedObjects(), 1);
  objectsManager.stopPublishing(&s);
  BOOST_CHECK_EQUAL(objectsManager.getNumberPublishedObjects(), 0);
  objectsManager.startPublishing(&s);
  BOOST_CHECK_EQUAL(objectsManager.getNumberPublishedObjects(), 1);
  objectsManager.stopPublishing("content");
  BOOST_CHECK_EQUAL(objectsManager.getNumberPublishedObjects(), 0);
  BOOST_CHECK_THROW(objectsManager.stopPublishing("content"), ObjectNotFoundError);
  BOOST_CHECK_THROW(objectsManager.stopPublishing("asdf"), ObjectNotFoundError);
}

} // namespace o2::quality_control::core
