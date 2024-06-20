#include "QualityControl/KafkaConsumer.h"
#include <chrono>
#include "kafka/Properties.h"

namespace o2::quality_control::core
{

kafka::Properties createProperties(const std::string& brokers)
{
  return { { { "bootstrap.servers", { brokers } } } };
}

KafkaConsumer::KafkaConsumer(const std::string& brokers)
  : mConsumer(createProperties(brokers))
{
}

void KafkaConsumer::consume(const std::string& topic)
{
  mRunning = true;
  std::cout << "starting consuming topic: " << topic << "\n";

  mConsumer.subscribe({ topic });

  while (mRunning) {
    const auto records = mConsumer.poll(std::chrono::milliseconds{ 100 });

    for (const auto& record : records) {
      if (!record.error()) {
        std::cout << "Got kafka message with value: " << record.value().toString() << "\n";
      }
    }
  }
}

void KafkaConsumer::stop() noexcept
{
  mRunning = false;
}

} // namespace o2::quality_control::core
