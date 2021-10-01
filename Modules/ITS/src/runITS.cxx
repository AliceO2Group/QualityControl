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
/// \file   runITS.cxx
/// \author Zhaozhong Shi
/// \author Markus Keil
//  \author Mario Sitta
//  \author Jian Liu
//  \author Li'Ang Zhang
/// \brief This is an executable to run the ITS QC Task.
///

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
  workflowOptions.push_back(
    ConfigParamSpec{ "config-path", VariantType::String, "", { "Path to the config file. Overwrite the default paths. Do not use with no-data-sampling." } });
  workflowOptions.push_back(
    ConfigParamSpec{ "no-data-sampling", VariantType::Bool, false, { "Skips data sampling, connects directly the task to the producer." } });
}

#include <Framework/Logger.h>
#include <TH1F.h>
#include <memory>
#include <random>

#include <Framework/runDataProcessing.h>

#include <Configuration/ConfigurationFactory.h>
#include <Configuration/ConfigurationInterface.h>
#include "QualityControl/Check.h"
#include "QualityControl/InfrastructureGenerator.h"
#include "QualityControl/runnerUtils.h"
#include "QualityControl/ExamplePrinterSpec.h"
#include "ITSQCDataReaderWorkflow/TestDataReader.h"
#include "DetectorsBase/GeometryManager.h"

std::string getConfigPath(const ConfigContext& config);

using namespace o2::configuration;
using namespace o2::framework;
using namespace o2::quality_control::checker;
using namespace std::chrono;

WorkflowSpec defineDataProcessing(const ConfigContext& config)
{
  WorkflowSpec specs;

  o2::base::GeometryManager::loadGeometry();

  QcInfoLogger::GetInstance() << "START READER" << AliceO2::InfoLogger::InfoLogger::endm;

  specs.emplace_back(o2::its::getTestDataReaderSpec());

  // Path to the config file
  std::string qcConfigurationSource = getConfigPath(config);
  LOG(info) << "Using config file '" << qcConfigurationSource << "'";

  // Generation of Data Sampling infrastructure
  auto configInterface = ConfigurationFactory::getConfiguration(qcConfigurationSource);
  auto dataSamplingTree = configInterface->getRecursive("dataSamplingPolicies");
  DataSampling::GenerateInfrastructure(specs, dataSamplingTree);

  // Generation of the QC topology (one task, one checker in this case)
  quality_control::generateStandaloneInfrastructure(specs, qcConfigurationSource);

  return specs;
}

// TODO merge this with the one from runReadout.cxx
std::string getConfigPath(const ConfigContext& config)
{
  // Determine the default config file path and name (based on option no-data-sampling and the QC_ROOT path)
  bool noDS = config.options().get<bool>("no-data-sampling");
  std::string filename = !noDS ? "its.json" : "basic-no-sampling.json";
  std::string defaultConfigPath = getenv("QUALITYCONTROL_ROOT") != nullptr ? std::string(getenv("QUALITYCONTROL_ROOT")) + "/etc/" + filename : "$QUALITYCONTROL_ROOT undefined";
  // The the optional one by the user
  auto userConfigPath = config.options().get<std::string>("config-path");
  // Finally build the config path based on the default or the user-base one
  std::string path = std::string("json:/") + (userConfigPath.empty() ? defaultConfigPath : userConfigPath);
  return path;
}
