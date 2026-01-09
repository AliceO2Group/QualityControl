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
/// \file   InfrastructureGenerator.cxx
/// \author Piotr Konopka
///

#include "QualityControl/InfrastructureGenerator.h"

#include "QualityControl/Aggregator.h"
#include "QualityControl/AggregatorRunnerFactory.h"
#include "QualityControl/BookkeepingQualitySink.h"
#include "QualityControl/Check.h"
#include "QualityControl/CheckRunnerFactory.h"
#include "QualityControl/InfrastructureSpec.h"
#include "QualityControl/InfrastructureSpecReader.h"
#include "QualityControl/PostProcessingDevice.h"
#include "QualityControl/PostProcessingRunner.h"
#include "QualityControl/QcInfoLogger.h"
#include "QualityControl/RootFileSink.h"
#include "QualityControl/RootFileSource.h"
#include "QualityControl/TaskRunner.h"
#include "QualityControl/TaskRunnerFactory.h"
#include "QualityControl/Version.h"

#include <Framework/DataProcessorSpec.h>
#include <Framework/DataRefUtils.h>
#include <Framework/DataSpecUtils.h>
#include <Framework/ExternalFairMQDeviceProxy.h>
#include <Framework/DataDescriptorQueryBuilder.h>
#include <Framework/O2ControlParameters.h>
#include <Framework/CommonLabels.h>
#include <Mergers/MergerInfrastructureBuilder.h>
#include <Mergers/MergerBuilder.h>
#include <DataSampling/DataSampling.h>
#include <boost/property_tree/ptree.hpp>

#include <algorithm>
#include <set>
#include <utility>
#include <vector>
#include <ranges>

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

constexpr uint16_t defaultPolicyPort = 42349;
constexpr auto proxyMemoryKillThresholdMB = "5000";

struct DataSamplingPolicySpec {
  DataSamplingPolicySpec(std::string name, std::string control, std::string remoteMachine = "")
    : name(std::move(name)), control(std::move(control)), remoteMachine(std::move(remoteMachine)) {}
  bool operator<(const DataSamplingPolicySpec& other) const
  {
    return std::tie(name, control, remoteMachine) < std::tie(other.name, other.control, other.remoteMachine);
  }
  std::string name;
  std::string control;
  std::string remoteMachine;
};

void enableDraining(framework::Options& options)
{
  if (auto readyStatePolicy = std::find_if(options.begin(), options.end(), [](const auto& option) { return option.name == "ready-state-policy"; });
      readyStatePolicy != options.end()) {
    readyStatePolicy->defaultValue = "drain";
  } else {
    ILOG(Error) << "Could not find 'ready-state-policy' option to enable draining in READY" << ENDM;
  }
}

framework::WorkflowSpec InfrastructureGenerator::generateStandaloneInfrastructure(const boost::property_tree::ptree& configurationTree)
{
  printVersion();

  auto infrastructureSpec = InfrastructureSpecReader::readInfrastructureSpec(configurationTree, WorkflowType::Standalone);
  // todo: report the number of tasks/checks/etc once all are read there.

  WorkflowSpec workflow;
  std::ranges::copy(infrastructureSpec.tasks | std::views::filter(&TaskSpec::active) | std::views::transform([&](const TaskSpec& taskSpec) {
                      // The "resetAfterCycles" parameters should be handled differently for standalone/remote and local tasks,
                      // thus we should not let TaskRunnerFactory read it and decide by itself, since it might not be aware of
                      // the context we run QC.
                      return TaskRunnerFactory::create(TaskRunnerFactory::extractConfig(infrastructureSpec.common, taskSpec, 0, taskSpec.resetAfterCycles));
                    }),
                    std::back_inserter(workflow));

  generateCheckRunners(workflow, infrastructureSpec);
  generateAggregator(workflow, infrastructureSpec);
  generatePostProcessing(workflow, infrastructureSpec);
  generateBookkeepingQualitySink(workflow, infrastructureSpec);

  return workflow;
}

void InfrastructureGenerator::generateStandaloneInfrastructure(framework::WorkflowSpec& workflow, const boost::property_tree::ptree& configurationTree)
{
  auto qcInfrastructure = InfrastructureGenerator::generateStandaloneInfrastructure(configurationTree);
  workflow.insert(std::end(workflow), std::begin(qcInfrastructure), std::end(qcInfrastructure));
}

framework::WorkflowSpec InfrastructureGenerator::generateFullChainInfrastructure(const ptree& configurationTree)
{
  printVersion();

  auto infrastructureSpec = InfrastructureSpecReader::readInfrastructureSpec(configurationTree, WorkflowType::FullChain);
  WorkflowSpec workflow;

  for (const auto& taskSpec : infrastructureSpec.tasks | std::views::filter(&TaskSpec::active)) {
    if (taskSpec.location == TaskLocationSpec::Local) {
      // If we use delta mergers, then the moving window is implemented by the last Merger layer.
      // The QC Tasks should always send a delta covering one cycle.
      auto taskConfig = TaskRunnerFactory::extractConfig(infrastructureSpec.common, taskSpec, 1, TaskRunnerFactory::computeResetAfterCycles(taskSpec, true));
      // Generate QC Task Runner
      workflow.emplace_back(TaskRunnerFactory::create(taskConfig));

      // In "delta" mode Mergers should implement moving window, in "entire" - QC Tasks.
      size_t resetAfterCycles = taskSpec.mergingMode == "delta" ? taskSpec.resetAfterCycles : 0;
      auto cycleDurationsMultiplied = TaskRunnerFactory::getSanitizedCycleDurations(infrastructureSpec.common, taskSpec);
      std::for_each(cycleDurationsMultiplied.begin(), cycleDurationsMultiplied.end(),
                    [taskSpec](std::pair<size_t, size_t>& p) { p.first *= taskSpec.mergerCycleMultiplier; });
      bool enableMovingWindows = !taskSpec.movingWindows.empty();
      generateMergers(workflow, taskSpec.taskName, 1, cycleDurationsMultiplied,
                      taskSpec.mergingMode, resetAfterCycles, infrastructureSpec.common.monitoringUrl,
                      taskSpec.detectorName, taskSpec.mergersPerLayer, enableMovingWindows, taskSpec.critical);
    } else { // TaskLocationSpec::Remote
      auto taskConfig = TaskRunnerFactory::extractConfig(infrastructureSpec.common, taskSpec, 0, taskSpec.resetAfterCycles);
      workflow.emplace_back(TaskRunnerFactory::create(taskConfig));
    }
  }

  generateCheckRunners(workflow, infrastructureSpec);
  generateAggregator(workflow, infrastructureSpec);
  generatePostProcessing(workflow, infrastructureSpec);
  generateBookkeepingQualitySink(workflow, infrastructureSpec);

  return workflow;
}

