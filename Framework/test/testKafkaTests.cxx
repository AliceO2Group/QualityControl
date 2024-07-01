// Copyright 2024 CERN and copyright holders of ALICE O2.
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
/// \file    testKafkaTests.cxx
/// \author  Michal Tichak
/// \brief Tests for kafkaPoller class and StartOfRun and EndOfRun triggers
///
/// Detailed description:
/// We are testing SOR and EOR triggers here so we have all tests that require
/// Kafka in one place. Tests in this file can be run manually only right now,
/// as there is no kafka cluster setup as a part of CI/CD for QC.
/// Change `kafkaCluster` variable that reflects correct kafka cluster that
/// you want to use for tests. In order to run these tests call from command line:
/// `o2-qc-test-core [.manual_kafka]`
///
/// NOTE: it might be necessary to recreate or purge topic when you are doing
/// a lot of consecutive tests.
///

#include <catch_amalgamated.hpp>
#include <chrono>
#include <semaphore>
#include <thread>
#include "kafka/ProducerRecord.h"
#include "kafka/Types.h"

#include <QualityControl/KafkaPoller.h>
#include <QualityControl/Triggers.h>
#include <kafka/KafkaProducer.h>
#include <proto/events.pb.h>

// change this url if you are testing on different kafka cluster
constexpr auto kafkaCluster = "mtichak-flp-1-27.cern.ch:9092";
constexpr auto testTopic = "qc_test_topic";

constexpr auto globalRunNumber = 123;
constexpr auto timestamp = 1234;
constexpr auto globalEnvironmentId = "envID";

auto createSorProtoMessage(const std::string& environmentId = globalEnvironmentId, int runNumber = globalRunNumber) -> events::Event
{
  events::Event event;
  event.set_timestamp(1234);

  auto* runEvent = event.mutable_runevent();
  runEvent->set_transition("START_ACTIVITY");
  runEvent->set_state("CONFIGURED");
  runEvent->set_transitionstatus(events::OpStatus::STARTED);
  runEvent->set_environmentid(environmentId);
  runEvent->set_runnumber(runNumber);

  return event;
};

auto createEorProtoMessage(const std::string& environmentId = globalEnvironmentId, int runNumber = globalRunNumber) -> events::Event
{
  events::Event event;
  event.set_timestamp(1234);

  auto* runEvent = event.mutable_runevent();
  runEvent->set_transition("STOP_ACTIVITY");
  runEvent->set_state("RUNNING");
  runEvent->set_transitionstatus(events::OpStatus::STARTED);
  runEvent->set_environmentid(environmentId);
  runEvent->set_runnumber(runNumber);

  return event;
}

auto createEorTeardownProtoMessage() -> events::Event
{
  events::Event event;
  event.set_timestamp(1234);

  auto* runEvent = event.mutable_runevent();
  runEvent->set_transition("TEARDOWN");
  runEvent->set_state("RUNNING");
  runEvent->set_transitionstatus(events::OpStatus::STARTED);
  runEvent->set_environmentid(globalEnvironmentId);
  runEvent->set_runnumber(globalRunNumber);

  return event;
}

void sendMessage(kafka::clients::producer::KafkaProducer& producer, events::Event&& event, const std::string& topic = testTopic)
{
  std::binary_semaphore semaphore{ 0 };
  auto deliveryCb = [&semaphore](const kafka::clients::producer::RecordMetadata& metadata, const kafka::Error& error) {
    semaphore.release();
  };

  std::string buffer{};
  REQUIRE(event.SerializeToString(&buffer));
  kafka::clients::producer::ProducerRecord record(topic, kafka::NullKey, kafka::Value(buffer.data(), buffer.size()));

  producer.send(record, deliveryCb, kafka::clients::producer::KafkaProducer::SendOption::ToCopyRecordValue);
  semaphore.acquire();
}

