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
/// In order to run these tests you need to setup TEST_KAFKA_CLUSTER environment
/// variable and to run these tests call from command line:
/// `o2-qc-test-core [.manual_kafka]`
///
/// NOTE: it might be necessary to recreate or purge topic when you are doing
/// a lot of consecutive tests.

#include <catch_amalgamated.hpp>
#include <chrono>
#include <cstdlib>
#include <semaphore>
#include <stdexcept>
#include <thread>
#include "kafka/ProducerRecord.h"
#include "kafka/Types.h"

#include <QualityControl/KafkaPoller.h>
#include <QualityControl/Triggers.h>
#include <kafka/KafkaProducer.h>
#include <proto/events.pb.h>

// get test kafka cluster id from Environment variable TEST_KAFKA_CLUSTER
auto getClusterURLFromEnv() -> std::string
{
  const auto* val = std::getenv("TEST_KAFKA_CLUSTER");
  if (val == nullptr) {
    throw std::runtime_error("TEST_KAFKA_CLUSTER env variable was not set");
  }
  return std::string{ val };
}

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
  const auto kafkaCluster = getClusterURLFromEnv();
  kafka::Properties props{ { { "bootstrap.servers", { kafkaCluster } } } };
  kafka::clients::producer::KafkaProducer producer(props);

  sendMessage(producer, createSorProtoMessage());
  sendMessage(producer, createEorProtoMessage());
}

void sendSorAndTeardown()
{
  kafka::Properties props{ {
    { "bootstrap.servers", { getClusterURLFromEnv() } },
  } };
  kafka::clients::producer::KafkaProducer producer(props);

  sendMessage(producer, createSorProtoMessage());
  sendMessage(producer, createEorTeardownProtoMessage());
}