void InfrastructureGenerator::generateFullChainInfrastructure(WorkflowSpec& workflow, const ptree& configurationTree)
{
  auto qcInfrastructure = InfrastructureGenerator::generateFullChainInfrastructure(configurationTree);
  workflow.insert(std::end(workflow), std::begin(qcInfrastructure), std::end(qcInfrastructure));
}

WorkflowSpec InfrastructureGenerator::generateLocalInfrastructure(const boost::property_tree::ptree& configurationTree, const std::string& targetHost)
{
  printVersion();

  auto infrastructureSpec = InfrastructureSpecReader::readInfrastructureSpec(configurationTree, WorkflowType::Local);

  WorkflowSpec workflow;
  std::set<DataSamplingPolicySpec> samplingPoliciesForRemoteTasks;

  if (infrastructureSpec.tasks.empty()) {
    return workflow;
  }

  for (const auto& taskSpec : infrastructureSpec.tasks | std::views::filter(&TaskSpec::active)) {

    if (taskSpec.location == TaskLocationSpec::Local) {
      if (taskSpec.localMachines.empty()) {
        throw std::runtime_error("No local machines specified for task " + taskSpec.taskName + " in its configuration");
      }

      size_t id = 1;
      for (const auto& machine : taskSpec.localMachines) {
        // We spawn a task and proxy only if we are on the right machine.
        if (machine == targetHost) {
          // If we use delta mergers, then the moving window is implemented by the last Merger layer.
          // The QC Tasks should always send a delta covering one cycle.
          auto taskConfig = TaskRunnerFactory::extractConfig(infrastructureSpec.common, taskSpec, id, TaskRunnerFactory::computeResetAfterCycles(taskSpec, true));
          // Generate QC Task Runner
          workflow.emplace_back(TaskRunnerFactory::create(taskConfig));
          // Generate an output proxy
          // These should be removed when we are able to declare dangling output in normal DPL devices
          generateLocalTaskLocalProxy(workflow, id, taskSpec);
          break;
        }
        id++;
      }
    } else // TaskLocationSpec::Remote
    {
      // Collecting Data Sampling Policies
      for (const auto& dataSource : taskSpec.dataSources) {
        if (dataSource.isOneOf(DataSourceType::DataSamplingPolicy)) {
          samplingPoliciesForRemoteTasks.insert({ dataSource.name, taskSpec.localControl, taskSpec.remoteMachine });
        } else {
          throw std::runtime_error(
            "Configuration error: dataSource '" + dataSource.name + "' for a remote QC Task '" + taskSpec.taskName + //
            "' does not have a supported type. Remote QC tasks can subscribe only to data sampling policies outputs.");
        }
      }
    }
  }

  if (!samplingPoliciesForRemoteTasks.empty()) {
    auto dataSamplingTree = configurationTree.get_child("dataSamplingPolicies");
    // Creating Data Sampling Policies proxies
    for (const auto& [policyName, control, remoteMachine] : samplingPoliciesForRemoteTasks) {
      std::string port = std::to_string(DataSampling::PortForPolicy(dataSamplingTree, policyName).value_or(defaultPolicyPort));
      Inputs inputSpecs = DataSampling::InputSpecsForPolicy(dataSamplingTree, policyName);
      std::vector<std::string> machines = DataSampling::MachinesForPolicy(dataSamplingTree, policyName);

      if (machines.empty() || std::ranges::find(machines, targetHost) != machines.end()) {
        if (DataSampling::BindLocationForPolicy(dataSamplingTree, policyName) == "remote") {
          generateDataSamplingPolicyLocalProxyConnect(workflow, policyName, inputSpecs, remoteMachine, port, control);
        } else {
          generateDataSamplingPolicyLocalProxyBind(workflow, policyName, inputSpecs, targetHost, port, control);
        }
      }
    }
  }

  return workflow;
}

void InfrastructureGenerator::generateLocalInfrastructure(framework::WorkflowSpec& workflow, const boost::property_tree::ptree& configurationTree, const std::string& host)
{
  auto qcInfrastructure = InfrastructureGenerator::generateLocalInfrastructure(configurationTree, host);
  workflow.insert(std::end(workflow), std::begin(qcInfrastructure), std::end(qcInfrastructure));
}

