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
/// \file   InfrastructureGenerator.cxx
/// \author Piotr Konopka
///

#include "QualityControl/InfrastructureGenerator.h"

#include "QualityControl/TaskRunner.h"
#include "QualityControl/TaskRunnerFactory.h"
#include "QualityControl/AggregatorRunnerFactory.h"
#include "QualityControl/CheckRunner.h"
#include "QualityControl/Check.h"
#include "QualityControl/CheckRunnerFactory.h"
#include "QualityControl/PostProcessingDevice.h"
#include "QualityControl/Version.h"
#include "QualityControl/QcInfoLogger.h"

#include <Configuration/ConfigurationFactory.h>
#include <Framework/DataSpecUtils.h>
#include <Framework/ExternalFairMQDeviceProxy.h>
#include <Framework/DataDescriptorQueryBuilder.h>
#include <Mergers/MergerInfrastructureBuilder.h>
#include <Mergers/MergerBuilder.h>
#include <DataSampling/DataSampling.h>

#include <algorithm>

using namespace o2::framework;
using namespace o2::configuration;
using namespace o2::mergers;
using namespace o2::utilities;
using namespace o2::quality_control::checker;
using namespace o2::quality_control::postprocessing;
using boost::property_tree::ptree;
using SubSpec = o2::header::DataHeader::SubSpecificationType;