void sendSorAndEor()
{
  kafka::Properties props{ { { "bootstrap.servers", { kafkaCluster } } } };
  kafka::clients::producer::KafkaProducer producer(props);

  sendMessage(producer, createSorProtoMessage());
  sendMessage(producer, createEorProtoMessage());
}

void sendSorAndTeardown()
{
  kafka::Properties props{ {
    { "bootstrap.servers", { kafkaCluster } },
  } };
  kafka::clients::producer::KafkaProducer producer(props);

  sendMessage(producer, createSorProtoMessage());
  sendMessage(producer, createEorTeardownProtoMessage());
}

TEST_CASE("test_kafka_poller_soreor", "[.manual_kafka]")
{
  using namespace o2::quality_control::core;
  KafkaPoller kafkaPoller(kafkaCluster, "unitTestID");
  REQUIRE_NOTHROW(kafkaPoller.subscribe(testTopic));
  // this timeout help to keep order of subscribing and consuming
  std::this_thread::sleep_for(std::chrono::milliseconds{ 500 });

  sendSorAndEor();

  bool receivedSOR = false;
  bool receivedEOR = false;

  while (!receivedSOR || !receivedEOR) {
    std::this_thread::sleep_for(std::chrono::seconds{ 1 });
    const auto records = kafkaPoller.poll();

    for (const auto& record : records) {
      const auto event = proto::recordToEvent(record.value());
      if (proto::start_of_run::check(event.value(), "", 0)) {
        receivedSOR = true;
      }
      if (proto::end_of_run::check(event.value(), "", 0)) {
        receivedEOR = true;
      }
    }

    if (receivedSOR && receivedEOR) {
      break;
    }
  }
}

TEST_CASE("test_kafka_poller_sorteardown", "[.manual_kafka]")
{
  using namespace o2::quality_control::core;
  KafkaPoller kafkaPoller(kafkaCluster, "unitTestID");
  REQUIRE_NOTHROW(kafkaPoller.subscribe(testTopic));

  // this timeout help to keep order of subscribing and consuming
  std::this_thread::sleep_for(std::chrono::milliseconds{ 500 });

  sendSorAndEor();

  bool receivedSOR = false;
  bool receivedTeardown = false;

  while (!receivedSOR || !receivedTeardown) {
    std::this_thread::sleep_for(std::chrono::seconds{ 1 });
    const auto records = kafkaPoller.poll();

    for (const auto& record : records) {
      const auto event = proto::recordToEvent(record.value());
      if (proto::start_of_run::check(event.value(), "", 0)) {
        receivedSOR = true;
      }
      if (proto::end_of_run::check(event.value(), "", 0)) {
        receivedTeardown = true;
      }
    }

    if (receivedSOR && receivedTeardown) {
      break;
    }
  }
}