o2::framework::WorkflowSpec InfrastructureGenerator::generateRemoteInfrastructure(const boost::property_tree::ptree& configurationTree)
{
  printVersion();

  auto infrastructureSpec = InfrastructureSpecReader::readInfrastructureSpec(configurationTree, WorkflowType::Remote);

  WorkflowSpec workflow;
  std::set<DataSamplingPolicySpec> samplingPoliciesForRemoteTasks;

  for (const auto& taskSpec : infrastructureSpec.tasks | std::views::filter(&TaskSpec::active)) {
    if (taskSpec.location == TaskLocationSpec::Local) {
      // if tasks are LOCAL, generate input proxies + mergers + checkers

      size_t numberOfLocalMachines = taskSpec.localMachines.size() > 1 ? taskSpec.localMachines.size() : 1;
      // Generate an input proxy
      // These should be removed when we are able to declare dangling inputs in normal DPL devices
      generateLocalTaskRemoteProxy(workflow, taskSpec, numberOfLocalMachines);

      // In "delta" mode Mergers should implement moving window, in "entire" - QC Tasks.
      size_t resetAfterCycles = taskSpec.mergingMode == "delta" ? taskSpec.resetAfterCycles : 0;
      auto cycleDurationsMultiplied = TaskRunnerFactory::getSanitizedCycleDurations(infrastructureSpec.common, taskSpec);
      std::for_each(cycleDurationsMultiplied.begin(), cycleDurationsMultiplied.end(),
                    [taskSpec](std::pair<size_t, size_t>& p) { p.first *= taskSpec.mergerCycleMultiplier; });
      bool enableMovingWindows = !taskSpec.movingWindows.empty();
      generateMergers(workflow, taskSpec.taskName, numberOfLocalMachines, cycleDurationsMultiplied, taskSpec.mergingMode,
                      resetAfterCycles, infrastructureSpec.common.monitoringUrl, taskSpec.detectorName, taskSpec.mergersPerLayer, enableMovingWindows, taskSpec.critical);

    } else if (taskSpec.location == TaskLocationSpec::Remote) {

      // -- if tasks are REMOTE, generate dispatcher proxies + tasks + checkers
      // (for the time being we don't foresee parallel tasks on QC servers, so no mergers here)

      // Collecting Data Sampling Policies
      for (const auto& dataSource : taskSpec.dataSources) {
        if (dataSource.isOneOf(DataSourceType::DataSamplingPolicy)) {
          samplingPoliciesForRemoteTasks.insert({ dataSource.name, taskSpec.localControl, taskSpec.remoteMachine });
        } else {
          throw std::runtime_error(
            "Configuration error: dataSource '" + dataSource.name + "' for a remote QC Task '" + taskSpec.taskName + //
            "' does not have a supported type. Remote QC tasks can subscribe only to data sampling policies outputs.");
        }
      }

      // Creating the remote task
      auto taskConfig = TaskRunnerFactory::extractConfig(infrastructureSpec.common, taskSpec, 0, taskSpec.resetAfterCycles);
      workflow.emplace_back(TaskRunnerFactory::create(taskConfig));
    }
  }

  if (!samplingPoliciesForRemoteTasks.empty()) {
    auto dataSamplingTree = configurationTree.get_child("dataSamplingPolicies");
    // Creating Data Sampling Policies proxies
    for (const auto& [policyName, control, remoteMachine] : samplingPoliciesForRemoteTasks) {
      (void)remoteMachine;
      std::string port = std::to_string(DataSampling::PortForPolicy(dataSamplingTree, policyName).value_or(defaultPolicyPort));
      Outputs outputSpecs = DataSampling::OutputSpecsForPolicy(dataSamplingTree, policyName);
      if (DataSampling::BindLocationForPolicy(dataSamplingTree, policyName) == "remote") {
        generateDataSamplingPolicyRemoteProxyBind(workflow, policyName, outputSpecs, port, control);
      } else {
        // todo now we have to generate one proxy per local machine and policy, because of the proxy limitations.
        //  Use one proxy per policy when it is possible.
        std::vector<std::string> localMachines = DataSampling::MachinesForPolicy(dataSamplingTree, policyName);
        for (const auto& localMachine : localMachines) {
          generateDataSamplingPolicyRemoteProxyConnect(workflow, policyName, outputSpecs, localMachine, port, control);
        }
      }
    }
  }

  generateCheckRunners(workflow, infrastructureSpec);
  generateAggregator(workflow, infrastructureSpec);
  generatePostProcessing(workflow, infrastructureSpec);
  generateBookkeepingQualitySink(workflow, infrastructureSpec);

  return workflow;
}

void InfrastructureGenerator::generateRemoteInfrastructure(framework::WorkflowSpec& workflow, const boost::property_tree::ptree& configurationTree)
{
  auto qcInfrastructure = InfrastructureGenerator::generateRemoteInfrastructure(configurationTree);
  workflow.insert(std::end(workflow), std::begin(qcInfrastructure), std::end(qcInfrastructure));
}

framework::WorkflowSpec InfrastructureGenerator::generateLocalBatchInfrastructure(const boost::property_tree::ptree& configurationTree, const std::string& sinkFilePath)
{
  printVersion();

  auto infrastructureSpec = InfrastructureSpecReader::readInfrastructureSpec(configurationTree, WorkflowType::LocalBatch);
  std::vector<InputSpec> fileSinkInputs;

  WorkflowSpec workflow;

  for (const auto& taskSpec : infrastructureSpec.tasks | std::views::filter(&TaskSpec::active)) {
    // We will merge deltas, thus we need to reset after each cycle (resetAfterCycles==1)
    auto taskConfig = TaskRunnerFactory::extractConfig(infrastructureSpec.common, taskSpec, 0, 1);
    workflow.emplace_back(TaskRunnerFactory::create(taskConfig));

    fileSinkInputs.emplace_back(taskSpec.taskName, TaskRunner::createTaskDataOrigin(taskSpec.detectorName), TaskRunner::createTaskDataDescription(taskSpec.taskName), Lifetime::Sporadic);
  }

  if (!fileSinkInputs.empty()) {
    // todo: could be moved to a factory.
    workflow.push_back({ "qc-root-file-sink",
                         std::move(fileSinkInputs),
                         Outputs{},
                         adaptFromTask<RootFileSink>(sinkFilePath),
                         Options{},
                         CommonServices::defaultServices(),
                         { RootFileSink::getLabel() } });
  }

  return workflow;
}

