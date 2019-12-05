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
/// \file   DataProducer.cxx
/// \author Piotr Konopka
///
#include "QualityControl/DataProducer.h"

#include <random>
#include <Common/Timer.h>
#include <Monitoring/MonitoringFactory.h>
#include <Framework/ControlService.h>

using namespace o2::framework;
using namespace o2::monitoring;

using SubSpec = o2::header::DataHeader::SubSpecificationType;
using namespace AliceO2::Common;

namespace o2::quality_control::core
{

DataProcessorSpec getDataProducerSpec(size_t minSize, size_t maxSize, double rate, uint64_t amount, size_t index,
                                      std::string monitoringUrl, bool fill)
{
  return DataProcessorSpec{
    "producer-" + std::to_string(index),
    Inputs{},
    Outputs{
      { { "out" }, "TST", "RAWDATA", static_cast<SubSpec>(index) } },
    getDataProducerAlgorithm({ "TST", "RAWDATA", static_cast<SubSpec>(index) }, minSize, maxSize, rate, amount,
                             monitoringUrl, fill)
  };
}

AlgorithmSpec getDataProducerAlgorithm(ConcreteDataMatcher output, size_t minSize, size_t maxSize, double rate,
                                       uint64_t amount, std::string monitoringUrl, bool fill)
{
  return AlgorithmSpec{
    [=](InitContext&) {
      // this is the initialization code
      std::default_random_engine generator(time(nullptr));
      std::shared_ptr<Timer> timer = nullptr;

      uint64_t messageCounter = 0;
      std::shared_ptr<monitoring::Monitoring> collector;
      if (!monitoringUrl.empty()) {
        collector = MonitoringFactory::Get(monitoringUrl);
        collector->enableProcessMonitoring();
      }

      // after the initialization, we return the processing callback
      return [=](ProcessingContext& processingContext) mutable {
        // everything inside this lambda function is invoked in a loop, because it this Data Processor has no inputs

        // checking if we have reached the maximum amount of messages
        if (amount != 0 && messageCounter >= amount) {
          processingContext.services().get<ControlService>().endOfStream();
          processingContext.services().get<ControlService>().readyToQuit(QuitRequest::Me);
          return;
        }

        // setting up the timer
        if (!timer) {
          timer = std::make_shared<Timer>();
          timer->reset(static_cast<int>(1000000.0 / rate));
        }
        // keeping the message rate
        double timeToSleep = timer->getRemainingTime();
        if (timeToSleep > 0) {
          usleep(timeToSleep * 1000000.0);
        }
        timer->increment();

        // generating data
        size_t length = (minSize == maxSize) ? minSize : (minSize + (generator() % (maxSize - minSize)));
        auto data = processingContext.outputs().make<char>({ output.origin, output.description, output.subSpec },
                                                           length);
        ++messageCounter;
        if (fill) {
          for (auto&& item : data) {
            item = static_cast<char>(generator());
          }
        }

        // send metrics
        if (collector) {
          collector->send({ messageCounter, "Data_producer_" + std::to_string(output.subSpec) + "_message_" },
                          DerivedMetricMode::RATE);
        }
      };
    }
  };
}

} // namespace o2::quality_control::core