TEST_CASE("test_SOR_trigger", "[.manual_kafka]")
{
  using namespace o2::quality_control;

  constexpr auto differentEnvId = "differentEnvId";
  constexpr int differentRunNumber = 42;

  core::Activity constrainedActivity;
  constrainedActivity.mId = globalRunNumber;
  constrainedActivity.mProvenance = globalEnvironmentId;
  auto sorTriggerConstrainedFn = postprocessing::triggers::StartOfRun(kafkaCluster, testTopic, "TST", "sor_test_constrained", constrainedActivity);
  // NOTE: calls for triggers in the beginning of function are meant to get rid of any offset lags on cluster
  sorTriggerConstrainedFn();

  core::Activity runNumberConstrainedActivity;
  runNumberConstrainedActivity.mId = globalRunNumber;
  auto sorTriggerRunNumberConstrainedFn = postprocessing::triggers::StartOfRun(kafkaCluster, testTopic, "TST", "sor_test_runnumber", runNumberConstrainedActivity);
  sorTriggerRunNumberConstrainedFn();

  core::Activity envIdConstrainedActivity;
  envIdConstrainedActivity.mProvenance = globalEnvironmentId;
  auto sorTriggerEnvIdConstrainedFn = postprocessing::triggers::StartOfRun(kafkaCluster, testTopic, "TST", "sor_test_envid", envIdConstrainedActivity);
  sorTriggerEnvIdConstrainedFn();

  auto sorTriggerUnconstrainedFn = postprocessing::triggers::StartOfRun(kafkaCluster, testTopic, "TST", "sor_test_unconstrained");
  sorTriggerUnconstrainedFn();

  {
    auto trigger = sorTriggerUnconstrainedFn();
    REQUIRE(trigger.triggerType == postprocessing::TriggerType::No);

    auto constrainedTrigger = sorTriggerConstrainedFn();
    REQUIRE(constrainedTrigger.triggerType == postprocessing::TriggerType::No);

    auto runConstrainedTrigger = sorTriggerRunNumberConstrainedFn();
    REQUIRE(runConstrainedTrigger.triggerType == postprocessing::TriggerType::No);

    auto envIdConstrainedTrigger = sorTriggerEnvIdConstrainedFn();
    REQUIRE(envIdConstrainedTrigger.triggerType == postprocessing::TriggerType::No);
  }

  kafka::Properties props{ { { "bootstrap.servers", { kafkaCluster } } } };
  kafka::clients::producer::KafkaProducer producer(props);

  sendMessage(producer, createSorProtoMessage());

  {
    auto trigger = sorTriggerUnconstrainedFn();
    auto& filledActivity = trigger.activity;
    REQUIRE(trigger.triggerType == postprocessing::TriggerType::StartOfRun);
    REQUIRE(filledActivity.mId == globalRunNumber);
    REQUIRE(filledActivity.mProvenance == globalEnvironmentId);
    REQUIRE(filledActivity.mValidity.getMin() == timestamp);

    auto constrainedTrigger = sorTriggerConstrainedFn();
    auto& constrainedFilledActivity = trigger.activity;
    REQUIRE(constrainedTrigger.triggerType == postprocessing::TriggerType::StartOfRun);
    REQUIRE(constrainedFilledActivity.mId == globalRunNumber);
    REQUIRE(constrainedFilledActivity.mProvenance == globalEnvironmentId);
    REQUIRE(constrainedFilledActivity.mValidity.getMin() == timestamp);

    auto runConstrainedTrigger = sorTriggerRunNumberConstrainedFn();
    auto& constrainedNumberFilledActivity = trigger.activity;
    REQUIRE(runConstrainedTrigger.triggerType == postprocessing::TriggerType::StartOfRun);
    REQUIRE(constrainedNumberFilledActivity.mId == globalRunNumber);
    REQUIRE(constrainedNumberFilledActivity.mProvenance == globalEnvironmentId);
    REQUIRE(constrainedNumberFilledActivity.mValidity.getMin() == timestamp);

    auto envIdConstrainedTrigger = sorTriggerEnvIdConstrainedFn();
    auto& constrainedEnvIdFilledActivity = trigger.activity;
    REQUIRE(envIdConstrainedTrigger.triggerType == postprocessing::TriggerType::StartOfRun);
    REQUIRE(constrainedEnvIdFilledActivity.mId == globalRunNumber);
    REQUIRE(constrainedEnvIdFilledActivity.mProvenance == globalEnvironmentId);
    REQUIRE(constrainedEnvIdFilledActivity.mValidity.getMin() == timestamp);
  }

  sendMessage(producer, createSorProtoMessage(differentEnvId, differentRunNumber));

  {
    auto trigger = sorTriggerUnconstrainedFn();
    auto& filledActivity = trigger.activity;
    REQUIRE(trigger.triggerType == postprocessing::TriggerType::StartOfRun);
    REQUIRE(filledActivity.mId == differentRunNumber);
    REQUIRE(filledActivity.mProvenance == differentEnvId);
    REQUIRE(filledActivity.mValidity.getMin() == timestamp);

    auto constrainedTrigger = sorTriggerConstrainedFn();
    REQUIRE(constrainedTrigger.triggerType == postprocessing::TriggerType::No);

    auto runConstrainedTrigger = sorTriggerRunNumberConstrainedFn();
    REQUIRE(runConstrainedTrigger.triggerType == postprocessing::TriggerType::No);

    auto envIdConstrainedTrigger = sorTriggerEnvIdConstrainedFn();
    REQUIRE(envIdConstrainedTrigger.triggerType == postprocessing::TriggerType::No);
  }

  sendMessage(producer, createSorProtoMessage(differentEnvId, globalRunNumber));

  {
    auto trigger = sorTriggerUnconstrainedFn();
    auto& filledActivity = trigger.activity;
    REQUIRE(trigger.triggerType == postprocessing::TriggerType::StartOfRun);
    REQUIRE(filledActivity.mId == globalRunNumber);
    REQUIRE(filledActivity.mProvenance == differentEnvId);
    REQUIRE(filledActivity.mValidity.getMin() == timestamp);

    auto constrainedTrigger = sorTriggerConstrainedFn();
    REQUIRE(constrainedTrigger.triggerType == postprocessing::TriggerType::No);

    auto runConstrainedTrigger = sorTriggerRunNumberConstrainedFn();
    auto& constrainedNumberFilledActivity = trigger.activity;
    REQUIRE(runConstrainedTrigger.triggerType == postprocessing::TriggerType::StartOfRun);
    REQUIRE(constrainedNumberFilledActivity.mId == globalRunNumber);
    REQUIRE(constrainedNumberFilledActivity.mProvenance == differentEnvId);
    REQUIRE(constrainedNumberFilledActivity.mValidity.getMin() == timestamp);

    auto envIdConstrainedTrigger = sorTriggerEnvIdConstrainedFn();
    REQUIRE(envIdConstrainedTrigger.triggerType == postprocessing::TriggerType::No);
  }

  sendMessage(producer, createSorProtoMessage(globalEnvironmentId, differentRunNumber));

  {
    auto trigger = sorTriggerUnconstrainedFn();
    auto& filledActivity = trigger.activity;
    REQUIRE(trigger.triggerType == postprocessing::TriggerType::StartOfRun);
    REQUIRE(filledActivity.mId == differentRunNumber);
    REQUIRE(filledActivity.mProvenance == globalEnvironmentId);
    REQUIRE(filledActivity.mValidity.getMin() == timestamp);

    auto constrainedTrigger = sorTriggerConstrainedFn();
    REQUIRE(constrainedTrigger.triggerType == postprocessing::TriggerType::No);

    auto runConstrainedTrigger = sorTriggerRunNumberConstrainedFn();
    REQUIRE(runConstrainedTrigger.triggerType == postprocessing::TriggerType::No);

    auto envIdConstrainedTrigger = sorTriggerEnvIdConstrainedFn();
    auto& constrainedEnvIdFilledActivity = trigger.activity;
    REQUIRE(envIdConstrainedTrigger.triggerType == postprocessing::TriggerType::StartOfRun);
    REQUIRE(constrainedEnvIdFilledActivity.mId == differentRunNumber);
    REQUIRE(constrainedEnvIdFilledActivity.mProvenance == globalEnvironmentId);
    REQUIRE(constrainedEnvIdFilledActivity.mValidity.getMin() == timestamp);
  }
}