TEST_CASE("test_kafka_poller_soreor", "[.manual_kafka]")
{
  const auto kafkaCluster = getClusterURLFromEnv();
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
      // we just need values that are different from global ones
      if (proto::start_of_run::isValid(event.value(), "randomEnvID", 1)) {
        receivedSOR = true;
      }
      if (proto::end_of_run::isValid(event.value(), "randomEnvID", 1)) {
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
  const auto kafkaCluster = getClusterURLFromEnv();
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
      // we just need values that are different from global ones
      if (proto::start_of_run::isValid(event.value(), "randomEnvID", 1)) {
        receivedSOR = true;
      }
      if (proto::end_of_run::isValid(event.value(), "randomEnvID", 1)) {
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

  const auto kafkaCluster = getClusterURLFromEnv();
  constexpr auto differentEnvId = "differentEnvId";
  constexpr int differentRunNumber = 42;

  core::Activity activityMatching;
  activityMatching.mId = differentRunNumber;
  activityMatching.mPartitionName = differentEnvId;
  auto sorTriggerMatchingFn = postprocessing::triggers::StartOfRun(kafkaCluster, testTopic, "TST", "sor_test_matching", activityMatching);
  // NOTE: calls for triggers in the beginning of function are meant to get rid of any offset lags on cluster
  sorTriggerMatchingFn();

  core::Activity activityNotMatching;
  activityNotMatching.mId = globalRunNumber;
  activityNotMatching.mPartitionName = globalEnvironmentId;
  auto sorTriggerNotMatchingFn = postprocessing::triggers::StartOfRun(kafkaCluster, testTopic, "TST", "sor_test_notmatching", activityNotMatching);
  sorTriggerNotMatchingFn();

  core::Activity activityMatchingDiffRunNumber;
  activityMatchingDiffRunNumber.mId = differentRunNumber;
  activityMatchingDiffRunNumber.mPartitionName = globalEnvironmentId;
  auto sorTriggerNotMatchingDiffRunNumber = postprocessing::triggers::StartOfRun(kafkaCluster, testTopic, "TST", "sor_test_not_matching_diff_runnumber", activityMatchingDiffRunNumber);
  sorTriggerNotMatchingDiffRunNumber();

  core::Activity activityMatchingDiffEnvId;
  activityMatchingDiffEnvId.mId = globalRunNumber;
  activityMatchingDiffEnvId.mPartitionName = differentEnvId;
  auto sorTriggerNotMatchingDiffEnvId = postprocessing::triggers::StartOfRun(kafkaCluster, testTopic, "TST", "sor_test_not_matching_diff_envid", activityMatchingDiffEnvId);
  sorTriggerNotMatchingDiffEnvId();

  {
    auto trigger = sorTriggerMatchingFn();
    REQUIRE(trigger.triggerType == postprocessing::TriggerType::No);

    auto notMatchingTrigger = sorTriggerNotMatchingFn();
    REQUIRE(notMatchingTrigger.triggerType == postprocessing::TriggerType::No);

    auto notMatchingTriggerDiffRunNumber = sorTriggerNotMatchingDiffRunNumber();
    REQUIRE(notMatchingTriggerDiffRunNumber.triggerType == postprocessing::TriggerType::No);

    auto notMatchingTriggerDiffEnvId = sorTriggerNotMatchingDiffEnvId();
    REQUIRE(notMatchingTriggerDiffEnvId.triggerType == postprocessing::TriggerType::No);
  }

  kafka::Properties props{ { { "bootstrap.servers", { kafkaCluster } } } };
  kafka::clients::producer::KafkaProducer producer(props);

  sendMessage(producer, createSorProtoMessage());

  {
    auto trigger = sorTriggerMatchingFn();
    auto& filledActivity = trigger.activity;
    REQUIRE(trigger.triggerType == postprocessing::TriggerType::StartOfRun);
    REQUIRE(filledActivity.mId == globalRunNumber);                // was set as differentRunNumber
    REQUIRE(filledActivity.mPartitionName == globalEnvironmentId); // was set as differentEnvId
    REQUIRE(filledActivity.mValidity.getMin() == timestamp);

    auto notmatchingTrigger = sorTriggerNotMatchingFn();
    REQUIRE(notmatchingTrigger.triggerType == postprocessing::TriggerType::No); // was set as globalEnvironmentId and globalRunNumber

    auto notMatchingTriggerDiffRunNumber = sorTriggerNotMatchingDiffRunNumber();
    auto& notMatchingActivityDiffRunNumber = trigger.activity;
    REQUIRE(notMatchingTriggerDiffRunNumber.triggerType == postprocessing::TriggerType::StartOfRun);
    REQUIRE(notMatchingActivityDiffRunNumber.mId == globalRunNumber);                // was set as globalRunNumber
    REQUIRE(notMatchingActivityDiffRunNumber.mPartitionName == globalEnvironmentId); // was set as differentEnvId
    REQUIRE(notMatchingActivityDiffRunNumber.mValidity.getMin() == timestamp);

    auto notMatchingTriggerDiffEnvId = sorTriggerNotMatchingDiffEnvId();
    auto& notMatchingActivityDiffEnvId = trigger.activity;
    REQUIRE(notMatchingTriggerDiffEnvId.triggerType == postprocessing::TriggerType::StartOfRun);
    REQUIRE(notMatchingActivityDiffRunNumber.mId == globalRunNumber);                // was set as differentRunNumber
    REQUIRE(notMatchingActivityDiffRunNumber.mPartitionName == globalEnvironmentId); // was set as globalEnvironmentId
    REQUIRE(notMatchingActivityDiffEnvId.mValidity.getMin() == timestamp);
  }
}

TEST_CASE("test_EOR_trigger", "[.manual_kafka]")
{
  using namespace o2::quality_control;

  const auto kafkaCluster = getClusterURLFromEnv();
  constexpr auto differentEnvId = "differentEnvId";
  constexpr int differentRunNumber = 42;

  core::Activity activityMatching;
  activityMatching.mId = differentRunNumber;
  activityMatching.mPartitionName = differentEnvId;
  auto eorTriggerMatchingFn = postprocessing::triggers::EndOfRun(kafkaCluster, testTopic, "TST", "eor_test_matching", activityMatching);
  // NOTE: calls for triggers in the beginning of function are meant to get rid of any offset lags on cluster
  eorTriggerMatchingFn();

  core::Activity activityNotMatching;
  activityNotMatching.mId = globalRunNumber;
  activityNotMatching.mPartitionName = globalEnvironmentId;
  auto eorTriggerNotMatchingFn = postprocessing::triggers::EndOfRun(kafkaCluster, testTopic, "TST", "eor_test_notmatching", activityNotMatching);
  eorTriggerNotMatchingFn();

  core::Activity activityMatchingDiffRunNumber;
  activityMatchingDiffRunNumber.mId = differentRunNumber;
  activityMatchingDiffRunNumber.mPartitionName = globalEnvironmentId;
  auto eorTriggerNotMatchingDiffRunNumber = postprocessing::triggers::EndOfRun(kafkaCluster, testTopic, "TST", "eor_test_not_matching_diff_runnumber", activityMatchingDiffRunNumber);
  eorTriggerNotMatchingDiffRunNumber();

  core::Activity activityMatchingDiffEnvId;
  activityMatchingDiffEnvId.mId = globalRunNumber;
  activityMatchingDiffEnvId.mPartitionName = differentEnvId;
  auto eorTriggerNotMatchingDiffEnvId = postprocessing::triggers::EndOfRun(kafkaCluster, testTopic, "TST", "eor_test_not_matching_diff_envid", activityMatchingDiffEnvId);
  eorTriggerNotMatchingDiffEnvId();

  {
    auto trigger = eorTriggerMatchingFn();
    REQUIRE(trigger.triggerType == postprocessing::TriggerType::No);

    auto notMatchingTrigger = eorTriggerNotMatchingFn();
    REQUIRE(notMatchingTrigger.triggerType == postprocessing::TriggerType::No);

    auto notMatchingTriggerDiffRunNumber = eorTriggerNotMatchingDiffRunNumber();
    REQUIRE(notMatchingTriggerDiffRunNumber.triggerType == postprocessing::TriggerType::No);

    auto notMatchingTriggerDiffEnvId = eorTriggerNotMatchingDiffEnvId();
    REQUIRE(notMatchingTriggerDiffEnvId.triggerType == postprocessing::TriggerType::No);
  }

  kafka::Properties props{ { { "bootstrap.servers", { kafkaCluster } } } };
  kafka::clients::producer::KafkaProducer producer(props);

  sendMessage(producer, createEorProtoMessage());

  {
    auto trigger = eorTriggerMatchingFn();
    auto& filledActivity = trigger.activity;
    REQUIRE(trigger.triggerType == postprocessing::TriggerType::EndOfRun);
    REQUIRE(filledActivity.mId == globalRunNumber);                // was set as differentRunNumber
    REQUIRE(filledActivity.mPartitionName == globalEnvironmentId); // was set as differentEnvId
    REQUIRE(filledActivity.mValidity.getMax() == timestamp);

    auto notmatchingTrigger = eorTriggerNotMatchingFn();
    REQUIRE(notmatchingTrigger.triggerType == postprocessing::TriggerType::No); // was set as globalEnvironmentId and globalRunNumber

    auto notMatchingTriggerDiffRunNumber = eorTriggerNotMatchingDiffRunNumber();
    auto& notMatchingActivityDiffRunNumber = trigger.activity;
    REQUIRE(notMatchingTriggerDiffRunNumber.triggerType == postprocessing::TriggerType::EndOfRun);
    REQUIRE(notMatchingActivityDiffRunNumber.mId == globalRunNumber);                // was set as globalRunNumber
    REQUIRE(notMatchingActivityDiffRunNumber.mPartitionName == globalEnvironmentId); // was set as differentEnvId
    REQUIRE(notMatchingActivityDiffRunNumber.mValidity.getMax() == timestamp);

    auto notMatchingTriggerDiffEnvId = eorTriggerNotMatchingDiffEnvId();
    auto& notMatchingActivityDiffEnvId = trigger.activity;
    REQUIRE(notMatchingTriggerDiffEnvId.triggerType == postprocessing::TriggerType::EndOfRun);
    REQUIRE(notMatchingActivityDiffRunNumber.mId == globalRunNumber);                // was set as differentRunNumber
    REQUIRE(notMatchingActivityDiffRunNumber.mPartitionName == globalEnvironmentId); // was set as globalEnvironmentId
    REQUIRE(notMatchingActivityDiffEnvId.mValidity.getMax() == timestamp);
  }
}
