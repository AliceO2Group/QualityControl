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
#include "QualityControl/CheckRunner.h"
#include "QualityControl/Check.h"
#include "QualityControl/CheckRunnerFactory.h"
#include "QualityControl/Version.h"
#include "QualityControl/QcInfoLogger.h"

#include <boost/property_tree/ptree.hpp>
#include <Configuration/ConfigurationFactory.h>
#include <Framework/DataSampling.h>
#include <Framework/DataSpecUtils.h>
#include <Framework/ExternalFairMQDeviceProxy.h>
#include <Mergers/MergerInfrastructureBuilder.h>
#include <Mergers/MergerBuilder.h>
#include <Framework/DataDescriptorQueryBuilder.h>

#include <algorithm>

using namespace o2::framework;
using namespace o2::configuration;
using namespace o2::mergers;
using namespace o2::quality_control::checker;
using boost::property_tree::ptree;
using SubSpec = o2::header::DataHeader::SubSpecificationType;

namespace o2::quality_control::core
{

framework::WorkflowSpec InfrastructureGenerator::generateStandaloneInfrastructure(std::string configurationSource)
{
  WorkflowSpec workflow;
  auto config = ConfigurationFactory::getConfiguration(configurationSource);
  printVersion();

  TaskRunnerFactory taskRunnerFactory;
  for (const auto& [taskName, taskConfig] : config->getRecursive("qc.tasks")) {
    if (taskConfig.get<bool>("active", true)) {
      workflow.emplace_back(taskRunnerFactory.create(taskName, configurationSource, 0));
    }
  }

  generateCheckRunners(workflow, configurationSource);

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

  for (const auto& [taskName, taskConfig] : config->getRecursive("qc.tasks")) {
    if (taskConfig.get<bool>("active")) {
      if (taskConfig.get<std::string>("location") == "local") {

        if (taskConfig.get_child("localMachines").empty()) {
          throw std::runtime_error("No local machines specified for task " + taskName + " in its configuration");
        }

        bool needsMergers = taskConfig.get_child("localMachines").size() > 1;
        size_t id = needsMergers ? 1 : 0;
        for (const auto& machine : taskConfig.get_child("localMachines")) {
          // We spawn a task and proxy only if we are on the right machine.
          if (machine.second.get<std::string>("") == host) {
            // Generate QC Task Runner
            workflow.emplace_back(taskRunnerFactory.create(taskName, configurationSource, id, needsMergers));
            // Generate an output proxy
            // These should be removed when we are able to declare dangling output in normal DPL devices
            generateLocalTaskLocalProxy(workflow, id, taskName, taskConfig.get<std::string>("remoteMachine"), taskConfig.get<std::string>("remotePort"));
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
        generateDataSamplingPolicyLocalProxy(workflow, policyName, inputSpecs, port);
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

  TaskRunnerFactory taskRunnerFactory;
  for (const auto& [taskName, taskConfig] : config->getRecursive("qc.tasks")) {
    if (taskConfig.get<bool>("active", true)) {

      if (taskConfig.get<std::string>("location") == "local") {
        // if tasks are LOCAL, generate input proxies + mergers + checkers

        size_t numberOfLocalMachines = taskConfig.get_child("localMachines").size() > 1 ? taskConfig.get_child("localMachines").size() : 1;

        // Generate an input proxy
        // These should be removed when we are able to declare dangling inputs in normal DPL devices
        generateLocalTaskRemoteProxy(workflow, taskName, numberOfLocalMachines, taskConfig.get<std::string>("remotePort"));

        // Generate a Merger only when there is a need to merge something - there is more than one machine with the QC Task
        // I don't expect the list of machines to be reconfigured - all of them should be declared beforehand,
        // even if some of them will be on standby.
        if (numberOfLocalMachines > 1) {
          generateMergers(workflow, taskName, numberOfLocalMachines, taskConfig.get<double>("cycleDurationSeconds"));
        }

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

  // Creating Data Sampling Policies proxies
  for (const auto& policyName : samplingPoliciesUsed) {
    // todo now we have to generate one proxy per local machine and policy, because of the proxy limitations.
    //  Use one proxy per policy when it is possible.
    std::string port = std::to_string(DataSampling::PortForPolicy(config.get(), policyName));
    Outputs outputSpecs = DataSampling::OutputSpecsForPolicy(config.get(), policyName);
    std::vector<std::string> machines = DataSampling::MachinesForPolicy(config.get(), policyName);
    for (const auto& machine : machines) {
      generateDataSamplingPolicyRemoteProxy(workflow, outputSpecs, machine, port);
    }
  }

  generateCheckRunners(workflow, configurationSource);

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
}

void InfrastructureGenerator::printVersion()
{
  // Log the version number
  QcInfoLogger::GetInstance() << "QC version " << o2::quality_control::core::Version::GetQcVersion().getString() << infologger::endm;
}

void InfrastructureGenerator::generateDataSamplingPolicyLocalProxy(framework::WorkflowSpec& workflow,
                                                                   const string& policyName,
                                                                   const framework::Inputs& inputSpecs,
                                                                   const string& localPort)
{
  std::string proxyName = policyName + "-proxy";
  std::string channelName = inputSpecs.at(0).binding; // channel name has to match binding name.
  std::string channelConfig = "name=" + channelName + ",type=push,method=bind,address=tcp://*:" + localPort +
                              ",rateLogging=60,transport=zeromq";

  workflow.emplace_back(
    specifyFairMQDeviceOutputProxy(
      proxyName.c_str(),
      inputSpecs,
      channelConfig.c_str()));
}

void InfrastructureGenerator::generateDataSamplingPolicyRemoteProxy(framework::WorkflowSpec& workflow,
                                                                    const Outputs& outputSpecs,
                                                                    const std::string& localMachine,
                                                                    const std::string& localPort)
{
  std::string channelName = outputSpecs.at(0).binding.value; // channel name has to match binding name.
  std::string proxyName = channelName;                       // channel name has to match proxy name

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
    specifyFairMQDeviceOutputProxy(
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
  if (numberOfLocalMachines > 1) {
    for (size_t id = 1; id <= numberOfLocalMachines; id++) {
      proxyOutputs.emplace_back(
        OutputSpec{ { channelName }, TaskRunner::createTaskDataOrigin(), TaskRunner::createTaskDataDescription(taskName), static_cast<SubSpec>(id) });
    }
  } else {
    proxyOutputs.emplace_back(
      OutputSpec{ { channelName }, { TaskRunner::createTaskDataOrigin(), TaskRunner::createTaskDataDescription(taskName) } });
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
                                              size_t numberOfLocalMachines, double cycleDurationSeconds)
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
  mergerConfig.inputObjectTimespan = { InputObjectsTimespan::LastDifference, 0 };
  mergerConfig.publicationDecision = {
    PublicationDecision::EachNSeconds, cycleDurationSeconds
  };
  mergerConfig.mergedObjectTimespan = { MergedObjectTimespan::FullHistory, 0 };
  // for now one merger should be enough, multiple layers to be supported later
  mergerConfig.topologySize = { TopologySize::NumberOfLayers, 1 };
  mergersBuilder.setConfig(mergerConfig);

  mergersBuilder.generateInfrastructure(workflow);
}

void InfrastructureGenerator::generateCheckRunners(framework::WorkflowSpec& workflow, std::string configurationSource)
{
  // todo have a look if this complex procedure can be simplified.
  // todo also make well defined and scoped functions to make it more readable and clearer.
  typedef std::vector<std::string> InputNames;
  typedef std::vector<Check> Checks;
  std::map<std::string, o2::framework::InputSpec> tasksOutputMap; // all active tasks' output, as inputs, keyed by their label
  std::map<InputNames, Checks> checksMap;                         // all the Checks defined in the config mapped keyed by their sorted inputNames
  std::map<InputNames, InputNames> storeVectorMap;

  auto config = ConfigurationFactory::getConfiguration(configurationSource);

  // Build tasksOutputMap based on active tasks in the config
  for (const auto& [taskName, taskConfig] : config->getRecursive("qc.tasks")) {
    if (taskConfig.get<bool>("active", true)) {
      o2::framework::InputSpec taskOutput{ taskName, TaskRunner::createTaskDataOrigin(), TaskRunner::createTaskDataDescription(taskName) };
      tasksOutputMap.insert({ DataSpecUtils::label(taskOutput), taskOutput });
    }
  }

  // For each external task prepare the InputSpec to be stored in tasksoutputMap
  for (const auto& [taskName, taskConfig] : config->getRecursive("qc.externalTasks")) {
    if (taskConfig.get<bool>("active", true)) {
      auto query = taskConfig.get<std::string>("query");
      framework::Inputs inputs = o2::framework::DataDescriptorQueryBuilder::parse(query.c_str());
      o2::framework::InputSpec taskOutput = inputs.at(0); // only consider the first one if several.

      string label = DataSpecUtils::label(taskOutput);
      tasksOutputMap.insert({ label, taskOutput });
    }
  }

  // Instantiate Checks based on the configuration and build a map of checks (keyed by their inputs names)
  if (config->getRecursive("qc").count("checks")) {
    // For every check definition, create a Check
    for (const auto& [checkName, checkConfig] : config->getRecursive("qc.checks")) {
      QcInfoLogger::GetInstance() << ">> Check name : " << checkName << AliceO2::InfoLogger::InfoLogger::endm;
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
    (void)inputSpec; // avoid warning
    // Look for this task as input in the Checks' inputs, if we found it then we are done
    for (auto& [inputNames, checks] : checksMap) { // for each set of inputs
      (void)checks; // avoid warning
      if (std::find(inputNames.begin(), inputNames.end(), label) != inputNames.end() && inputNames.size() == 1) {
        storeVectorMap[inputNames].push_back(label);
        break;
      }
    }

    // If there is no Check for a given task output, create a candidate for a sink device
    InputNames singleEntry{ label };
    // Init empty Check vector to appear in the next step
    checksMap[singleEntry];
    storeVectorMap[singleEntry].push_back(label);
  }

  // Create CheckRunners: 1 per set of inputs
  CheckRunnerFactory checkRunnerFactory;
  for (auto& [inputNames, checks] : checksMap) {
    //Logging
    ILOG(Info) << ">> Inputs (" << inputNames.size() << "): ";
    for (auto& name : inputNames)
      ILOG(Info) << name << " ";
    ILOG(Info) << " checks (" << checks.size() << "): ";
    for (auto& check : checks)
      ILOG(Info) << check.getName() << " ";
    ILOG(Info) << " stores (" << storeVectorMap[inputNames].size() << "): ";
    for (auto& input : storeVectorMap[inputNames])
      ILOG(Info) << input << " ";
    ILOG(Info) << ENDM;

    if (!checks.empty()) { // Create a CheckRunner for the grouped checks
      workflow.emplace_back(checkRunnerFactory.create(checks, configurationSource, storeVectorMap[inputNames]));
    } else { // If there are no checks, create a sink CheckRunner
      workflow.emplace_back(checkRunnerFactory.createSinkDevice(tasksOutputMap.find(inputNames[0])->second, configurationSource));
    }
  }
}

} // namespace o2::quality_control::core
