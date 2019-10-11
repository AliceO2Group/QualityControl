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

using namespace o2::framework;

using SubSpec = o2::header::DataHeader::SubSpecificationType;
using namespace AliceO2::Common;

namespace o2::quality_control::core
{

DataProcessorSpec getDataProducerSpec(size_t minSize, size_t maxSize, double rate, bool fill, size_t index)
{
  return DataProcessorSpec{
    "producer-" + std::to_string(index),
    Inputs{},
    Outputs{
      { { "out" }, "TST", "RAWDATA", static_cast<SubSpec>(index) } },
    getDataProducerAlgorithm({ "TST", "RAWDATA", static_cast<SubSpec>(index) }, minSize, maxSize, rate, fill)
  };
}

framework::AlgorithmSpec getDataProducerAlgorithm(ConcreteDataMatcher output, size_t minSize, size_t maxSize, double rate, bool fill)
{
  return AlgorithmSpec{
    [=](InitContext&) {
      // this is the initialization code
      std::default_random_engine generator(time(nullptr));
      std::shared_ptr<Timer> timer = nullptr;

      // after the initialization, we return the processing callback
      return [=](ProcessingContext& processingContext) mutable {
        // everything inside this lambda function is invoked in a loop, because it this Data Processor has no inputs

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
        size_t length = minSize + (generator() % (maxSize - minSize));
        auto data = processingContext.outputs().make<char>({ output.origin, output.description, output.subSpec }, length);
        if (fill) {
          for (auto&& item : data) {
            item = static_cast<char>(generator());
          }
        }
      };
    }
  };
}

} // namespace o2::quality_control::core