void InfrastructureGenerator::generateLocalBatchInfrastructure(framework::WorkflowSpec& workflow, const boost::property_tree::ptree& configurationTree, const std::string& sinkFilePath)
{
  auto qcInfrastructure = InfrastructureGenerator::generateLocalBatchInfrastructure(configurationTree, sinkFilePath);
  workflow.insert(std::end(workflow), std::begin(qcInfrastructure), std::end(qcInfrastructure));
}

framework::WorkflowSpec InfrastructureGenerator::generateRemoteBatchInfrastructure(const boost::property_tree::ptree& configurationTree, const std::string& sourceFilePath)
{
  printVersion();

  auto infrastructureSpec = InfrastructureSpecReader::readInfrastructureSpec(configurationTree, WorkflowType::RemoteBatch);

  WorkflowSpec workflow;

  std::vector<OutputSpec> fileSourceOutputs;
  for (const auto& taskSpec : infrastructureSpec.tasks | std::views::filter(&TaskSpec::active)) {
    auto taskConfig = TaskRunnerFactory::extractConfig(infrastructureSpec.common, taskSpec, 0, 1);
    fileSourceOutputs.push_back(taskConfig.moSpec);
    fileSourceOutputs.back().binding = RootFileSource::outputBinding(taskSpec.detectorName, taskSpec.taskName);

    // We create an OutputSpec for moving windows for this task only if they are expected.
    if (!taskConfig.movingWindows.empty()) {
      fileSourceOutputs.push_back(
        { RootFileSource::outputBinding(taskSpec.detectorName, taskSpec.taskName, true),
          TaskRunner::createTaskDataOrigin(taskSpec.detectorName, true),
          TaskRunner::createTaskDataDescription(taskSpec.taskName), 0, Lifetime::Sporadic });
    }
  }
  if (!fileSourceOutputs.empty()) {
    workflow.push_back({ "qc-root-file-source", {}, std::move(fileSourceOutputs), adaptFromTask<RootFileSource>(sourceFilePath) });
  }

  generateCheckRunners(workflow, infrastructureSpec);
  generateAggregator(workflow, infrastructureSpec);
  generatePostProcessing(workflow, infrastructureSpec);
  generateBookkeepingQualitySink(workflow, infrastructureSpec);

  return workflow;
}

void InfrastructureGenerator::generateRemoteBatchInfrastructure(framework::WorkflowSpec& workflow, const boost::property_tree::ptree& configurationTree, const std::string& sourceFilePath)
{
  auto qcInfrastructure = InfrastructureGenerator::generateRemoteBatchInfrastructure(configurationTree, sourceFilePath);
  workflow.insert(std::end(workflow), std::begin(qcInfrastructure), std::end(qcInfrastructure));
}

void InfrastructureGenerator::customizeInfrastructure(std::vector<framework::CompletionPolicy>& policies)
{
  TaskRunnerFactory::customizeInfrastructure(policies);
  MergerBuilder::customizeInfrastructure(policies);
  CheckRunnerFactory::customizeInfrastructure(policies);
  AggregatorRunnerFactory::customizeInfrastructure(policies);
  RootFileSink::customizeInfrastructure(policies);
  BookkeepingQualitySink::customizeInfrastructure(policies);
}

void InfrastructureGenerator::printVersion()
{
  ILOG(Debug, Devel) << "QC version " << o2::quality_control::core::Version::GetQcVersion().getString() << ENDM;
}

void InfrastructureGenerator::generateDataSamplingPolicyLocalProxyBind(framework::WorkflowSpec& workflow,
                                                                       const string& policyName,
                                                                       const framework::Inputs& inputSpecs,
                                                                       const std::string& localMachine,
                                                                       const string& localPort,
                                                                       const std::string& control)
{
  std::string proxyName = policyName + "-proxy";
  std::string channelName = policyName + "-" + localMachine;
  std::string channelConfig = "name=" + channelName + ",type=pub,method=bind,address=tcp://*:" + localPort +
                              ",rateLogging=60,transport=zeromq,sndBufSize=4,autoBind=false";
  auto channelSelector = [channelName](InputSpec const&, const std::unordered_map<std::string, std::vector<fair::mq::Channel>>&) {
    return channelName;
  };

  workflow.emplace_back(
    specifyFairMQDeviceMultiOutputProxy(
      proxyName.c_str(),
      inputSpecs,
      channelConfig.c_str(),
      channelSelector));
  workflow.back().labels.emplace_back(control == "odc" ? ecs::preserveRawChannelsLabel : ecs::uniqueProxyLabel);
  if (getenv("O2_QC_KILL_PROXIES") != nullptr) {
    workflow.back().metadata.push_back(DataProcessorMetadata{ ecs::privateMemoryKillThresholdMB, proxyMemoryKillThresholdMB });
  }
}

