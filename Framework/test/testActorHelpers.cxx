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
/// \file    testActorHelpers.cxx
/// \author  Piotr Konopka
///

#include "QualityControl/ActorHelpers.h"
#include "QualityControl/CommonSpec.h"
#include <catch_amalgamated.hpp>

namespace o2::quality_control::core
{

using namespace actor_helpers;

struct ActorWithTwoServices;
template <>
struct ActorTraits<ActorWithTwoServices> {
  constexpr static std::array<ServiceRequest, 2> sRequiredServices{ ServiceRequest::Monitoring, ServiceRequest::CCDB };
  constexpr static UserCodeCardinality sUserCodeCardinality{ UserCodeCardinality::None };
};

TEST_CASE("requiresService")
{
  STATIC_CHECK(requiresService<ActorWithTwoServices, ServiceRequest::Monitoring>() == true);
  STATIC_CHECK(requiresService<ActorWithTwoServices, ServiceRequest::CCDB>() == true);
  STATIC_CHECK(requiresService<ActorWithTwoServices, ServiceRequest::InfoLogger>() == false);
  STATIC_CHECK(requiresService<ActorWithTwoServices, ServiceRequest::Bookkeeping>() == false);
}

struct ActorUserCodeNone;
template <>
struct ActorTraits<ActorUserCodeNone> {
  constexpr static UserCodeCardinality sUserCodeCardinality{ UserCodeCardinality::None };
};

struct ActorUserCodeOne;
template <>
struct ActorTraits<ActorUserCodeOne> {
  constexpr static UserCodeCardinality sUserCodeCardinality{ UserCodeCardinality::One };
};

struct ActorUserCodeMany;
template <>
struct ActorTraits<ActorUserCodeMany> {
  constexpr static UserCodeCardinality sUserCodeCardinality{ UserCodeCardinality::Many };
};

TEST_CASE("runsUserCode")
{
  STATIC_CHECK(runsUserCode<ActorUserCodeNone>() == false);
  STATIC_CHECK(runsUserCode<ActorUserCodeOne>() == true);
  STATIC_CHECK(runsUserCode<ActorUserCodeMany>() == true);
}

struct ActorPublishesTwoDataSources;
template <>
struct ActorTraits<ActorPublishesTwoDataSources> {
  constexpr static std::array<DataSourceType, 2> sPublishedDataSources{ DataSourceType::Task, DataSourceType::Check };
};

TEST_CASE("ValidDataSourceForActor")
{
  STATIC_CHECK(ValidDataSourceForActor<ActorPublishesTwoDataSources, DataSourceType::Task>);
  STATIC_CHECK(ValidDataSourceForActor<ActorPublishesTwoDataSources, DataSourceType::Check>);
  STATIC_CHECK(!ValidDataSourceForActor<ActorPublishesTwoDataSources, DataSourceType::Aggregator>);
  STATIC_CHECK(!ValidDataSourceForActor<ActorPublishesTwoDataSources, DataSourceType::Direct>);
}

TEST_CASE("extractConfig copies CommonSpec into ServicesConfig")
{
  CommonSpec spec;
  spec.database = { { "implementation", "ccdb" }, { "host", "example.invalid" } };

  spec.activityNumber = 42;
  spec.activityType = "PHYSICS";
  spec.activityPeriodName = "LHCxx";
  spec.activityPassName = "pass1";
  spec.activityProvenance = "qc";
  spec.activityStart = 1234;
  spec.activityEnd = 5678;
  spec.activityBeamType = "pp";
  spec.activityPartitionName = "physics_1";
  spec.activityFillNumber = 777;
  spec.activityOriginalNumber = 4242;

  spec.monitoringUrl = "infologger:///debug?qc_test";
  spec.conditionDBUrl = "http://ccdb.example.invalid:8080";

  spec.infologgerDiscardParameters.debug = false;
  spec.infologgerDiscardParameters.fromLevel = 10;
  spec.infologgerDiscardParameters.file = "/tmp/qc-discard.log";
  spec.infologgerDiscardParameters.rotateMaxBytes = 123456;
  spec.infologgerDiscardParameters.rotateMaxFiles = 7;
  spec.infologgerDiscardParameters.debugInDiscardFile = true;

  spec.bookkeepingUrl = "http://bookkeeping.example.invalid";
  spec.kafkaBrokersUrl = "broker1:9092,broker2:9092";
  spec.kafkaTopicAliECSRun = "aliecs.run.test";

  const auto cfg = extractConfig(spec);

  REQUIRE(cfg.database == spec.database);

  REQUIRE(cfg.activity.mId == spec.activityNumber);
  REQUIRE(cfg.activity.mType == spec.activityType);
  REQUIRE(cfg.activity.mPeriodName == spec.activityPeriodName);
  REQUIRE(cfg.activity.mPassName == spec.activityPassName);
  REQUIRE(cfg.activity.mProvenance == spec.activityProvenance);
  REQUIRE(cfg.activity.mValidity.getMin() == spec.activityStart);
  REQUIRE(cfg.activity.mValidity.getMax() == spec.activityEnd);
  REQUIRE(cfg.activity.mBeamType == spec.activityBeamType);
  REQUIRE(cfg.activity.mPartitionName == spec.activityPartitionName);
  REQUIRE(cfg.activity.mFillNumber == spec.activityFillNumber);
  REQUIRE(cfg.activity.mOriginalId == spec.activityOriginalNumber);

  REQUIRE(cfg.monitoringUrl == spec.monitoringUrl);
  REQUIRE(cfg.conditionDBUrl == spec.conditionDBUrl);

  REQUIRE(cfg.infologgerDiscardParameters.debug == spec.infologgerDiscardParameters.debug);
  REQUIRE(cfg.infologgerDiscardParameters.fromLevel == spec.infologgerDiscardParameters.fromLevel);
  REQUIRE(cfg.infologgerDiscardParameters.file == spec.infologgerDiscardParameters.file);
  REQUIRE(cfg.infologgerDiscardParameters.rotateMaxBytes == spec.infologgerDiscardParameters.rotateMaxBytes);
  REQUIRE(cfg.infologgerDiscardParameters.rotateMaxFiles == spec.infologgerDiscardParameters.rotateMaxFiles);
  REQUIRE(cfg.infologgerDiscardParameters.debugInDiscardFile == spec.infologgerDiscardParameters.debugInDiscardFile);

  REQUIRE(cfg.bookkeepingUrl == spec.bookkeepingUrl);
  REQUIRE(cfg.kafkaBrokersUrl == spec.kafkaBrokersUrl);
  REQUIRE(cfg.kafkaTopicAliECSRun == spec.kafkaTopicAliECSRun);
}

} // namespace o2::quality_control::core
