///
/// \file    runMergerTest.cxx
/// \author  Piotr Konopka
///
/// \brief This is DPL workflow to see HistoMerger in action

#include <random>
#include <memory>
#include <TH1F.h>
#include <FairLogger.h>
#include <Framework/CompletionPolicy.h>
#include <Framework/CompletionPolicyHelpers.h>
using namespace o2::framework;

void customize(std::vector<CompletionPolicy>& policies)
{
  CompletionPolicy mergerConsumesASAP = CompletionPolicyHelpers::defineByName("merger", CompletionPolicy::CompletionOp::Consume);
  policies.push_back(mergerConsumesASAP);
}

#include "Framework/runDataProcessing.h"

#include "QualityControl/TaskDataProcessorFactory.h"
#include "QualityControl/CheckerDataProcessorFactory.h"
#include "QualityControl/CheckerDataProcessor.h"
#include "QualityControl/HistoMerger.h"

using namespace o2::quality_control::core;
using namespace o2::quality_control::checker;
using namespace std::chrono;

WorkflowSpec defineDataProcessing(ConfigContext const&)
{
  WorkflowSpec specs;

  size_t producersAmount = 20;
  for(size_t p = 0; p < producersAmount; p++) {
    DataProcessorSpec producer1{
      "producer" + std::to_string(p),
      Inputs{},
      Outputs{
        { {"mo"}, "TST", "HISTO", p + 1, Lifetime::Timeframe }
      },
      AlgorithmSpec{
        (AlgorithmSpec::ProcessCallback) [p, producersAmount](ProcessingContext& processingContext) mutable {

          usleep(100000);

          TH1F* histo = new TH1F("gauss", "gauss", producersAmount, 0, 1);
          histo->Fill(p/(double)producersAmount);

          MonitorObject* mo = new MonitorObject("histo", histo, "histo-task");
          mo->setIsOwner(false);

          processingContext.outputs().snapshot<MonitorObject>(Output{"TST", "HISTO", p + 1}, *mo);
//        processingContext.outputs().snapshot<MonitorObject>(OutputRef{ "mo", 1 }, *mo);
          delete mo;
          delete histo;
        }
      }
    };
    specs.push_back(producer1);
  }

  HistoMerger merger("merger");
  merger.configureEdges("TST", "HISTO", {1, producersAmount});
  merger.setPublicationRate(1000000);
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
      { "mo", "TST", "HISTO", 0 }
    },
    Outputs{},
    AlgorithmSpec{
      (AlgorithmSpec::InitCallback) [](InitContext&) {

        return (AlgorithmSpec::ProcessCallback) [](ProcessingContext& processingContext) mutable {
          LOG(INFO) << "printer invoked";
          auto mo = processingContext.inputs().get<MonitorObject*>("mo").get();

          if (mo->getName() == "histo") {
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
