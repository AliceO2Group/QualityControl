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

#include "getTestDataDirectory.h"
#include "QualityControl/CheckRunner.h"
#include "QualityControl/InfrastructureGenerator.h"
#include "QualityControl/runnerUtils.h"
#include <Framework/runDataProcessing.h>
#include <Framework/ControlService.h>
#include <Framework/DataSampling.h>
#include <TH1F.h>

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
        pctx.outputs().make<int>(OutputRef{ "tst-data" }, 1);
      } }
  };
  specs.push_back(producer);

  const std::string qcConfigurationSource = std::string("json://") + getTestDataDirectory() + "testWorkflow.json";

  LOG(INFO) << "Using config file '" << qcConfigurationSource << "'";

  // Generation of Data Sampling infrastructure
  DataSampling::GenerateInfrastructure(specs, qcConfigurationSource);

  // Generation of the QC topology (one task, one checker in this case)
  quality_control::generateRemoteInfrastructure(specs, qcConfigurationSource);

  // Finally the receiver
  DataProcessorSpec receiver{
    "receiver",
    Inputs{
      { "checked-mo", "QC", CheckRunner::createCheckRunnerDataDescription(getFirstCheckerName(qcConfigurationSource)), 0 } },
    Outputs{},
    AlgorithmSpec{
      [](ProcessingContext& pctx) {
        // If any message reaches this point, the QC workflow should work at least on a basic level.

        auto qo = pctx.inputs().get<QualityObject*>("checked-mo");
        if (!qo) {
          LOG(ERROR) << "Quality Object is a NULL";
          pctx.services().get<ControlService>().readyToQuit(true);
          return;
        }

        LOG(DEBUG) << qo->getName() << " - qualit: "<< qo->getQuality();

        // We ask to shut the topology down, returning 0 if there were no ERROR logs.
        pctx.services().get<ControlService>().readyToQuit(true);
      } }
  };
  specs.push_back(receiver);

  return specs;
}
