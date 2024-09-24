#include "QualityControl/KafkaPoller.h"

#include "QualityControl/QcInfoLogger.h"
#include "kafka/KafkaException.h"
#include "kafka/Properties.h"
#include "proto/events.pb.h"
#include <chrono>
#include <ranges>
#include <stdexcept>

namespace o2::quality_control::core
{

namespace proto
{

struct Ev_RunEventPartial {
  std::string_view state;
  events::OpStatus transitionStatus;
  std::string_view environmentID;
  int runNumber;
};

// return true if runEvent and runEventPartial have the same state and transition state and have
// DIFFERENT(!) runNumber AND environmentID
bool operator==(const events::Ev_RunEvent& runEvent, const Ev_RunEventPartial& runEventPartial)
{
  // TODO: Should we check whether the error is empty?
  if (runEvent.state() != runEventPartial.state ||
      runEvent.transitionstatus() != runEventPartial.transitionStatus) {
    return false;
  }

  if (runEvent.runnumber() == runEventPartial.runNumber && runEvent.environmentid() == runEventPartial.environmentID) {
    return false;
  }

  return true;
}

auto recordToEvent(const kafka::Value& kafkaRecord) -> std::optional<events::Event>
{
  events::Event event;

  if (!event.ParseFromArray(kafkaRecord.data(), kafkaRecord.size())) {
    ILOG(Error, Ops) << "Received wrong or inconsistent data while parser Event from kafka proto" << ENDM;
    return std::nullopt;
  }

  return event;
}

void fillActivityWithoutTimestamp(const events::Event& event, Activity& activity)
{
  if (event.has_runevent()) {
    auto& runEvent = event.runevent();
    activity.mId = runEvent.runnumber();
    activity.mPartitionName = runEvent.environmentid();
  }
}

void start_of_run::fillActivity(const events::Event& event, Activity& activity)
{
  fillActivityWithoutTimestamp(event, activity);
  activity.mValidity.setMin(event.timestamp());
}

bool start_of_run::isValid(const events::Event& event, const std::string& environmentID, int runNumber)
{
  if (!event.has_runevent()) {
    return false;
  }

  const auto& runEvent = event.runevent();

  if (runEvent.transition() != "START_ACTIVITY") {
    return false;
  }

  if (runEvent.transitionstatus() != events::OpStatus::STARTED) {
    return false;
  }

  return runEvent == Ev_RunEventPartial{ "CONFIGURED", events::OpStatus::STARTED, environmentID, runNumber };
}

void end_of_run::fillActivity(const events::Event& event, Activity& activity)
{
  fillActivityWithoutTimestamp(event, activity);
  activity.mValidity.setMax(event.timestamp());
}

bool end_of_run::isValid(const events::Event& event, const std::string& environmentID, int runNumber)
{
  if (!event.has_runevent()) {
    return false;
  }

  const auto& runEvent = event.runevent();

  if (runEvent.transition() != "STOP_ACTIVITY" && runEvent.transition() != "TEARDOWN") {
    return false;
  }

  if (runEvent.transitionstatus() != events::OpStatus::STARTED) {
    return false;
  }

  return runEvent == Ev_RunEventPartial{ "RUNNING", events::OpStatus::STARTED, environmentID, runNumber };
}

} // namespace proto

kafka::Properties createProperties(const std::string& brokers, const std::string& groupId)
{
  if (brokers.empty()) {
    constexpr std::string_view message{ "You are trying to start KafkaPoller without any brokers" };
    ILOG(Fatal, Ops) << message << ENDM;
    throw std::invalid_argument{ message.data() };
  }

  auto properties = kafka::Properties{ { { "bootstrap.servers", { brokers } },
                                         { "enable.auto.commit", { "true" } },
                                         { "auto.offset.reset", { "latest" } } } };

  if (!groupId.empty()) {
    properties.put("group.id", groupId);
  }

  return properties;
}

KafkaPoller::KafkaPoller(const std::string& brokers, const std::string& groupId)
  : mConsumer(createProperties(brokers, groupId))
{
}

void KafkaPoller::subscribe(const std::string& topic, size_t numberOfRetries)
{
  for (size_t retryNumber = 0; retryNumber != numberOfRetries; ++retryNumber) {
    try {
      mConsumer.subscribe({ topic });
      return;
    } catch (const kafka::KafkaException& ex) {
      // it sometimes happens that subscibe timeouts but another retry succeeds
      if (ex.error().value() != RD_KAFKA_RESP_ERR__TIMED_OUT) {
        throw;
      } else {
        ILOG(Warning, Ops) << "Failed to subscribe to kafka due to timeout " << retryNumber + 1 << "/" << numberOfRetries + 1 << " times, retrying..." << ENDM;
      }
    }
  }

  throw std::runtime_error(std::string{ "Kafka Poller failed to subscribe after " }.append(std::to_string(numberOfRetries)).append(" retries"));
}

auto KafkaPoller::poll(std::chrono::milliseconds timeout) -> KafkaRecords
{
  const auto records = mConsumer.poll(timeout);
  auto filtered = records | std::ranges::views::filter([](const auto& record) { return !record.error(); });
  KafkaRecords result{};
  result.reserve(records.size());
  std::ranges::copy(filtered, std::back_inserter(result));
  return result;
}

} // namespace o2::quality_control::core