void InfrastructureGenerator::generateDataSamplingPolicyRemoteProxyConnect(framework::WorkflowSpec& workflow,
                                                                           const std::string& policyName,
                                                                           const Outputs& outputSpecs,
                                                                           const std::string& localMachine,
                                                                           const std::string& localPort,
                                                                           const std::string& control)
{
  std::string channelName = policyName + "-" + localMachine;
  const std::string& proxyName = channelName; // channel name has to match proxy name

  std::string channelConfig = "name=" + channelName + ",type=sub,method=connect,address=tcp://" +
                              localMachine + ":" + localPort + ",rateLogging=60,transport=zeromq,rcvBufSize=1";

  auto proxy = specifyExternalFairMQDeviceProxy(
    proxyName.c_str(),
    outputSpecs,
    channelConfig.c_str(),
    dplModelAdaptor());
  proxy.labels.emplace_back(control == "odc" ? ecs::preserveRawChannelsLabel : ecs::uniqueProxyLabel);
  proxy.labels.emplace_back(DataProcessorLabel{ "input-proxy" });
  // if not in RUNNING, we should drop all the incoming messages, we set the corresponding proxy option.
  enableDraining(proxy.options);
  workflow.emplace_back(std::move(proxy));
  if (getenv("O2_QC_KILL_PROXIES") != nullptr) {
    workflow.back().metadata.push_back(DataProcessorMetadata{ ecs::privateMemoryKillThresholdMB, proxyMemoryKillThresholdMB });
  }
}

void InfrastructureGenerator::generateDataSamplingPolicyLocalProxyConnect(framework::WorkflowSpec& workflow,
                                                                          const string& policyName,
                                                                          const framework::Inputs& inputSpecs,
                                                                          const std::string& remoteMachine,
                                                                          const string& remotePort,
                                                                          const std::string& control)
{
  std::string proxyName = policyName + "-proxy";
  const std::string& channelName = policyName;
  std::string channelConfig = "name=" + channelName + ",type=pub,method=connect,address=tcp://" + remoteMachine + ":" + remotePort +
                              ",rateLogging=60,transport=zeromq,sndBufSize=4";
  auto channelSelector = [channelName](InputSpec const&, const std::unordered_map<std::string, std::vector<fair::mq::Channel>>&) {
    return channelName;
  };

  workflow.emplace_back(
    specifyFairMQDeviceMultiOutputProxy(
      proxyName.c_str(),
      inputSpecs,
      channelConfig.c_str(),
      channelSelector));
  workflow.back().labels.emplace_back(control == "odc" ? ecs::preserveRawChannelsLabel : ecs::uniqueProxyLabel);
  if (getenv("O2_QC_KILL_PROXIES") != nullptr) {
    workflow.back().metadata.push_back(DataProcessorMetadata{ ecs::privateMemoryKillThresholdMB, proxyMemoryKillThresholdMB });
  }
}

void InfrastructureGenerator::generateDataSamplingPolicyRemoteProxyBind(framework::WorkflowSpec& workflow,
                                                                        const std::string& policyName,
                                                                        const Outputs& outputSpecs,
                                                                        const std::string& remotePort,
                                                                        const std::string& control)
{
  const std::string& channelName = policyName;
  const std::string& proxyName = channelName; // channel name has to match proxy name

  std::string channelConfig = "name=" + channelName + ",type=sub,method=bind,address=tcp://*:" + remotePort + ",rateLogging=60,transport=zeromq,rcvBufSize=1,autoBind=false";

  auto proxy = specifyExternalFairMQDeviceProxy(
    proxyName.c_str(),
    outputSpecs,
    channelConfig.c_str(),
    dplModelAdaptor());
  proxy.labels.emplace_back(control == "odc" ? ecs::preserveRawChannelsLabel : ecs::uniqueProxyLabel);
  proxy.labels.emplace_back(DataProcessorLabel{ "input-proxy" });
  // if not in RUNNING, we should drop all the incoming messages, we set the corresponding proxy option.
  enableDraining(proxy.options);
  if (getenv("O2_QC_KILL_PROXIES") != nullptr) {
    proxy.metadata.push_back(DataProcessorMetadata{ ecs::privateMemoryKillThresholdMB, proxyMemoryKillThresholdMB });
  }
  workflow.emplace_back(std::move(proxy));
}

void InfrastructureGenerator::generateLocalTaskLocalProxy(framework::WorkflowSpec& workflow, size_t id,
                                                          const TaskSpec& taskSpec)
{
  std::string taskName = taskSpec.taskName;
  std::string remotePort = std::to_string(taskSpec.remotePort);
  std::string proxyName = taskSpec.detectorName + "-" + taskName + "-proxy";
  std::string channelName = taskSpec.detectorName + "-" + taskName + "-proxy";
  InputSpec proxyInput{ channelName, TaskRunner::createTaskDataOrigin(taskSpec.detectorName, false), TaskRunner::createTaskDataDescription(taskName), static_cast<SubSpec>(id), Lifetime::Sporadic };
  std::string channelConfig = "name=" + channelName + ",type=pub,method=connect,address=tcp://" +
                              taskSpec.remoteMachine + ":" + remotePort + ",rateLogging=60,transport=zeromq,sndBufSize=4";

  workflow.emplace_back(specifyFairMQDeviceMultiOutputProxy(proxyName.c_str(), { proxyInput }, channelConfig.c_str()));
  workflow.back().labels.emplace_back(taskSpec.localControl == "odc" ? ecs::preserveRawChannelsLabel : ecs::uniqueProxyLabel);
  if (!taskSpec.critical) {
    workflow.back().labels.emplace_back(framework::DataProcessorLabel{ "expendable" });
  }
  if (getenv("O2_QC_KILL_PROXIES") != nullptr) {
    workflow.back().metadata.push_back(DataProcessorMetadata{ ecs::privateMemoryKillThresholdMB, proxyMemoryKillThresholdMB });
  }
}

