#include "QualityControl/KafkaPoller.h"

#include "QualityControl/QcInfoLogger.h"
#include "kafka/Properties.h"
#include "proto/events.pb.h"
#include <chrono>
#include <ranges>

namespace o2::quality_control::core
{

namespace proto_parser
{

bool contains(const events::Ev_RunEvent& runEvent, std::string_view state, events::OpStatus transitionStatus, std::optional<uint32_t> runNumber, std::optional<std::string_view> environmentID)
{
  // TODO: Should we check whether the error is empty?
  if (runEvent.state() != state ||
      runEvent.transitionstatus() != transitionStatus) {
    return false;
  }

  if (runNumber.has_value() && runEvent.runnumber() != runNumber.value()) {
    return false;
  }

  if (environmentID.has_value() && runEvent.environmentid() != environmentID.value()) {
    return false;
  }

  return true;
}

bool isSOR(const kafka::Value& kafkaMessage, const std::optional<std::string> envId, const std::optional<uint32_t> runNumber)
{
  events::Event event;

  if (!event.ParseFromArray(kafkaMessage.data(), kafkaMessage.size())) {
    ILOG(Error, Ops) << "Received wrong or inconsistent data in SOR parser" << ENDM;
    return false;
  }

  if (event.Payload_case() != events::Event::PayloadCase::kRunEvent) {
    return false;
  }

  const auto& runEvent = event.runevent();

  if (runEvent.transition() != "START_ACTIVITY") {
    return false;
  }

  // std::cout << "SOR!!!\n";
  // std::cout << "transition: " << runEvent.transition() << "\n";
  // std::cout << "state: " << runEvent.state() << "\n";
  // std::cout << "error: " << runEvent.error() << "\n";
  // std::cout << "transitionStatus: " << runEvent.transitionstatus() << "\n";
  // std::cout << "envid: " << runEvent.environmentid() << "\n";
  // std::cout << "runnumber: " << runEvent.runnumber() << "\n";

  return contains(runEvent, "CONFIGURED", events::OpStatus::STARTED, runNumber, envId);

  return true;
}

bool isEOR(const kafka::Value& kafkaMessage, const std::optional<std::string> envId, const std::optional<uint32_t> runNumber)
{
  events::Event event;

  if (!event.ParseFromArray(kafkaMessage.data(), kafkaMessage.size())) {
    ILOG(Error, Ops) << "Received wrong or inconsistent data in EOR parser" << ENDM;
    return false;
  }

  if (event.Payload_case() != events::Event::PayloadCase::kRunEvent) {
    return false;
  }

  const auto& runEvent = event.runevent();

  if (runEvent.transition() != "STOP_ACTIVITY" && runEvent.transition() != "TEARDOWN") {
    return false;
  }

  // std::cout << "EOR!!!\n";
  // std::cout << "transition: " << runEvent.transition() << "\n";
  // std::cout << "state: " << runEvent.state() << "\n";
  // std::cout << "error: " << runEvent.error() << "\n";
  // std::cout << "transitionStatus: " << runEvent.transitionstatus() << "\n";
  // std::cout << "envid: " << runEvent.environmentid() << "\n";
  // std::cout << "runnumber: " << runEvent.runnumber() << "\n";

  return contains(runEvent, "RUNNING", events::OpStatus::STARTED, runNumber, envId);
}

} // namespace proto_parser

kafka::Properties createProperties(const std::string& brokers, const std::string& groupId)
{
  return { { { "bootstrap.servers", { brokers } },
             { "group.id", { groupId } },
             { "enable.auto.commit", { "true" } } } };
}

KafkaPoller::KafkaPoller(const std::string& brokers, const std::string& groupId)
  : mConsumer(createProperties(brokers, groupId))
{
}

void KafkaPoller::subscribe(const std::string& topic)
{
  mConsumer.subscribe({ topic });
}

auto KafkaPoller::poll() -> KafkaRecords
{
  const auto records = mConsumer.poll(std::chrono::milliseconds{ 100 });
  auto filtered = records | std::ranges::views::filter([](const auto& record) { return !record.error(); });
  KafkaRecords result{};
  result.reserve(records.size());
  std::ranges::copy(filtered, std::back_inserter(result));
  return result;
}

} // namespace o2::quality_control::core
