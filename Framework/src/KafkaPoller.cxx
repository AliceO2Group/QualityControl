#include "QualityControl/KafkaPoller.h"

#include "QualityControl/QcInfoLogger.h"
#include "kafka/Properties.h"
#include <chrono>
#include <ranges>

namespace o2::quality_control::core
{

kafka::Properties createProperties(const std::string& brokers)
{
  return { { { "bootstrap.servers", { brokers } } } };
}

KafkaPoller::KafkaPoller(const std::string& brokers)
  : mConsumer(createProperties(brokers))
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