namespace o2::quality_control::core
{

const char* defaultRemotePort = "36543";

framework::WorkflowSpec InfrastructureGenerator::generateStandaloneInfrastructure(std::string configurationSource)
{
  WorkflowSpec workflow;
  auto config = ConfigurationFactory::getConfiguration(configurationSource);
  printVersion();

  TaskRunnerFactory taskRunnerFactory;
  if (config->getRecursive("qc").count("tasks")) {
    for (const auto& [taskName, taskConfig] : config->getRecursive("qc.tasks")) {
      if (taskConfig.get<bool>("active", true)) {
        workflow.emplace_back(taskRunnerFactory.create(taskName, configurationSource, 0));
      }
    }
  }
  auto checkRunnerOutputs = generateCheckRunners(workflow, configurationSource);
  generateAggregator(workflow, configurationSource, checkRunnerOutputs);
  generatePostProcessing(workflow, configurationSource);

  return workflow;
}

void InfrastructureGenerator::generateStandaloneInfrastructure(framework::WorkflowSpec& workflow, std::string configurationSource)
{
  auto qcInfrastructure = InfrastructureGenerator::generateStandaloneInfrastructure(configurationSource);
  workflow.insert(std::end(workflow), std::begin(qcInfrastructure), std::end(qcInfrastructure));
}

WorkflowSpec InfrastructureGenerator::generateLocalInfrastructure(std::string configurationSource, std::string host)
{
  WorkflowSpec workflow;
  TaskRunnerFactory taskRunnerFactory;
  std::unordered_set<std::string> samplingPoliciesUsed;
  auto config = ConfigurationFactory::getConfiguration(configurationSource);
  printVersion();

  if (config->getRecursive("qc").count("tasks") == 0) {
    return workflow;
  }

  for (const auto& [taskName, taskConfig] : config->getRecursive("qc.tasks")) {
    if (taskConfig.get<bool>("active")) {
      if (taskConfig.get<std::string>("location") == "local") {

        if (taskConfig.get_child("localMachines").empty()) {
          throw std::runtime_error("No local machines specified for task " + taskName + " in its configuration");
        }

        size_t id = 1;
        for (const auto& machine : taskConfig.get_child("localMachines")) {
          // We spawn a task and proxy only if we are on the right machine.
          if (machine.second.get<std::string>("") == host) {
            // Generate QC Task Runner
            bool needsResetAfterCycle = taskConfig.get<std::string>("mergingMode", "delta") == "delta";
            workflow.emplace_back(taskRunnerFactory.create(taskName, configurationSource, id, needsResetAfterCycle));
            // Generate an output proxy
            // These should be removed when we are able to declare dangling output in normal DPL devices
            auto remoteMachine = taskConfig.get_optional<std::string>("remoteMachine");
            if (!remoteMachine.has_value()) {
              ILOG(Warning, Devel)
                << "No remote machine was specified for a multinode QC setup."
                   " This is fine if running with AliECS, but it will fail in standalone mode."
                << ENDM;
            }
            auto remotePort = taskConfig.get_optional<std::string>("remotePort");
            if (!remotePort.has_value()) {
              ILOG(Warning, Devel)
                << "No remote port was specified for a multinode QC setup."
                   " This is fine if running with AliECS, but it might fail in standalone mode."
                << ENDM;
            }
            generateLocalTaskLocalProxy(workflow, id, taskName, remoteMachine.value_or("any"), remotePort.value_or(defaultRemotePort));
            break;
          }
          id++;
        }
      } else // if (taskConfig.get<std::string>("location") == "remote")
      {
        // Collecting Data Sampling Policies
        auto dataSourceTree = taskConfig.get_child("dataSource");
        std::string type = dataSourceTree.get<std::string>("type");
        if (type == "dataSamplingPolicy") {
          samplingPoliciesUsed.insert(dataSourceTree.get<std::string>("name"));
        } else if (type == "direct") {
          throw std::runtime_error("Configuration error: Remote QC tasks such as " + taskName + " cannot use direct data sources");
        } else {
          throw std::runtime_error("Configuration error: dataSource type unknown : " + type);
        }
      }
    }
  }

  // Creating Data Sampling Policies proxies
  for (const auto& policyName : samplingPoliciesUsed) {
    std::string port = std::to_string(DataSampling::PortForPolicy(config.get(), policyName));
    Inputs inputSpecs = DataSampling::InputSpecsForPolicy(config.get(), policyName);

    std::vector<std::string> machines = DataSampling::MachinesForPolicy(config.get(), policyName);
    for (const auto& machine : machines) {
      if (machine == host) {
        generateDataSamplingPolicyLocalProxy(workflow, policyName, inputSpecs, machine, port);
      }
    }
  }

  return workflow;
}

void InfrastructureGenerator::generateLocalInfrastructure(framework::WorkflowSpec& workflow, std::string configurationSource, std::string host)
{
  auto qcInfrastructure = InfrastructureGenerator::generateLocalInfrastructure(configurationSource, host);
  workflow.insert(std::end(workflow), std::begin(qcInfrastructure), std::end(qcInfrastructure));
}

o2::framework::WorkflowSpec InfrastructureGenerator::generateRemoteInfrastructure(std::string configurationSource)
{
  WorkflowSpec workflow;
  std::unordered_set<std::string> samplingPoliciesUsed;
  auto config = ConfigurationFactory::getConfiguration(configurationSource);
  printVersion();

  if (config->getRecursive("qc").count("tasks")) {
    TaskRunnerFactory taskRunnerFactory;
    for (const auto& [taskName, taskConfig] : config->getRecursive("qc.tasks")) {
      if (taskConfig.get<bool>("active", true)) {

        if (taskConfig.get<std::string>("location") == "local") {
          // if tasks are LOCAL, generate input proxies + mergers + checkers

          size_t numberOfLocalMachines = taskConfig.get_child("localMachines").size() > 1 ? taskConfig.get_child("localMachines").size() : 1;
          // Generate an input proxy
          // These should be removed when we are able to declare dangling inputs in normal DPL devices
          auto remotePort = taskConfig.get_optional<std::string>("remotePort");
          if (!remotePort.has_value()) {
            ILOG(Warning, Devel) << "No remote port was specified for a multinode QC setup."
                                    " This is fine if running with AliECS, but it might fail in standalone mode."
                                 << ENDM;
          }
          generateLocalTaskRemoteProxy(workflow, taskName, numberOfLocalMachines, remotePort.value_or(defaultRemotePort));

          generateMergers(workflow, taskName, numberOfLocalMachines,
                          taskConfig.get<double>("cycleDurationSeconds"),
                          taskConfig.get<std::string>("mergingMode", "delta"));

        } else if (taskConfig.get<std::string>("location") == "remote") {

          // -- if tasks are REMOTE, generate dispatcher proxies + tasks + checkers
          // (for the time being we don't foresee parallel tasks on QC servers, so no mergers here)

          // fixme: ideally we should check if we are on the right remote machine, but now we support only n -> 1 setups,
          //  so there is no point. Also, I expect that we should be able to generate one big topology or its parts
          //  and we would place it among QC servers using AliECS, not by configuration files.

          // Collecting Data Sampling Policies
          auto dataSourceTree = taskConfig.get_child("dataSource");
          std::string type = dataSourceTree.get<std::string>("type");
          if (type == "dataSamplingPolicy") {
            samplingPoliciesUsed.insert(dataSourceTree.get<std::string>("name"));
          } else if (type == "direct") {
            throw std::runtime_error("Configuration error: Remote QC tasks such as " + taskName + " cannot use direct data sources");
          } else {
            throw std::runtime_error("Configuration error: dataSource type unknown : " + type);
          }

          // Creating the remote task
          workflow.emplace_back(taskRunnerFactory.create(taskName, configurationSource, 0));
        }
      }
    }
  }

  // Creating Data Sampling Policies proxies
  for (const auto& policyName : samplingPoliciesUsed) {
    // todo now we have to generate one proxy per local machine and policy, because of the proxy limitations.
    //  Use one proxy per policy when it is possible.
    std::string port = std::to_string(DataSampling::PortForPolicy(config.get(), policyName));
    Outputs outputSpecs = DataSampling::OutputSpecsForPolicy(config.get(), policyName);
    std::vector<std::string> machines = DataSampling::MachinesForPolicy(config.get(), policyName);
    for (const auto& machine : machines) {
      generateDataSamplingPolicyRemoteProxy(workflow, policyName, outputSpecs, machine, port);
    }
  }

  generateCheckRunners(workflow, configurationSource);
  generatePostProcessing(workflow, configurationSource);

  return workflow;
}

void InfrastructureGenerator::generateRemoteInfrastructure(framework::WorkflowSpec& workflow, std::string configurationSource)
{
  auto qcInfrastructure = InfrastructureGenerator::generateRemoteInfrastructure(configurationSource);
  workflow.insert(std::end(workflow), std::begin(qcInfrastructure), std::end(qcInfrastructure));
}

void InfrastructureGenerator::customizeInfrastructure(std::vector<framework::CompletionPolicy>& policies)
{
  TaskRunnerFactory::customizeInfrastructure(policies);
  MergerBuilder::customizeInfrastructure(policies);
  CheckRunnerFactory::customizeInfrastructure(policies);
  AggregatorRunnerFactory::customizeInfrastructure(policies);
}

void InfrastructureGenerator::printVersion()
{
  // Log the version number
  ILOG(Info, Support) << "QC version " << o2::quality_control::core::Version::GetQcVersion().getString() << ENDM;
}

void InfrastructureGenerator::generateDataSamplingPolicyLocalProxy(framework::WorkflowSpec& workflow,
                                                                   const string& policyName,
                                                                   const framework::Inputs& inputSpecs,
                                                                   const std::string& localMachine,
                                                                   const string& localPort)
{
  std::string proxyName = policyName + "-proxy";
  std::string channelName = policyName + "-" + localMachine;
  std::string channelConfig = "name=" + channelName + ",type=push,method=bind,address=tcp://*:" + localPort +
                              ",rateLogging=60,transport=zeromq";
  auto channelSelector = [channelName](InputSpec const&, const std::unordered_map<std::string, std::vector<FairMQChannel>>&) {
    return channelName;
  };

  workflow.emplace_back(
    specifyFairMQDeviceMultiOutputProxy(
      proxyName.c_str(),
      inputSpecs,
      channelConfig.c_str(),
      channelSelector));
}

void InfrastructureGenerator::generateDataSamplingPolicyRemoteProxy(framework::WorkflowSpec& workflow,
                                                                    const std::string& policyName,
                                                                    const Outputs& outputSpecs,
                                                                    const std::string& localMachine,
                                                                    const std::string& localPort)
{
  std::string channelName = policyName + "-" + localMachine;
  const std::string& proxyName = channelName; // channel name has to match proxy name

  std::string channelConfig = "name=" + channelName + ",type=pull,method=connect,address=tcp://" +
                              localMachine + ":" + localPort + ",rateLogging=60,transport=zeromq";

  workflow.emplace_back(specifyExternalFairMQDeviceProxy(
    proxyName.c_str(),
    outputSpecs,
    channelConfig.c_str(),
    dplModelAdaptor()));
}

void InfrastructureGenerator::generateLocalTaskLocalProxy(framework::WorkflowSpec& workflow, size_t id,
                                                          std::string taskName, std::string remoteHost,
                                                          std::string remotePort)
{
  std::string proxyName = taskName + "-proxy-" + std::to_string(id);
  std::string channelName = taskName + "-proxy";
  InputSpec proxyInput{ channelName, TaskRunner::createTaskDataOrigin(), TaskRunner::createTaskDataDescription(taskName), static_cast<SubSpec>(id) };
  std::string channelConfig = "name=" + channelName + ",type=push,method=connect,address=tcp://" +
                              remoteHost + ":" + remotePort + ",rateLogging=60,transport=zeromq";

  workflow.emplace_back(
    specifyFairMQDeviceMultiOutputProxy(
      proxyName.c_str(),
      { proxyInput },
      channelConfig.c_str()));
}

void InfrastructureGenerator::generateLocalTaskRemoteProxy(framework::WorkflowSpec& workflow, std::string taskName,
                                                           size_t numberOfLocalMachines, std::string remotePort)
{
  std::string proxyName = taskName + "-proxy"; // channel name has to match proxy name
  std::string channelName = taskName + "-proxy";

  Outputs proxyOutputs;
  for (size_t id = 1; id <= numberOfLocalMachines; id++) {
    proxyOutputs.emplace_back(
      OutputSpec{ { channelName }, TaskRunner::createTaskDataOrigin(), TaskRunner::createTaskDataDescription(taskName), static_cast<SubSpec>(id) });
  }

  std::string channelConfig = "name=" + channelName + ",type=pull,method=bind,address=tcp://*:" + remotePort +
                              ",rateLogging=60,transport=zeromq";

  workflow.emplace_back(specifyExternalFairMQDeviceProxy(
    proxyName.c_str(),
    proxyOutputs,
    channelConfig.c_str(),
    dplModelAdaptor()));
}

void InfrastructureGenerator::generateMergers(framework::WorkflowSpec& workflow, std::string taskName,
                                              size_t numberOfLocalMachines, double cycleDurationSeconds,
                                              std::string mergingMode)
{
  Inputs mergerInputs;
  for (size_t id = 1; id <= numberOfLocalMachines; id++) {
    mergerInputs.emplace_back(
      InputSpec{ { taskName + std::to_string(id) },
                 TaskRunner::createTaskDataOrigin(),
                 TaskRunner::createTaskDataDescription(taskName),
                 static_cast<SubSpec>(id) });
  }

  MergerInfrastructureBuilder mergersBuilder;
  mergersBuilder.setInfrastructureName(taskName);
  mergersBuilder.setInputSpecs(mergerInputs);
  mergersBuilder.setOutputSpec(
    { { "main" }, TaskRunner::createTaskDataOrigin(), TaskRunner::createTaskDataDescription(taskName), 0 });
  MergerConfig mergerConfig;
  // if we are to change the mode to Full, disable reseting tasks after each cycle.
  mergerConfig.inputObjectTimespan = { (mergingMode.empty() || mergingMode == "delta") ? InputObjectsTimespan::LastDifference : InputObjectsTimespan::FullHistory };
  mergerConfig.publicationDecision = { PublicationDecision::EachNSeconds, cycleDurationSeconds };
  mergerConfig.mergedObjectTimespan = { MergedObjectTimespan::FullHistory, 0 };
  // for now one merger should be enough, multiple layers to be supported later
  mergerConfig.topologySize = { TopologySize::NumberOfLayers, 1 };
  mergersBuilder.setConfig(mergerConfig);

  mergersBuilder.generateInfrastructure(workflow);
}

vector<OutputSpec> InfrastructureGenerator::generateCheckRunners(framework::WorkflowSpec& workflow, std::string configurationSource)
{
  // todo have a look if this complex procedure can be simplified.
  // todo also make well defined and scoped functions to make it more readable and clearer.
  typedef std::vector<std::string> InputNames;
  typedef std::vector<Check> Checks;
  std::map<std::string, o2::framework::InputSpec> tasksOutputMap; // all active tasks' output, as inputs, keyed by their label
  std::map<InputNames, Checks> checksMap;                         // all the Checks defined in the config mapped keyed by their sorted inputNames
  std::map<InputNames, InputNames> storeVectorMap;

  auto config = ConfigurationFactory::getConfiguration(configurationSource);

  if (config->getRecursive("qc").count("tasks")) {
    for (const auto& [taskName, taskConfig] : config->getRecursive("qc.tasks")) {
      if (taskConfig.get<bool>("active", true)) {
        InputSpec taskOutput{ taskName, TaskRunner::createTaskDataOrigin(), TaskRunner::createTaskDataDescription(taskName) };
        tasksOutputMap.insert({ DataSpecUtils::label(taskOutput), taskOutput });
      }
    }
  }

  if (config->getRecursive("qc").count("postprocessing")) {
    for (const auto& [ppTaskName, ppTaskConfig] : config->getRecursive("qc.postprocessing")) {
      if (ppTaskConfig.get<bool>("active", true)) {
        InputSpec taskOutput{ ppTaskName, PostProcessingDevice::createPostProcessingDataOrigin(), PostProcessingDevice::createPostProcessingDataDescription(ppTaskName) };
        tasksOutputMap.insert({ DataSpecUtils::label(taskOutput), taskOutput });
      }
    }
  }

  // For each external task prepare the InputSpec to be stored in tasksoutputMap
  if (config->getRecursive("qc").count("externalTasks")) {
    for (const auto& [taskName, taskConfig] : config->getRecursive("qc.externalTasks")) {
      (void)taskName;
      if (taskConfig.get<bool>("active", true)) {
        auto query = taskConfig.get<std::string>("query");
        Inputs inputs = DataDescriptorQueryBuilder::parse(query.c_str());
        for (const auto& taskOutput : inputs) {
          tasksOutputMap.insert({ DataSpecUtils::label(taskOutput), taskOutput });
        }
      }
    }
  }

  // Instantiate Checks based on the configuration and build a map of checks (keyed by their inputs names)
  if (config->getRecursive("qc").count("checks")) {
    // For every check definition, create a Check
    for (const auto& [checkName, checkConfig] : config->getRecursive("qc.checks")) {
      ILOG(Debug, Devel) << ">> Check name : " << checkName << ENDM;
      if (checkConfig.get<bool>("active", true)) {
        auto check = Check(checkName, configurationSource);
        InputNames inputNames;

        for (auto& inputSpec : check.getInputs()) {
          auto name = DataSpecUtils::label(inputSpec);
          inputNames.push_back(name);
        }
        // Create a grouping key - sorted vector of stringified InputSpecs
        std::sort(inputNames.begin(), inputNames.end());
        // Group checks
        checksMap[inputNames].push_back(check);
      }
    }
  }

  // For every Task output, find a Check to store the MOs in the database.
  // If none is found we create a sink device.
  for (auto& [label, inputSpec] : tasksOutputMap) { // for each task output
    (void)inputSpec;
    bool isStored = false;
    // Look for this task as input in the Checks' inputs, if we found it then we are done
    for (auto& [inputNames, checks] : checksMap) { // for each set of inputs
      (void)checks;
      if (std::find(inputNames.begin(), inputNames.end(), label) != inputNames.end() && inputNames.size() == 1) {
        storeVectorMap[inputNames].push_back(label);
        break;
      }
    }
    if (!isStored) {
      // If there is no Check for a given input, create a candidate for a sink device
      InputNames singleEntry{ label };
      // Init empty Check vector to appear in the next step
      checksMap[singleEntry];
      storeVectorMap[singleEntry].push_back(label);
    }
  }

  // Create CheckRunners: 1 per set of inputs
  CheckRunnerFactory checkRunnerFactory;
  vector<framework::OutputSpec> checkRunnerOutputs; // needed later for the aggregators
  for (auto& [inputNames, checks] : checksMap) {
    //Logging
    ILOG(Info, Devel) << ">> Inputs (" << inputNames.size() << "): ";
    for (const auto& name : inputNames)
      ILOG(Info, Devel) << name << " ";
    ILOG(Info, Devel) << " ; Checks (" << checks.size() << "): ";
    for (auto& check : checks)
      ILOG(Info, Devel) << check.getName() << " ";
    ILOG(Info, Devel) << " ; Stores (" << storeVectorMap[inputNames].size() << "): ";
    for (auto& input : storeVectorMap[inputNames])
      ILOG(Info, Devel) << input << " ";
    ILOG(Info, Devel) << ENDM;

    if (!checks.empty()) { // Create a CheckRunner for the grouped checks
      DataProcessorSpec spec = checkRunnerFactory.create(checks, configurationSource, storeVectorMap[inputNames]);
      workflow.emplace_back(spec);
      checkRunnerOutputs.insert(checkRunnerOutputs.end(), spec.outputs.begin(), spec.outputs.end());
    } else { // If there are no checks, create a sink CheckRunner
      DataProcessorSpec spec = checkRunnerFactory.createSinkDevice(tasksOutputMap.find(inputNames[0])->second, configurationSource);
      workflow.emplace_back(spec);
      checkRunnerOutputs.insert(checkRunnerOutputs.end(), spec.outputs.begin(), spec.outputs.end());
    }
  }

  ILOG(Info) << ">> Outputs (" << checkRunnerOutputs.size() << "): ";
  for (const auto& output : checkRunnerOutputs)
    ILOG(Info) << DataSpecUtils::describe(output) << " ";
  ILOG(Info) << ENDM;

  return checkRunnerOutputs;
}

void InfrastructureGenerator::generateAggregator(WorkflowSpec& workflow, std::string configurationSource, vector<framework::OutputSpec>& checkRunnerOutputs)
{
  // TODO consider whether we should recompute checkRunnerOutputs instead of receiving it all baked.
  auto config = ConfigurationFactory::getConfiguration(configurationSource);
  if (config->getRecursive("qc").count("aggregators") == 0) {
    ILOG(Debug, Devel) << "No \"aggregators\" structure found in the config file. If no quality aggregation is expected, then it is completely fine." << ENDM;
    return;
  }
  DataProcessorSpec spec = AggregatorRunnerFactory::create(checkRunnerOutputs, configurationSource);
  workflow.emplace_back(spec);
}

void InfrastructureGenerator::generatePostProcessing(WorkflowSpec& workflow, std::string configurationSource)
{
  auto config = ConfigurationFactory::getConfiguration(configurationSource);
  if (config->getRecursive("qc").count("postprocessing") == 0) {
    ILOG(Debug, Devel) << "No \"postprocessing\" structure found in the config file. If no postprocessing is expected, then it is completely fine." << ENDM;
    return;
  }
  for (const auto& [ppTaskName, ppTaskConfig] : config->getRecursive("qc.postprocessing")) {
    if (ppTaskConfig.get<bool>("active", true)) {

      PostProcessingDevice ppTask{ ppTaskName, configurationSource };

      DataProcessorSpec ppTaskSpec{
        ppTask.getDeviceName(),
        ppTask.getInputsSpecs(),
        ppTask.getOutputSpecs(),
        {},
        ppTask.getOptions()
      };

      ppTaskSpec.algorithm = adaptFromTask<PostProcessingDevice>(std::move(ppTask));

      workflow.emplace_back(std::move(ppTaskSpec));
    }
  }
}

} // namespace o2::quality_control::core