void InfrastructureGenerator::generateLocalTaskRemoteProxy(framework::WorkflowSpec& workflow, const TaskSpec& taskSpec, size_t numberOfLocalMachines)
{
  std::string taskName = taskSpec.taskName;
  std::string remotePort = std::to_string(taskSpec.remotePort);
  std::string proxyName = taskSpec.detectorName + "-" + taskName + "-proxy"; // channel name has to match proxy name
  std::string channelName = taskSpec.detectorName + "-" + taskName + "-proxy";

  Outputs proxyOutputs;
  for (size_t id = 1; id <= numberOfLocalMachines; id++) {
    proxyOutputs.emplace_back(
      OutputSpec{ { channelName }, TaskRunner::createTaskDataOrigin(taskSpec.detectorName, false), TaskRunner::createTaskDataDescription(taskName), static_cast<SubSpec>(id), Lifetime::Sporadic });
  }

  std::string channelConfig = "name=" + channelName + ",type=sub,method=bind,address=tcp://*:" + remotePort +
                              ",rateLogging=60,transport=zeromq,rcvBufSize=1,autoBind=false";

  auto proxy = specifyExternalFairMQDeviceProxy(
    proxyName.c_str(),
    proxyOutputs,
    channelConfig.c_str(),
    dplModelAdaptor());
  proxy.labels.emplace_back(taskSpec.localControl == "odc" ? ecs::preserveRawChannelsLabel : ecs::uniqueProxyLabel);
  proxy.labels.emplace_back(DataProcessorLabel{ "input-proxy" });
  if (!taskSpec.critical) {
    proxy.labels.emplace_back(framework::DataProcessorLabel{ "expendable" });
  }
  proxy.labels.emplace_back(framework::suppressDomainInfoLabel); // QC-1320
  // if not in RUNNING, we should drop all the incoming messages, we set the corresponding proxy option.
  enableDraining(proxy.options);
  if (getenv("O2_QC_KILL_PROXIES") != nullptr) {
    proxy.metadata.push_back(DataProcessorMetadata{ ecs::privateMemoryKillThresholdMB, proxyMemoryKillThresholdMB });
  }
  workflow.emplace_back(std::move(proxy));
}
void InfrastructureGenerator::generateMergers(framework::WorkflowSpec& workflow, const std::string& taskName,
                                              size_t numberOfLocalMachines, std::vector<std::pair<size_t, size_t>> cycleDurations,
                                              const std::string& mergingMode, size_t resetAfterCycles, std::string monitoringUrl,
                                              const std::string& detectorName, std::vector<size_t> mergersPerLayer, bool enableMovingWindows, bool critical)
{
  Inputs mergerInputs;
  for (size_t id = 1; id <= numberOfLocalMachines; id++) {
    mergerInputs.emplace_back(
      InputSpec{ { taskName + std::to_string(id) },
                 TaskRunner::createTaskDataOrigin(detectorName, false),
                 TaskRunner::createTaskDataDescription(taskName),
                 static_cast<SubSpec>(id),
                 Lifetime::Sporadic });
  }

  MergerInfrastructureBuilder mergersBuilder;
  mergersBuilder.setInfrastructureName(taskName);
  mergersBuilder.setInputSpecs(mergerInputs);
  mergersBuilder.setOutputSpec(
    { { "main" }, TaskRunner::createTaskDataOrigin(detectorName, false), TaskRunner::createTaskDataDescription(taskName), 0, Lifetime::Sporadic });
  mergersBuilder.setOutputSpecMovingWindow(
    { { "main_mw" }, TaskRunner::createTaskDataOrigin(detectorName, true), TaskRunner::createTaskDataDescription(taskName), 0, Lifetime::Sporadic });
  MergerConfig mergerConfig;
  // if we are to change the mode to Full, disable reseting tasks after each cycle.
  mergerConfig.inputObjectTimespan = { (mergingMode.empty() || mergingMode == "delta") ? InputObjectsTimespan::LastDifference : InputObjectsTimespan::FullHistory };
  mergerConfig.publicationDecision = { PublicationDecision::EachNSeconds, cycleDurations };
  mergerConfig.mergedObjectTimespan = { MergedObjectTimespan::NCycles, (int)resetAfterCycles };
  // for now one merger should be enough, multiple layers to be supported later
  mergerConfig.topologySize = { TopologySize::MergersPerLayer, mergersPerLayer };
  mergerConfig.monitoringUrl = std::move(monitoringUrl);
  mergerConfig.detectorName = detectorName;
  mergerConfig.labels.push_back({ "resilient" });
  mergerConfig.labels.push_back(framework::suppressDomainInfoLabel); // QC-1320
  mergerConfig.publishMovingWindow = { enableMovingWindows ? PublishMovingWindow::Yes : PublishMovingWindow::No };
  mergerConfig.parallelismType = { (mergerConfig.inputObjectTimespan.value == InputObjectsTimespan::LastDifference) ? ParallelismType::RoundRobin : ParallelismType::SplitInputs };
  mergersBuilder.setConfig(mergerConfig);

  mergersBuilder.generateInfrastructure(workflow);
}

