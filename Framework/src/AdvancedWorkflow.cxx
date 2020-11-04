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
/// \file   AdvancedWorkflow.cxx
/// \author Piotr Konopka
/// \author Barthelemy von Haller
///

#include "QualityControl/AdvancedWorkflow.h"

#include <Framework/CompletionPolicyHelpers.h>
#include <Framework/DataSpecUtils.h>
#include "QualityControl/QcInfoLogger.h"
#include <random>

using namespace o2;
using namespace o2::header;
using namespace o2::framework;
using SubSpecificationType = o2::header::DataHeader::SubSpecificationType;
using SubSpec = o2::header::DataHeader::SubSpecificationType;

namespace o2::quality_control::core
{

// clang-format off
WorkflowSpec getProcessingTopology(SubSpecificationType subspec)
{
  DataProcessorSpec source{
    "source-" + std::to_string(subspec),
    Inputs{},
    Outputs{{ "TST", "DATA",  subspec },
            { "TST", "PARAM", subspec }},
    AlgorithmSpec{
      (AlgorithmSpec::ProcessCallback)
      [generator = std::default_random_engine{ static_cast<unsigned int>(time(nullptr)) }, subspec](ProcessingContext & ctx) mutable {
        usleep(200000);
        auto data = ctx.outputs().make<int>(Output{ "TST", "DATA", subspec }, generator() % 10000);
        for (auto&& item : data) {
          item = static_cast<int>(generator());
        }
        ctx.outputs().make<double>(Output{ "TST", "PARAM", subspec }, 1)[0] = 1 / static_cast<double>(1 + generator());
      }
    }
  };

  DataProcessorSpec step{
    "step-" + std::to_string(subspec),
    Inputs{{ "data", "TST", "DATA", subspec }},
    Outputs{{ "TST", "SUM", subspec }},
    AlgorithmSpec{
      (AlgorithmSpec::ProcessCallback)[subspec](ProcessingContext & ctx) {
        auto data = DataRefUtils::as<int>(ctx.inputs().get("data"));
        long long sum = 0;
        for (auto d : data) { sum += d; }
        ctx.outputs().snapshot(Output{ "TST", "SUM", subspec }, sum);
      }
    }
  };

  DataProcessorSpec sink{
    "sink-" + std::to_string(subspec),
    Inputs{{ "sum",   "TST", "SUM",   subspec },
           { "param", "TST", "PARAM", subspec }},
    Outputs{},
    AlgorithmSpec{
      (AlgorithmSpec::ProcessCallback)[](ProcessingContext & ctx) {
        ILOG(Debug, Devel) << "Sum is: " << DataRefUtils::as<long long>(ctx.inputs().get("sum"))[0] << ENDM;
        ILOG(Debug, Devel) << "Param is: " << DataRefUtils::as<double>(ctx.inputs().get("param"))[0] << ENDM;
      }
    }
  };

  return { source, step, sink };
}
// clang-format on

WorkflowSpec getFullProcessingTopology()
{
  WorkflowSpec specs;
  // here we pretend to spawn topologies on three processing machines
  for (int i = 1; i < 4; i++) {
    auto localTopology = getProcessingTopology(i);
    specs.insert(std::end(specs), std::begin(localTopology), std::end(localTopology));
  }
  return specs;
}

} // namespace o2::quality_control::core
