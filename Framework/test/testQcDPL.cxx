// Copyright CERN and copyright holders of ALICE O2. This software is
// distributed under the terms of the GNU General Public License v3 (GPL
// Version 3), copied verbatim in the file "COPYING".
//
// See http://alice-o2.web.cern.ch/license for full licensing information.
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.

#include <random>
#include <memory>
#include <TH1F.h>

#include "Framework/DataSampling.h"
#include "Framework/runDataProcessing.h"
#include "QualityControl/TaskDataProcessorFactory.h"
#include "QualityControl/TaskDataProcessor.h"

using namespace std;
using namespace AliceO2;
using namespace o2::framework;
using namespace o2::quality_control::core;
using namespace std::chrono;

//todo:
// rename it to taskDPL.cxx, move it to src, say it is not usable because there are no arguments passed to exe

void defineDataProcessing(vector<DataProcessorSpec>& specs)
{
  DataProcessorSpec producer{
    "producer",
    Inputs{},
    Outputs{
      {"ITS", "RAWDATA", 0, OutputSpec::Timeframe}
    },
    AlgorithmSpec{
      (AlgorithmSpec::InitCallback) [](InitContext& initContext) {

        std::default_random_engine generator(11);

        return (AlgorithmSpec::ProcessCallback) [generator] (ProcessingContext &processingContext) mutable {

          usleep(100000);
          size_t length = generator() % 10000;

          auto data = processingContext.allocator().make<char>(OutputSpec{ "ITS", "RAWDATA", 0, OutputSpec::Timeframe },
                                                               length);
          for (auto&& item : data) {
            item = static_cast<char>(generator());
          }
        };
      }
    }
  };

  specs.push_back(producer);

  const string qcTaskName = "skeletonTask";
  const std::string qcConfigurationSource = std::string("file://") + getenv("QUALITYCONTROL_ROOT") + "/etc/qcTaskDplConfig.ini";

  TaskDataProcessorFactory qcFactory;
  specs.push_back(qcFactory.create(qcTaskName, qcConfigurationSource));

  DataProcessorSpec checker{
    "checker",
    Inputs{
      {"aaa", "ITS", "HIST_SKLT_TASK", 0, InputSpec::QA}
    },
    Outputs{},
    AlgorithmSpec{
      (AlgorithmSpec::InitCallback) [](InitContext& initContext) {

        return (AlgorithmSpec::ProcessCallback) [] (ProcessingContext &processingContext) mutable {
          LOG(INFO) << "checker invoked";
          auto mo = processingContext.inputs().get<MonitorObject>("aaa");

          if (mo->getName() == "example") {
            auto *g = dynamic_cast<TH1F *>(mo->getObject());
            std::string bins = "BINS:";
            for(int i=0; i < g->GetNbinsX(); i++) {
              bins += " " + std::to_string((int)g->GetBinContent(i));
            }
            LOG(INFO) << bins;
          }

        };
      }
    }
  };
  specs.push_back(checker);

  LOG(INFO) << "Using config file '" << qcConfigurationSource << "'";
  o2::framework::DataSampling::GenerateInfrastructure(specs, qcConfigurationSource);
}