void InfrastructureGenerator::generateCheckRunners(framework::WorkflowSpec& workflow, const InfrastructureSpec& infrastructureSpec)
{
  // todo have a look if this complex procedure can be simplified.
  // todo also make well defined and scoped functions to make it more readable and clearer.
  typedef std::vector<std::string> InputNames;
  typedef std::vector<CheckConfig> CheckConfigs;
  std::map<std::string, o2::framework::InputSpec> tasksOutputMap; // all active tasks' output, as inputs, keyed by their label
  std::map<InputNames, CheckConfigs> checksMap;                   // all the Checks defined in the config mapped keyed by their sorted inputNames
  std::map<InputNames, InputNames> storeVectorMap;

  // todo: avoid code repetition
  for (const auto& taskSpec : infrastructureSpec.tasks | std::views::filter(&TaskSpec::active)) {
    InputSpec taskOutput{ taskSpec.taskName, TaskRunner::createTaskDataOrigin(taskSpec.detectorName), TaskRunner::createTaskDataDescription(taskSpec.taskName), Lifetime::Sporadic };
    tasksOutputMap.insert({ DataSpecUtils::label(taskOutput), taskOutput });

    bool movingWindowsEnabled = !taskSpec.movingWindows.empty();
    bool synchronousRemote = taskSpec.location == TaskLocationSpec::Local && (infrastructureSpec.workflowType == WorkflowType::Remote || infrastructureSpec.workflowType == WorkflowType::FullChain);
    bool asynchronousRemote = infrastructureSpec.workflowType == WorkflowType::RemoteBatch;
    if (movingWindowsEnabled && (synchronousRemote || asynchronousRemote)) {
      InputSpec taskMovingWindowOutput{ taskSpec.taskName, TaskRunner::createTaskDataOrigin(taskSpec.detectorName, true), TaskRunner::createTaskDataDescription(taskSpec.taskName), Lifetime::Sporadic };
      tasksOutputMap.insert({ DataSpecUtils::label(taskMovingWindowOutput), taskMovingWindowOutput });
    }
  }

  for (const auto& ppTaskSpec : infrastructureSpec.postProcessingTasks | std::views::filter(&PostProcessingTaskSpec::active)) {
    InputSpec ppTaskOutput{ ppTaskSpec.taskName,
                            PostProcessingDevice::createPostProcessingDataOrigin(ppTaskSpec.detectorName),
                            PostProcessingDevice::createPostProcessingDataDescription(ppTaskSpec.taskName),
                            Lifetime::Sporadic };
    tasksOutputMap.insert({ DataSpecUtils::label(ppTaskOutput), ppTaskOutput });
  }

  for (const auto& externalTaskSpec : infrastructureSpec.externalTasks | std::views::filter(&ExternalTaskSpec::active)) {
    auto query = externalTaskSpec.query;
    Inputs inputs = DataDescriptorQueryBuilder::parse(query.c_str());
    for (const auto& taskOutput : inputs) {
      tasksOutputMap.insert({ DataSpecUtils::label(taskOutput), taskOutput });
    }
  }

  // Instantiate Checks based on the configuration and build a map of checks (keyed by their inputs names)
  for (const auto& checkSpec : infrastructureSpec.checks | std::views::filter(&CheckSpec::active)) {
    auto checkConfig = Check::extractConfig(infrastructureSpec.common, checkSpec);
    InputNames inputNames;

    for (const auto& inputSpec : checkConfig.inputSpecs) {
      inputNames.push_back(DataSpecUtils::label(inputSpec));
    }
    // Create a grouping key - sorted vector of InputSpecs as strings //todo: consider std::set, which is sorted
    std::ranges::sort(inputNames);
    // Group checks
    checksMap[inputNames].push_back(checkConfig);
  }

  // For every Task output, find a Check to store the MOs in the database.
  // If none is found we create a sink device.
  for (const auto& label : tasksOutputMap | std::views::keys) { // for each task output
    bool isStored = false;
    // Look for this task as input in the Checks' inputs, if we found it then we are done
    for (const auto& inputNames : checksMap | std::views::keys) { // for each set of inputs
      if (std::ranges::find(inputNames, label) != inputNames.end() && inputNames.size() == 1) {
        storeVectorMap[inputNames].push_back(label);
        break;
      }
    }
    if (!isStored) { // fixme: statement is always true
      // If there is no Check for a given input, create a candidate for a sink device
      InputNames singleEntry{ label };
      // Init empty Check vector to appear in the next step
      checksMap[singleEntry];
      storeVectorMap[singleEntry].push_back(label);
    }
  }

  // Create CheckRunners: 1 per set of inputs
  std::vector<framework::OutputSpec> checkRunnerOutputs;
  auto checkRunnerConfig = CheckRunnerFactory::extractConfig(infrastructureSpec.common);
  for (auto& [inputNames, checkConfigs] : checksMap) {
    // Logging
    ILOG(Debug, Devel) << ">> Inputs (" << inputNames.size() << "): ";
    for (const auto& name : inputNames)
      ILOG(Debug, Devel) << name << " ";
    ILOG(Debug, Devel) << " ; Checks (" << checkConfigs.size() << "): ";
    for (const auto& checkConfig : checkConfigs)
      ILOG(Debug, Devel) << checkConfig.name << " ";
    ILOG(Debug, Devel) << " ; Stores (" << storeVectorMap[inputNames].size() << "): ";
    for (const auto& input : storeVectorMap[inputNames])
      ILOG(Debug, Devel) << input << " ";
    ILOG(Debug, Devel) << ENDM;

    DataProcessorSpec spec = checkConfigs.empty()
                               ? CheckRunnerFactory::createSinkDevice(checkRunnerConfig, tasksOutputMap.find(inputNames[0])->second)
                               : CheckRunnerFactory::create(checkRunnerConfig, checkConfigs, storeVectorMap[inputNames]);
    workflow.emplace_back(spec);
    checkRunnerOutputs.insert(checkRunnerOutputs.end(), spec.outputs.begin(), spec.outputs.end());
  }

  ILOG(Debug, Devel) << ">> Outputs (" << checkRunnerOutputs.size() << "): ";
  for (const auto& output : checkRunnerOutputs)
    ILOG(Debug, Devel) << DataSpecUtils::describe(output) << " ";
  ILOG(Debug, Devel) << ENDM;
}

