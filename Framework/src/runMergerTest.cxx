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
/// \file    runMergerTest.cxx
/// \author  Piotr Konopka
///
/// \brief This is DPL workflow to see HistoMerger in action

#include <fairlogger/Logger.h>
#include <Framework/CompletionPolicy.h>
#include <Framework/CompletionPolicyHelpers.h>
#include <TH1F.h>
#include <memory>
#include <random>
using namespace o2::framework;

// The customize() functions are used to declare the executable arguments and to specify custom completion and channel
// configuration policies. They have to be above `#include "Framework/runDataProcessing.h"` - that header checks if
// these functions are defined by user and if so, it invokes them. It uses a trick with SFINAE expressions to do that.

void customize(std::vector<CompletionPolicy>& policies)
{
  CompletionPolicy mergerConsumesASAP =
    CompletionPolicyHelpers::defineByName("merger", CompletionPolicy::CompletionOp::Consume);
  policies.push_back(mergerConsumesASAP);
}

#include <Framework/runDataProcessing.h>

#include "QualityControl/CheckerFactory.h"
#include "QualityControl/HistoMerger.h"

using namespace o2::quality_control::core;
using namespace o2::quality_control::checker;
using namespace std::chrono;

// clang-format off
WorkflowSpec defineDataProcessing(ConfigContext const&)
{
  WorkflowSpec specs;

  size_t producersAmount = 10;
  for(size_t p = 0; p < producersAmount; p++) {
    DataProcessorSpec producer{
      "producer" + std::to_string(p),
      Inputs{},
      Outputs{
        { { "mo" }, "TST", "HISTO", static_cast<o2::framework::DataAllocator::SubSpecificationType>(p + 1), Lifetime::Timeframe } },
      AlgorithmSpec{
        (AlgorithmSpec::ProcessCallback)[p, producersAmount](ProcessingContext & processingContext) mutable {

          usleep(100000);

          TH1F* histo = new TH1F("gauss", "gauss", producersAmount, 0, 1);
          histo->Fill(p / (double)producersAmount);

          MonitorObject* mo = new MonitorObject(histo, "histo-task");
          mo->setIsOwner(true);

          TObjArray* array = new TObjArray;
          array->SetOwner(true);
          array->Add(mo);

          processingContext.outputs().adopt(Output{ "TST", "HISTO", static_cast<o2::framework::DataAllocator::SubSpecificationType>(p + 1) }, array);
        }
      }
    };
    specs.push_back(producer);
  }

  HistoMerger merger("merger", 1);
  merger.configureInputsOutputs("TST", "HISTO", { 1, producersAmount });
  DataProcessorSpec mergerSpec{
    merger.getName(),
    merger.getInputSpecs(),
    Outputs{merger.getOutputSpec()},
    adaptFromTask<HistoMerger>(std::move(merger)),
  };
  specs.push_back(mergerSpec);

  DataProcessorSpec printer{
    "printer",
    Inputs{
      { "moarray", "TST", "HISTO", 0 }
    },
    Outputs{},
    AlgorithmSpec{
      (AlgorithmSpec::InitCallback) [](InitContext&) {
        return (AlgorithmSpec::ProcessCallback) [](ProcessingContext& processingContext) mutable {
          LOG(INFO) << "printer invoked";
          auto moArray = processingContext.inputs().get<TObjArray*>("moarray");
          auto mo = dynamic_cast<MonitorObject*>(moArray->First());

          if (mo->getName() == "gauss") {
            auto* g = dynamic_cast<TH1F*>(mo->getObject());
            std::string bins = "BINS:";
            for (int i = 0; i <= g->GetNbinsX(); i++) {
              bins += " " + std::to_string((int) g->GetBinContent(i));
            }
            LOG(INFO) << bins;
          }
        };
      }
    }
  };
  specs.push_back(printer);

  return specs;
}
// clang-format on
