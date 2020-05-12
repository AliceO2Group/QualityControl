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

#define BOOST_TEST_MODULE ObjectManager test
#define BOOST_TEST_MAIN
#define BOOST_TEST_DYN_LINK
#include <TObjString.h>
#include <TObjArray.h>
#include <TH1F.h>
#include <boost/test/unit_test.hpp>

using namespace std;
using namespace o2::quality_control::core;
using namespace AliceO2::Common;

namespace o2::quality_control::core
{

BOOST_AUTO_TEST_CASE(invalid_url_test)
{
  TaskConfig config;
  config.taskName = "test";
  config.consulUrl = "bad-url:1234";
  ObjectsManager objectsManager(config, true);
}

BOOST_AUTO_TEST_CASE(duplicate_object_test)
{
  TaskConfig config;
  config.taskName = "test";
  config.consulUrl = "http://consul-test.cern.ch:8500";
  ObjectsManager objectsManager(config, true);
  TObjString s("content");
  objectsManager.startPublishing(&s);
  BOOST_CHECK_THROW(objectsManager.startPublishing(&s), o2::quality_control::core::DuplicateObjectError);
}

BOOST_AUTO_TEST_CASE(is_being_published_test)
{
  TaskConfig config;
  config.taskName = "test";
  config.consulUrl = "http://consul-test.cern.ch:8500";
  ObjectsManager objectsManager(config, true);
  TObjString s("content");
  BOOST_CHECK(!objectsManager.isBeingPublished("content"));
  objectsManager.startPublishing(&s);
  BOOST_CHECK_THROW(objectsManager.startPublishing(&s), o2::quality_control::core::DuplicateObjectError);
  BOOST_CHECK(objectsManager.isBeingPublished("content"));
}

BOOST_AUTO_TEST_CASE(unpublish_test)
{
  TaskConfig config;
  config.taskName = "test";
  ObjectsManager objectsManager(config, true);
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

BOOST_AUTO_TEST_CASE(getters_test)
{
  TaskConfig config;
  config.taskName = "test";
  config.consulUrl = "http://consul-test.cern.ch:8500";
  ObjectsManager objectsManager(config, true);

  TObjString s("content");
  TH1F h("histo", "h", 100, 0, 99);

  objectsManager.startPublishing(&s);
  objectsManager.startPublishing(&h);

  // basic gets
  BOOST_CHECK_NO_THROW(objectsManager.getMonitorObject("content"));
  BOOST_CHECK_NO_THROW(objectsManager.getMonitorObject("histo"));
  BOOST_CHECK_THROW(objectsManager.getMonitorObject("unexisting object"), ObjectNotFoundError);
  BOOST_CHECK_NO_THROW(objectsManager.getObject("content"));
  BOOST_CHECK_NO_THROW(objectsManager.getObject("histo"));
  BOOST_CHECK_THROW(objectsManager.getObject("unexisting object"), ObjectNotFoundError);

  // non owning array
  TObjArray* array = objectsManager.getNonOwningArray();
  BOOST_CHECK_EQUAL(array->GetEntries(), 2);
  BOOST_CHECK(array->FindObject("content") != nullptr);
  BOOST_CHECK(array->FindObject("histo") != nullptr);

  // we confirm that deleting the array does not delete objects
  delete array;
  BOOST_CHECK_NO_THROW(objectsManager.getMonitorObject("content"));
  BOOST_CHECK_NO_THROW(objectsManager.getMonitorObject("histo"));
}

BOOST_AUTO_TEST_CASE(metadata_test)
{
  TaskConfig config;
  config.taskName = "test";
  config.consulUrl = "http://consul-test.cern.ch:8500";
  ObjectsManager objectsManager(config, true);

  TObjString s("content");
  TH1F h("histo", "h", 100, 0, 99);
  objectsManager.startPublishing(&s);
  objectsManager.startPublishing(&h);

  objectsManager.addMetadata("content", "aaa", "bbb");
  BOOST_CHECK_EQUAL(objectsManager.getMonitorObject("content")->getMetadataMap().at("aaa"), "bbb");
}

} // namespace o2::quality_control::core
