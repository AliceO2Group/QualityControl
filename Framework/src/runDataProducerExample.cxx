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
/// \file    runDataProducerExample.cxx
/// \author Barthelemy von Haller
///
/// \brief This is just an example of very basic data producer in Data Processing Layer.
/// It produces a fixed number on TST/RAWDATA/0
///

#include <vector>
#include <Framework/ConfigParamSpec.h>

using namespace o2;
using namespace o2::framework;

// We add custom arguments to the executable by implementing a customize function.
void customize(std::vector<ConfigParamSpec>& workflowOptions)
{
  workflowOptions.push_back(
    ConfigParamSpec{ "my-param", VariantType::Int, 1, { "Example parameter." } });
}

#include <Framework/runDataProcessing.h>
#include "QualityControl/DataProducerExample.h"

using namespace o2::quality_control::core;

// Here we define all Data Processors which should be run by DPL (only the data producer in this case).
WorkflowSpec defineDataProcessing(const ConfigContext& config)
{
  size_t myParam = config.options().get<int>("my-param");

  WorkflowSpec specs;
  specs.push_back(getDataProducerExampleSpec(myParam));
  return specs;
}