TEST_CASE("test_EOR_trigger", "[.manual_kafka]")
{
  using namespace o2::quality_control;

  constexpr auto differentEnvId = "differentEnvId";
  constexpr int differentRunNumber = 42;

  core::Activity constrainedActivity;
  constrainedActivity.mId = globalRunNumber;
  constrainedActivity.mProvenance = globalEnvironmentId;
  auto eorTriggerConstrainedFn = postprocessing::triggers::EndOfRun(kafkaCluster, testTopic, "TST", "eor_test_constrained", constrainedActivity);
  // NOTE: calls for triggers in the beginning of function are meant to get rid of any offset lags on cluster
  eorTriggerConstrainedFn();

  core::Activity runNumberConstrainedActivity;
  runNumberConstrainedActivity.mId = globalRunNumber;
  auto eorTriggerRunNumberConstrainedFn = postprocessing::triggers::EndOfRun(kafkaCluster, testTopic, "TST", "eor_test_runnumber", runNumberConstrainedActivity);
  eorTriggerRunNumberConstrainedFn();

  core::Activity envIdConstrainedActivity;
  envIdConstrainedActivity.mProvenance = globalEnvironmentId;
  auto eorTriggerEnvIdConstrainedFn = postprocessing::triggers::EndOfRun(kafkaCluster, testTopic, "TST", "eor_test_envid", envIdConstrainedActivity);
  eorTriggerEnvIdConstrainedFn();

  auto eorTriggerUnconstrainedFn = postprocessing::triggers::EndOfRun(kafkaCluster, testTopic, "TST", "eor_test_unconstrained");
  eorTriggerUnconstrainedFn();

  {
    auto trigger = eorTriggerUnconstrainedFn();
    REQUIRE(trigger.triggerType == postprocessing::TriggerType::No);

    auto constrainedTrigger = eorTriggerConstrainedFn();
    REQUIRE(constrainedTrigger.triggerType == postprocessing::TriggerType::No);

    auto runConstrainedTrigger = eorTriggerRunNumberConstrainedFn();
    REQUIRE(runConstrainedTrigger.triggerType == postprocessing::TriggerType::No);

    auto envIdConstrainedTrigger = eorTriggerEnvIdConstrainedFn();
    REQUIRE(envIdConstrainedTrigger.triggerType == postprocessing::TriggerType::No);
  }

  kafka::Properties props{ { { "bootstrap.servers", { kafkaCluster } } } };
  kafka::clients::producer::KafkaProducer producer(props);

  sendMessage(producer, createEorProtoMessage());

  {
    auto trigger = eorTriggerUnconstrainedFn();
    auto& filledActivity = trigger.activity;
    REQUIRE(trigger.triggerType == postprocessing::TriggerType::EndOfRun);
    REQUIRE(filledActivity.mId == globalRunNumber);
    REQUIRE(filledActivity.mProvenance == globalEnvironmentId);
    REQUIRE(filledActivity.mValidity.getMax() == timestamp);

    auto constrainedTrigger = eorTriggerConstrainedFn();
    auto& constrainedFilledActivity = trigger.activity;
    REQUIRE(constrainedTrigger.triggerType == postprocessing::TriggerType::EndOfRun);
    REQUIRE(constrainedFilledActivity.mId == globalRunNumber);
    REQUIRE(constrainedFilledActivity.mProvenance == globalEnvironmentId);
    REQUIRE(constrainedFilledActivity.mValidity.getMax() == timestamp);

    auto runConstrainedTrigger = eorTriggerRunNumberConstrainedFn();
    auto& constrainedNumberFilledActivity = trigger.activity;
    REQUIRE(runConstrainedTrigger.triggerType == postprocessing::TriggerType::EndOfRun);
    REQUIRE(constrainedNumberFilledActivity.mId == globalRunNumber);
    REQUIRE(constrainedNumberFilledActivity.mProvenance == globalEnvironmentId);
    REQUIRE(constrainedNumberFilledActivity.mValidity.getMax() == timestamp);

    auto envIdConstrainedTrigger = eorTriggerEnvIdConstrainedFn();
    auto& constrainedEnvIdFilledActivity = trigger.activity;
    REQUIRE(envIdConstrainedTrigger.triggerType == postprocessing::TriggerType::EndOfRun);
    REQUIRE(constrainedEnvIdFilledActivity.mId == globalRunNumber);
    REQUIRE(constrainedEnvIdFilledActivity.mProvenance == globalEnvironmentId);
    REQUIRE(constrainedEnvIdFilledActivity.mValidity.getMax() == timestamp);
  }

  sendMessage(producer, createEorProtoMessage(differentEnvId, differentRunNumber));

  {
    auto trigger = eorTriggerUnconstrainedFn();
    auto& filledActivity = trigger.activity;
    REQUIRE(trigger.triggerType == postprocessing::TriggerType::EndOfRun);
    REQUIRE(filledActivity.mId == differentRunNumber);
    REQUIRE(filledActivity.mProvenance == differentEnvId);
    REQUIRE(filledActivity.mValidity.getMax() == timestamp);

    auto constrainedTrigger = eorTriggerConstrainedFn();
    REQUIRE(constrainedTrigger.triggerType == postprocessing::TriggerType::No);

    auto runConstrainedTrigger = eorTriggerRunNumberConstrainedFn();
    REQUIRE(runConstrainedTrigger.triggerType == postprocessing::TriggerType::No);

    auto envIdConstrainedTrigger = eorTriggerEnvIdConstrainedFn();
    REQUIRE(envIdConstrainedTrigger.triggerType == postprocessing::TriggerType::No);
  }

  sendMessage(producer, createEorProtoMessage(differentEnvId, globalRunNumber));

  {
    auto trigger = eorTriggerUnconstrainedFn();
    auto& filledActivity = trigger.activity;
    REQUIRE(trigger.triggerType == postprocessing::TriggerType::EndOfRun);
    REQUIRE(filledActivity.mId == globalRunNumber);
    REQUIRE(filledActivity.mProvenance == differentEnvId);
    REQUIRE(filledActivity.mValidity.getMax() == timestamp);

    auto constrainedTrigger = eorTriggerConstrainedFn();
    REQUIRE(constrainedTrigger.triggerType == postprocessing::TriggerType::No);

    auto runConstrainedTrigger = eorTriggerRunNumberConstrainedFn();
    auto& constrainedNumberFilledActivity = trigger.activity;
    REQUIRE(runConstrainedTrigger.triggerType == postprocessing::TriggerType::EndOfRun);
    REQUIRE(constrainedNumberFilledActivity.mId == globalRunNumber);
    REQUIRE(constrainedNumberFilledActivity.mProvenance == differentEnvId);
    REQUIRE(constrainedNumberFilledActivity.mValidity.getMax() == timestamp);

    auto envIdConstrainedTrigger = eorTriggerEnvIdConstrainedFn();
    REQUIRE(envIdConstrainedTrigger.triggerType == postprocessing::TriggerType::No);
  }

  sendMessage(producer, createEorProtoMessage(globalEnvironmentId, differentRunNumber));

  {
    auto trigger = eorTriggerUnconstrainedFn();
    auto& filledActivity = trigger.activity;
    REQUIRE(trigger.triggerType == postprocessing::TriggerType::EndOfRun);
    REQUIRE(filledActivity.mId == differentRunNumber);
    REQUIRE(filledActivity.mProvenance == globalEnvironmentId);
    REQUIRE(filledActivity.mValidity.getMax() == timestamp);

    auto constrainedTrigger = eorTriggerConstrainedFn();
    REQUIRE(constrainedTrigger.triggerType == postprocessing::TriggerType::No);

    auto runConstrainedTrigger = eorTriggerRunNumberConstrainedFn();
    REQUIRE(runConstrainedTrigger.triggerType == postprocessing::TriggerType::No);

    auto envIdConstrainedTrigger = eorTriggerEnvIdConstrainedFn();
    auto& constrainedEnvIdFilledActivity = trigger.activity;
    REQUIRE(envIdConstrainedTrigger.triggerType == postprocessing::TriggerType::EndOfRun);
    REQUIRE(constrainedEnvIdFilledActivity.mId == differentRunNumber);
    REQUIRE(constrainedEnvIdFilledActivity.mProvenance == globalEnvironmentId);
    REQUIRE(constrainedEnvIdFilledActivity.mValidity.getMax() == timestamp);
  }
}
