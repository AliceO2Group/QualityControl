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
/// \file    testWorkflow.cxx
/// \author  Piotr Konopka
///

#include "QualityControl/Checker.h"
#include "QualityControl/InfrastructureGenerator.h"
#include "../src/runnerUtils.h"
#include <Framework/runDataProcessing.h>
#include <Framework/ControlService.h>
#include <Framework/DataSampling.h>

using namespace o2;
using namespace o2::framework;
using namespace o2::quality_control::core;
using namespace o2::quality_control::checker;

WorkflowSpec defineDataProcessing(ConfigContext const&)
{
  WorkflowSpec specs;

  // The producer to generate some data in the workflow
  DataProcessorSpec producer{
    "producer",
    Inputs{},
    Outputs{
      { { "tst-data" }, "TST", "DATA" } },
    AlgorithmSpec{
      [](ProcessingContext& pctx) {
        usleep(100000);
        auto data = pctx.outputs().make<int>(OutputRef{ "tst-data" }, 1);
      } }
  };
  specs.push_back(producer);

  const std::string qcConfigurationSource = std::string("json://") + getenv("QUALITYCONTROL_ROOT") + "/tests/testWorkflow.json";
  LOG(INFO) << "Using config file '" << qcConfigurationSource << "'";

  // Generation of Data Sampling infrastructure
  DataSampling::GenerateInfrastructure(specs, qcConfigurationSource);

  // Generation of the QC topology (one task, one checker in this case)
  quality_control::generateRemoteInfrastructure(specs, qcConfigurationSource);

  // Finally the receiver
  DataProcessorSpec receiver{
    "receiver",
    Inputs{
      { "checked-mo", "QC", Checker::createCheckerDataDescription(getFirstTaskName(qcConfigurationSource)), 0 } },
    Outputs{},
    AlgorithmSpec{
      [](ProcessingContext& pctx) {
        // If a message reaches this point, the QC workflow works, at least in a basic level.
        // We ask to shut the topology down, returning 0.
        pctx.services().get<ControlService>().readyToQuit(true);
      } }
  };
  specs.push_back(receiver);

  return specs;
}
