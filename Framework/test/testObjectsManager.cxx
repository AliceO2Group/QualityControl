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
/// \file   Publisher_test.cpp
/// \author Barthelemy von Haller
///

#include "QualityControl/ObjectsManager.h"

#include <TObjString.h>
#include <TObjArray.h>
#include <TH1F.h>
#include <catch_amalgamated.hpp>

using namespace std;
using namespace o2::quality_control::core;
using namespace AliceO2::Common;

namespace o2::quality_control::core
{

// anonymous namespace to keep these symbols visible only to this file (instead of all files combined into o2-qc-test-core)
namespace
{
struct Config {
  std::string taskName = "test";
  std::string detectorName = "TST";
  std::string consulUrl = "invalid";
  std::string taskClass = "TestClass";
};
} // namespace

TEST_CASE("invalid_url_test")
{
  Config config;
  config.taskName = "test";
  config.consulUrl = "bad-url:1234";
  ObjectsManager objectsManager(config.taskName, config.taskClass, config.detectorName, 0);
}

TEST_CASE("duplicate_object_test")
{
  Config config;
  config.taskName = "test";
  config.consulUrl = "";
  ObjectsManager objectsManager(config.taskName, config.taskClass, config.detectorName, 0);
  TObjString s("content");
  objectsManager.startPublishing<true>(&s, PublicationPolicy::Forever);
  CHECK_NOTHROW(objectsManager.startPublishing<true>(&s, PublicationPolicy::Forever));
  REQUIRE(objectsManager.getMonitorObject("content") != nullptr);

  TObjString s2("content");
  CHECK_NOTHROW(objectsManager.startPublishing<true>(&s2, PublicationPolicy::Forever));
  auto mo2 = objectsManager.getMonitorObject("content");
  REQUIRE(mo2 != nullptr);
  REQUIRE(mo2->getObject() != &s);
  REQUIRE(mo2->getObject() == &s2);
}

TEST_CASE("is_being_published_test")
{
  Config config;
  config.taskName = "test";
  config.consulUrl = "";
  ObjectsManager objectsManager(config.taskName, config.taskClass, config.detectorName, 0);
  TObjString s("content");
  CHECK(!objectsManager.isBeingPublished("content"));
  objectsManager.startPublishing<true>(&s, PublicationPolicy::Forever);
  CHECK_NOTHROW(objectsManager.startPublishing<true>(&s, PublicationPolicy::Forever));
  CHECK(objectsManager.isBeingPublished("content"));
}

TEST_CASE("unpublish_test")
{
  Config config;
  config.taskName = "test";
  ObjectsManager objectsManager(config.taskName, config.taskClass, config.detectorName, 0);
  TObjString s("content");
  objectsManager.startPublishing<true>(&s, PublicationPolicy::Forever);
  CHECK(objectsManager.getNumberPublishedObjects() == 1);
  objectsManager.stopPublishing(&s);
  CHECK(objectsManager.getNumberPublishedObjects() == 0);
  objectsManager.startPublishing<true>(&s, PublicationPolicy::Forever);
  CHECK(objectsManager.getNumberPublishedObjects() == 1);
  objectsManager.stopPublishing("content");
  CHECK(objectsManager.getNumberPublishedObjects() == 0);
  CHECK_THROWS_AS(objectsManager.stopPublishing("content"), ObjectNotFoundError);
  CHECK_THROWS_AS(objectsManager.stopPublishing("asdf"), ObjectNotFoundError);

  // unpublish all
  objectsManager.startPublishing<true>(&s, PublicationPolicy::Forever);
  CHECK(objectsManager.getNumberPublishedObjects() == 1);
  objectsManager.stopPublishingAll();
  CHECK(objectsManager.getNumberPublishedObjects() == 0);
  CHECK_NOTHROW(objectsManager.stopPublishingAll());

  // unpublish after deletion
  auto s2 = new TObjString("content");
  objectsManager.startPublishing<true>(s2, PublicationPolicy::Forever);
  CHECK(objectsManager.getNumberPublishedObjects() == 1);
  delete s2;
  objectsManager.stopPublishing(s2);
  CHECK(objectsManager.getNumberPublishedObjects() == 0);

  // unpublish for publication policy
  auto s3 = new TObjString("content3");
  objectsManager.startPublishing<true>(s3, PublicationPolicy::Once);
  auto s4 = new TObjString("content4");
  objectsManager.startPublishing<true>(s4, PublicationPolicy::Once);
  auto s5 = new TObjString("content5");
  objectsManager.startPublishing<true>(s5, PublicationPolicy::ThroughStop);
  CHECK(objectsManager.getNumberPublishedObjects() == 3);
  objectsManager.stopPublishing(PublicationPolicy::Once);
  CHECK(objectsManager.getNumberPublishedObjects() == 1);
  objectsManager.stopPublishing(PublicationPolicy::ThroughStop);
  CHECK(objectsManager.getNumberPublishedObjects() == 0);

  objectsManager.startPublishing<true>(s3, PublicationPolicy::Once);
  objectsManager.stopPublishing(s3);
  CHECK(objectsManager.getNumberPublishedObjects() == 0);
  CHECK_NOTHROW(objectsManager.stopPublishing(PublicationPolicy::Once));
  CHECK(objectsManager.getNumberPublishedObjects() == 0);
  CHECK_NOTHROW(objectsManager.stopPublishing(s3));

  delete s3;
  delete s4;
  delete s5;
}

TEST_CASE("getters_test")
{
  Config config;
  config.taskName = "test";
  config.consulUrl = "";
  ObjectsManager objectsManager(config.taskName, config.taskClass, config.detectorName, 0);

  TObjString s("content");
  TH1F h("histo", "h", 100, 0, 99);

  objectsManager.startPublishing<true>(&s, PublicationPolicy::Forever);
  objectsManager.startPublishing<true>(&h, PublicationPolicy::Forever);

  // basic gets
  CHECK_NOTHROW(objectsManager.getMonitorObject("content"));
  CHECK_NOTHROW(objectsManager.getMonitorObject("histo"));
  CHECK_THROWS_AS(objectsManager.getMonitorObject("unexisting object"), ObjectNotFoundError);

  // non owning array
  TObjArray* array = objectsManager.getNonOwningArray();
  CHECK(array->GetEntries() == 2);
  CHECK(array->FindObject("content") != nullptr);
  CHECK(array->FindObject("histo") != nullptr);

  // we confirm that deleting the array does not delete objects
  delete array;
  CHECK_NOTHROW(objectsManager.getMonitorObject("content"));
  CHECK_NOTHROW(objectsManager.getMonitorObject("histo"));
}

TEST_CASE("metadata_test")
{
  Config config;
  config.taskName = "test";
  config.consulUrl = "";
  ObjectsManager objectsManager(config.taskName, config.taskClass, config.detectorName, 0);

  TObjString s("content");
  TH1F h("histo", "h", 100, 0, 99);
  objectsManager.startPublishing<true>(&s, PublicationPolicy::Forever);
  objectsManager.startPublishing<true>(&h, PublicationPolicy::Forever);

  objectsManager.addMetadata("content", "aaa", "bbb");
  CHECK(objectsManager.getMonitorObject("content")->getMetadataMap().at("aaa") == "bbb");
}

TEST_CASE("drawOptions_test")
{
  Config config;
  config.taskName = "test";
  config.consulUrl = "";
  ObjectsManager objectsManager(config.taskName, config.taskClass, config.detectorName, 0);

  TH1F h("histo", "h", 100, 0, 99);
  objectsManager.startPublishing(&h, PublicationPolicy::Forever);

  CHECK_THROWS_AS(objectsManager.getMonitorObject("histo")->getMetadataMap().at(ObjectsManager::gDrawOptionsKey), out_of_range);
  objectsManager.setDefaultDrawOptions(&h, "colz");
  CHECK(objectsManager.getMonitorObject("histo")->getMetadataMap().at(ObjectsManager::gDrawOptionsKey) == "colz");
  objectsManager.setDefaultDrawOptions("histo", "alp lego1");
  CHECK(objectsManager.getMonitorObject("histo")->getMetadataMap().at(ObjectsManager::gDrawOptionsKey) == "alp lego1");

  CHECK_THROWS_AS(objectsManager.getMonitorObject("histo")->getMetadataMap().at(ObjectsManager::gDisplayHintsKey), out_of_range);
  objectsManager.setDisplayHint(&h, "logx");
  CHECK(objectsManager.getMonitorObject("histo")->getMetadataMap().at(ObjectsManager::gDisplayHintsKey) == "logx");
  objectsManager.setDisplayHint("histo", "gridy logy");
  CHECK(objectsManager.getMonitorObject("histo")->getMetadataMap().at(ObjectsManager::gDisplayHintsKey) == "gridy logy");
}

TEST_CASE("feed_with_nullptr")
{
  Config config;
  config.taskName = "test";
  config.consulUrl = "";
  ObjectsManager objectsManager(config.taskName, config.taskClass, config.detectorName, 0);

  CHECK_NOTHROW(objectsManager.startPublishing<true>(nullptr, PublicationPolicy::Forever));
  CHECK_NOTHROW(objectsManager.setDefaultDrawOptions(nullptr, ""));
  CHECK_NOTHROW(objectsManager.setDisplayHint(nullptr, ""));
  CHECK_NOTHROW(objectsManager.stopPublishing(nullptr));
}

} // namespace o2::quality_control::core
