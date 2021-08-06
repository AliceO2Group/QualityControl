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
/// \file    runTPCQCPID.cxx
/// \author  Jens Wiechula
///
/// \brief run TPC PID QC task, reading tracks from file for the moment
///

// IMPORTANT:
// for some reason, if the 'customize' functions are below the other includes,
// the options are not added to the workflow.
// So the structure should be kept as it is
#include <DataSampling/DataSampling.h>
#include "QualityControl/InfrastructureGenerator.h"

using namespace o2;
using namespace o2::framework;
using namespace o2::utilities;

void customize(std::vector<CompletionPolicy>& policies)
{
  DataSampling::CustomizeInfrastructure(policies);
  quality_control::customizeInfrastructure(policies);
}

void customize(std::vector<ChannelConfigurationPolicy>& policies)
{
  DataSampling::CustomizeInfrastructure(policies);
}

void customize(std::vector<ConfigParamSpec>& workflowOptions)
{
  workflowOptions.push_back(ConfigParamSpec{ "input-file", VariantType::String, "tpctracks.root", { "Input file name for TPC tracks" } });
  workflowOptions.push_back(ConfigParamSpec{ "tree-name", VariantType::String, "tpcrec", { "Name of the tree containing the TPC tracks vector" } });
  workflowOptions.push_back(ConfigParamSpec{ "branch-name", VariantType::String, "TPCTracks", { "Name of the branch of the TPC tracks vector" } });
}

// c++ includes
#include <memory>
#include <random>

// ROOT includes
#include <TH1F.h>

// o2 included
#include "Framework/runDataProcessing.h"
#include "DPLUtils/RootTreeReader.h"

// qc includes
#include "QualityControl/CheckRunner.h"
#include "QualityControl/runnerUtils.h"

WorkflowSpec defineDataProcessing(const ConfigContext& config)
{
  WorkflowSpec specs;

  // ===| workflow options |====================================================
  //
  auto inputFile = config.options().get<std::string>("input-file");
  auto treeName = config.options().get<std::string>("tree-name");
  auto branchName = config.options().get<std::string>("branch-name");

  // ===| tree reader |=========================================================
  //
  // The tree reader will read tpc tracks from file crated via the o2 sim/rec
  // workflow
  DataProcessorSpec producer{
    "tpc-track-reader",
    Inputs{},
    Outputs{
      { "TPC", "TRACKS", 0, Lifetime::Timeframe } },
    AlgorithmSpec{
      (AlgorithmSpec::InitCallback)[inputFile, treeName, branchName](InitContext&){

        // root tree reader
        //constexpr auto persistency = Lifetime::Transient;
        constexpr auto persistency = Lifetime::Timeframe;

  auto reader = std::make_shared<RootTreeReader>(treeName.data(),                      // tree name
                                                 inputFile.data(),                     // input file name
                                                 RootTreeReader::PublishingMode::Loop, // loop over
                                                 Output{ "TPC", "TRACKS", 0, persistency },
                                                 branchName.data() // name of the branch
  );

  return (AlgorithmSpec::ProcessCallback)[reader](ProcessingContext & processingContext) mutable
  {
    //(++(*reader))(processingContext);
    if (reader->next()) {
      (*reader)(processingContext);
      //LOG(INFO) << "Call producer AlgorithmSpec::ProcessCallback:  has data " << reader->getCount();
    } else {
      //LOG(INFO) << "Call producer AlgorithmSpec::ProcessCallback:  no next data" << reader->getCount();
    }
  };
}
}
}
;

specs.push_back(producer);

return specs;
}
