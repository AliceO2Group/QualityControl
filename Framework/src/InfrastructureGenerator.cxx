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
#include <Framework/DataSpecUtils.h>
#include <Framework/ExternalFairMQDeviceProxy.h>
#include <Mergers/MergerInfrastructureBuilder.h>

#include <algorithm>

using namespace o2::framework;
using namespace o2::configuration;
using namespace o2::experimental::mergers;
using namespace o2::quality_control::checker;
using boost::property_tree::ptree;
using SubSpec = o2::header::DataHeader::SubSpecificationType;

namespace o2::quality_control::core
{

WorkflowSpec InfrastructureGenerator::generateLocalInfrastructure(std::string configurationSource, std::string host)
{
  WorkflowSpec workflow;
  TaskRunnerFactory taskRunnerFactory;
  auto config = ConfigurationFactory::getConfiguration(configurationSource);
  printVersion();

  for (const auto& [taskName, taskConfig] : config->getRecursive("qc.tasks")) {
    if (taskConfig.get<bool>("active") && taskConfig.get<std::string>("location") == "local") {

      // ids are assigned to local tasks in order to distinguish monitor objects outputs from each other and be able to
      // merge them. If there is no need to merge (only one qc task), it gets subspec 0.
      size_t id = taskConfig.get_child("machines").size() > 1 ? 1 : 0;

      // Create a TaskRunner and find out the id
      if (host.empty() || taskConfig.get_child("machines").size() <= 1) {
        workflow.emplace_back(taskRunnerFactory.create(taskName, configurationSource, 0, false));
      } else {
        for (const auto& machine : taskConfig.get_child("machines")) {
          if (machine.second.get<std::string>("") == host) {
            workflow.emplace_back(taskRunnerFactory.create(taskName, configurationSource, id, true));
          }
          id++;
        }
      }

      // Generate an output proxy
      // These should be removed when we are able to declare dangling output in normal DPL devices
      generateLocalOutputProxy(workflow, id, taskName, taskConfig.get<std::string>("remoteHost"), taskConfig.get<std::string>("remotePort"));
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
  auto config = ConfigurationFactory::getConfiguration(configurationSource);
  printVersion();

  TaskRunnerFactory taskRunnerFactory;
  for (const auto& [taskName, taskConfig] : config->getRecursive("qc.tasks")) {
    // todo sanitize somehow this if-frenzy
    if (taskConfig.get<bool>("active", true)) {

      if (taskConfig.get<std::string>("location") == "local") {
        // if tasks are LOCAL, generate input proxies + mergers + checkers

        // Generate an input proxy
        // These should be removed when we are able to declare dangling inputs in normal DPL devices
        generateRemoteInputProxy(workflow, taskName, taskConfig.get<std::string>("remoteHost"), taskConfig.get<std::string>("remotePort"));

        // Generate a Merger only when there is a need to merge something - there is more than one machine with the QC Task
        // I don't expect the list of machines to be reconfigured - all of them should be declared beforehand,
        // even if some of them will be on standby.
        if (taskConfig.get_child("machines").size() > 1) {
          generateMergers(workflow, taskName, taskConfig.get<double>("cycleDurationSeconds"));
        }

      } else if (taskConfig.get<std::string>("location") == "remote") {

        // -- if tasks are REMOTE, generate dispatcher proxies (to do) + tasks + checkers
        // (for the time being we don't foresee parallel tasks on QC servers, so no mergers here)
        workflow.emplace_back(taskRunnerFactory.create(taskName, configurationSource, 0));
      }
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
  CheckRunnerFactory::customizeInfrastructure(policies);
}

void InfrastructureGenerator::printVersion()
{
  // Log the version number
  QcInfoLogger::GetInstance() << "QC version " << o2::quality_control::core::Version::GetQcVersion().getString() << infologger::endm;
}

void InfrastructureGenerator::generateLocalOutputProxy(framework::WorkflowSpec& workflow, size_t id, std::string taskName, std::string remoteHost, std::string remotePort)
{
  std::string proxyName = taskName + "-input-proxy-" + std::to_string(id);
  InputSpec proxyInput{taskName, TaskRunner::createTaskDataOrigin(), TaskRunner::createTaskDataDescription(taskName), static_cast<SubSpec>(id)};
  std::string channelConfig =
    "name=" + taskName + "-proxy"
    ",type=push"
    ",method=connect,"
    ",address=tcp://" + remoteHost + ":" + remotePort +
    ",rateLogging=60"
    ",transport=zeromq";

  workflow.emplace_back(
     specifyFairMQDeviceOutputProxy(
        proxyName.c_str(),
        { proxyInput },
        channelConfig.c_str()
      ));
}
void InfrastructureGenerator::generateRemoteInputProxy(framework::WorkflowSpec& workflow, std::string taskName, std::string /* remoteHost */, std::string remotePort)
{
  std::string proxyName = taskName + "-input-proxy";
  OutputSpec proxyOutput{{taskName}, {TaskRunner::createTaskDataOrigin(), TaskRunner::createTaskDataDescription(taskName)}};
  std::string channelConfig =
    "name=" + taskName + "-proxy"
                         ",type=pull"
                         ",method=bind,"
                         ",address=tcp://*:" + remotePort +
    ",rateLogging=60"
    ",transport=zeromq";

  workflow.emplace_back(specifyExternalFairMQDeviceProxy(
    proxyName.c_str(),
    { proxyOutput },
    channelConfig.c_str(),
    dplModelAdaptor()));
}

void InfrastructureGenerator::generateMergers(framework::WorkflowSpec& workflow, std::string taskName, double cycleDurationSeconds)
{
  MergerInfrastructureBuilder mergersBuilder;
  mergersBuilder.setInfrastructureName(taskName);
  mergersBuilder.setInputSpecs(
    {{ taskName, { TaskRunner::createTaskDataOrigin(), TaskRunner::createTaskDataDescription(taskName) }}});
  mergersBuilder.setOutputSpec(
    {{ "main" }, TaskRunner::createTaskDataOrigin(), TaskRunner::createTaskDataDescription(taskName), 0 });
  MergerConfig mergerConfig;
  mergerConfig.ownershipMode = { OwnershipMode::Integral, 0 };
  mergerConfig.publicationDecision = {
    PublicationDecision::EachNSeconds, cycleDurationSeconds
  };
  mergerConfig.timespan = { Timespan::FullHistory, 0 };
  // for now one merger should be enough, multiple layers to be supported later
  mergerConfig.topologySize = { TopologySize::NumberOfLayers, 1 };
  mergersBuilder.setConfig(mergerConfig);

  mergersBuilder.generateInfrastructure(workflow);
}

void InfrastructureGenerator::generateCheckRunners(framework::WorkflowSpec& workflow, std::string configurationSource)
{
  // todo have a look if this complex procedure can be simplified.
  typedef std::vector<std::string> InputNames;
  typedef std::vector<Check> CheckRunnerNames;
  std::map<std::string, o2::framework::InputSpec> checkInputMap;
  std::map<InputNames, CheckRunnerNames> checkRunnerMap;
  std::map<InputNames, InputNames> storeVectorMap;

  auto config = ConfigurationFactory::getConfiguration(configurationSource);

  for (const auto& [taskName, taskConfig] : config->getRecursive("qc.tasks")) {
    if (taskConfig.get<bool>("active", true)) {
      o2::framework::InputSpec checkInput{ taskName, TaskRunner::createTaskDataOrigin(), TaskRunner::createTaskDataDescription(taskName) };
      checkInputMap.insert({ DataSpecUtils::label(checkInput), checkInput });
    }
  }

  // Check if there is "checks" section
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
        checkRunnerMap[inputNames].push_back(check);
      }
    }
  }

  // For every Task output, find device to store the MO
  for (auto& [label, inputSpec] : checkInputMap) {
    (void)inputSpec;
    bool isStored = false;
    // Look for single input
    for (auto& [inputNames, checks] : checkRunnerMap) {
      (void)checks;
      if (std::find(inputNames.begin(), inputNames.end(), label) != inputNames.end() && inputNames.size() == 1) {
        storeVectorMap[inputNames].push_back(label);
        isStored = true;
        break;
      }
    }

    if (!isStored) {
      // If not assigned to store in previous step, find a candidate without input size limitation
      for (auto& [inputNames, checks] : checkRunnerMap) {
        (void)checks;
        if (std::find(inputNames.begin(), inputNames.end(), label) != inputNames.end()) {
          storeVectorMap[inputNames].push_back(label);
          isStored = true;
          break;
        }
      }
    }

    if (!isStored) {
      // If there is no Check for a given input, create a candidate for a sink device
      InputNames singleEntry{ label };
      // Init empty Check vector to appear in the next step
      checkRunnerMap[singleEntry];
      storeVectorMap[singleEntry].push_back(label);
    }
  }

  for (auto& [inputNames, checks] : checkRunnerMap) {
    //Logging
    QcInfoLogger::GetInstance() << ">> Inputs (" << inputNames.size() << "): ";
    for (auto& name : inputNames)
      QcInfoLogger::GetInstance() << name << " ";
    QcInfoLogger::GetInstance() << " checks (" << checks.size() << "): ";
    for (auto& check : checks)
      QcInfoLogger::GetInstance() << check.getName() << " ";
    QcInfoLogger::GetInstance() << " stores (" << storeVectorMap[inputNames].size() << "): ";
    for (auto& input : storeVectorMap[inputNames])
      QcInfoLogger::GetInstance() << input << " ";
    QcInfoLogger::GetInstance() << AliceO2::InfoLogger::InfoLogger::endm;


    CheckRunnerFactory checkRunnerFactory;
    //push workflow
    if (checks.size() > 0) {
      // Create a CheckRunner for the grouped checks
      workflow.emplace_back(checkRunnerFactory.create(checks, configurationSource, storeVectorMap[inputNames]));
    } else {
      // If there are no checks, create a sink CheckRunner
      workflow.emplace_back(checkRunnerFactory.createSinkDevice(checkInputMap.find(inputNames[0])->second, configurationSource));
    }
  }
}

} // namespace o2::quality_control::core
