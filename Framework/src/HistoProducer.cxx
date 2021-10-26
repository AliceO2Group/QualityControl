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
/// \file   HistoProducer.cxx
/// \author Barthelemy von Haller
///
#include "QualityControl/HistoProducer.h"

#include <Common/Timer.h>
#include <TH1F.h>
#include <QualityControl/MonitorObjectCollection.h>
#include <string>
#include <QualityControl/QcInfoLogger.h>
#include <fairlogger/Logger.h>

using namespace o2::framework;

using SubSpec = o2::header::DataHeader::SubSpecificationType;
using namespace AliceO2::Common;
using namespace std;

namespace o2::quality_control::core
{

framework::DataProcessorSpec getHistoProducerSpec(size_t subspec, size_t nbHistograms, bool noTobjArray)
{
  return DataProcessorSpec{
    "histoProducer-" + std::to_string(subspec),
    Inputs{},
    Outputs{
      { { "out" }, "TST", "HISTO", static_cast<SubSpec>(subspec) } },
    getHistoProducerAlgorithm({ "TST", "HISTO", static_cast<SubSpec>(subspec) }, nbHistograms, noTobjArray)
  };
}

framework::AlgorithmSpec getHistoProducerAlgorithm(framework::ConcreteDataMatcher output, size_t nbHistograms, bool noTobjArray)
{
  return AlgorithmSpec{
    [=](InitContext&) {
      // this is the initialization code
      std::shared_ptr<Timer> timer = nullptr;
      double period = 2; // how many seconds between the updates of the histogram
      vector<TH1F*> allHistos;
      allHistos.reserve(nbHistograms);
      for (size_t i = 0; i < nbHistograms; i++) {
        TH1F* histo = new TH1F(string("hello_") + i, "fromHistoProducer", 100, -3, 3);
        allHistos.push_back(histo);
      }

      return [=](ProcessingContext& processingContext) mutable {
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

        if (noTobjArray) { // just send the histogram, not a tobjarray
          TH1F& th1f = processingContext.outputs().make<TH1F>({ output.origin, output.description, output.subSpec }, "hello", "fromHistoProducer", 100, -3, 3);
          allHistos[0]->FillRandom("gaus", 100);
          th1f.Add(allHistos[0]);
          ILOG(Info, Devel) << "sending 1 histo named `hello`." << ENDM;
          return;
        }

        // Prepare the tobjarray
        MonitorObjectCollection& monitorObjects = processingContext.outputs().make<MonitorObjectCollection>({ output.origin, output.description, output.subSpec });
        // Fill histograms
        for (size_t i = 0; i < nbHistograms; i++) {
          allHistos[i]->FillRandom("gaus", 100);
          monitorObjects.Add(allHistos[i]);
        }
        ILOG(Info, Devel) << "Sending a TObjArray with " << nbHistograms << " histos named `hello_<index>`.";
      };
    }
  };
}

DataProcessorSpec getHistoPrinterSpec(size_t subspec)
{
  return DataProcessorSpec{
    "histoPrinter",
    Inputs{ { { "in" }, "TST", "HISTO", static_cast<SubSpec>(subspec) } },
    Outputs{},
    getHistoPrinterAlgorithm()
  };
}

void printHisto(shared_ptr<const TH1F>& histo)
{
  ILOG(Info, Devel) << "histo : " << histo->GetName() << " : " << histo->GetTitle() << ENDM;
  std::string bins = "BINS:";
  for (int i = 1; i <= histo->GetNbinsX(); i++) {
    bins += " " + std::to_string((int)histo->GetBinContent(i));
  }
  ILOG(Info, Devel) << bins << ENDM;
}

framework::AlgorithmSpec getHistoPrinterAlgorithm()
{
  return AlgorithmSpec{
    [=](InitContext&) {
      // this is the initialization code

      return [=](ProcessingContext& processingContext) mutable {
        // We don't know what we receive, so we test for an array and then try a TH1F.
        shared_ptr<const TObjArray> array = nullptr;
        shared_ptr<const TH1F> th1f = nullptr;
        try {
          array = processingContext.inputs().get<TObjArray*>("in");
        } catch (std::runtime_error& e) {
          // we failed to get the TObjArray, let's try a TH1F. If it fails it will throw.
          th1f = processingContext.inputs().get<TH1F*>("in");
        }
        if (array != nullptr) {
          for (auto* const tObject : *array) {
            std::shared_ptr<const TH1F> histo{ dynamic_cast<TH1F*>(tObject) };
            printHisto(histo);
          }
        } else if (th1f != nullptr) {
          printHisto(th1f);
        }
      };
    }
  };
}

} // namespace o2::quality_control::core