void InfrastructureGenerator::throwIfAggNamesClashCheckNames(const InfrastructureSpec& infrastructureSpec)
{
  auto checksNames = infrastructureSpec.checks | std::views::transform([](const auto& check) {
                       return check.checkName;
                     });

  auto conflictingAggregator = std::ranges::find_if(infrastructureSpec.aggregators, [&](const auto& aggregator) {
    return std::ranges::find(checksNames, aggregator.aggregatorName) != checksNames.end();
  });

  // If a conflict is found, log the error and throw an exception
  if (conflictingAggregator != infrastructureSpec.aggregators.end()) {
    ILOG(Error, Ops) << "The aggregator \"" << conflictingAggregator->aggregatorName << "\" has the same name as one of the Check. This is forbidden." << ENDM;
    throw std::runtime_error(std::string("aggregator has the same name as a check: ") + conflictingAggregator->aggregatorName);
  }
}

void InfrastructureGenerator::generateAggregator(WorkflowSpec& workflow, const InfrastructureSpec& infrastructureSpec)
{
  if (infrastructureSpec.aggregators.empty()) {
    ILOG(Debug, Devel) << "No \"aggregators\" structure found in the config file. If no quality aggregation is expected, then it is completely fine." << ENDM;
    return;
  }

  // Make sure we don't have duplicated names in the checks and aggregators
  throwIfAggNamesClashCheckNames(infrastructureSpec);

  DataProcessorSpec spec = AggregatorRunnerFactory::create(infrastructureSpec.common, infrastructureSpec.aggregators);
  workflow.emplace_back(spec);
}

void InfrastructureGenerator::generatePostProcessing(WorkflowSpec& workflow, const InfrastructureSpec& infrastructureSpec)
{
  if (infrastructureSpec.postProcessingTasks.empty()) {
    ILOG(Debug, Devel) << "No \"postprocessing\" structure found in the config file. If no postprocessing is expected, then it is completely fine." << ENDM;
    return;
  }
  for (const auto& ppTaskSpec : infrastructureSpec.postProcessingTasks | std::views::filter(&PostProcessingTaskSpec::active)) {
    PostProcessingDevice ppTask{ PostProcessingRunner::extractConfig(infrastructureSpec.common, ppTaskSpec) };

    DataProcessorSpec dataProcessorSpec{
      ppTask.getDeviceName(),
      ppTask.getInputsSpecs(),
      ppTask.getOutputSpecs(),
      {},
      ppTask.getOptions()
    };
    dataProcessorSpec.labels.emplace_back(PostProcessingDevice::getLabel());
    if (!ppTaskSpec.critical) {
      framework::DataProcessorLabel expendableLabel = { "expendable" };
      dataProcessorSpec.labels.emplace_back(expendableLabel);
    }
    dataProcessorSpec.algorithm = adaptFromTask<PostProcessingDevice>(std::move(ppTask));

    workflow.emplace_back(std::move(dataProcessorSpec));
  }
}

template <typename Type>
auto createSinkInput(const std::string& detectorName, const std::string& name) -> framework::InputSpec
{
  const auto outputSpec = Type::createOutputSpec(detectorName, name);
  auto input = DataSpecUtils::matchingInput(outputSpec);
  input.binding = name;
  return input;
}

void InfrastructureGenerator::generateBookkeepingQualitySink(WorkflowSpec& workflow, const InfrastructureSpec& infrastructureSpec)
{
  framework::Inputs sinkInputs{};

  for (const auto& checkSpec : infrastructureSpec.checks | std::views::filter(&CheckSpec::active) | std::views::filter(&CheckSpec::exportToBookkeeping)) {
    ILOG(Debug, Support) << "Adding input to BookkeepingSink from check " << checkSpec.checkName << " and detector: " << checkSpec.detectorName << ENDM;
    sinkInputs.emplace_back(createSinkInput<Check>(checkSpec.detectorName, checkSpec.checkName));
  }

  for (const auto& aggregatorSpec : infrastructureSpec.aggregators | std::views::filter(&AggregatorSpec::active) | std::views::filter(&AggregatorSpec::exportToBookkeeping)) {
    ILOG(Debug, Support) << "Adding input to BookkeepingSink from aggregator " << aggregatorSpec.aggregatorName << " and detector: " << aggregatorSpec.detectorName << ENDM;
    sinkInputs.emplace_back(createSinkInput<Aggregator>(aggregatorSpec.detectorName, aggregatorSpec.aggregatorName));
  }

  if (sinkInputs.empty()) {
    ILOG(Debug, Support) << "BookkeepingSink is not being created because we couldn't find any suitable inputs." << ENDM;
    return;
  }

  DataProcessorSpec sinkDataProcessor{
    .name = "BookkeepingSink",
    .inputs = sinkInputs,
    .outputs = Outputs{},
    .algorithm = adaptFromTask<quality_control::core::BookkeepingQualitySink>(
      infrastructureSpec.common.bookkeepingUrl,
      core::toEnum(infrastructureSpec.common.activityProvenance)),
    .labels = { { "resilient" }, BookkeepingQualitySink::getLabel() }
  };
  workflow.emplace_back(std::move(sinkDataProcessor));
}

} // namespace o2::quality_control::core
