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
/// \file   HistoProducer.cxx
/// \author Barthelemy von Haller
///
#include "QualityControl/HistoProducer.h"

#include <random>
#include <Common/Timer.h>
#include <TH1F.h>
#include <Common/Timer.h>
#include <QualityControl/MonitorObjectCollection.h>

using namespace o2::framework;

using SubSpec = o2::header::DataHeader::SubSpecificationType;
using namespace AliceO2::Common;

namespace o2::quality_control::core
{

DataProcessorSpec getHistoProducerSpec(size_t index)
{
  return DataProcessorSpec{
    "producer",
    Inputs{},
    Outputs{
      { { "out" }, "TST", "HISTO", static_cast<SubSpec>(index) } },
    getHistoProducerAlgorithm({ "TST", "HISTO", static_cast<SubSpec>(index) }, index)
  };
}

framework::AlgorithmSpec getHistoProducerAlgorithm(framework::ConcreteDataMatcher output, size_t index)
{
  return AlgorithmSpec{
    [=](InitContext&) {
      // this is the initialization code
      std::default_random_engine generator(time(nullptr));
      std::shared_ptr<Timer> timer = nullptr;
      double period = 2; // how many seconds between the updates of the histogram
      TH1F *histo = new TH1F("hello", "fromHistoProducer", 100, 0, 99);

      return [=](ProcessingContext& processingContext) mutable {
        // everything inside this lambda function is invoked in a loop, because this Data Processor has no inputs

        // setting up the timer
        if (!timer) {
          timer = std::make_shared<Timer>();
          timer->reset(static_cast<int>(1000000.0 * period));
        }
        // keeping the message rate
        double timeToSleep = timer->getRemainingTime();
        if (timeToSleep > 0) {
          usleep(timeToSleep * 1000000.0);
        }
        timer->increment();

        // Prepare array
        MonitorObjectCollection& monitorObjects = processingContext.outputs().make<MonitorObjectCollection>({ output.origin, output.description, output.subSpec });
        // Generate data
        histo->Fill(index%100); // just fill 1 bin
        monitorObjects.Add(histo);

        LOG(INFO) << "sending array with 1 histo (" << histo->GetName() << " : " << histo->GetTitle() << " : " << histo->GetEntries() << ")";
      };
    }
  };
}

} // namespace o2::quality_control::core
