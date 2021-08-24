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
/// \file   InfrastructureSpecReader.cxx
/// \author Piotr Konopka
///

#include "QualityControl/InfrastructureSpecReader.h"
#include "QualityControl/QcInfoLogger.h"

#include <DataSampling/DataSampling.h>
#include <Framework/DataDescriptorQueryBuilder.h>
#include <boost/property_tree/ptree.hpp>

using namespace o2::utilities;
using namespace o2::framework;

namespace o2::quality_control::core
{

InfrastructureSpec InfrastructureSpecReader::readInfrastructureSpec(const boost::property_tree::ptree& wholeTree, const std::string& configurationSource)
{
  InfrastructureSpec spec;
  const auto& qcTree = wholeTree.get_child("qc");
  if (qcTree.find("config") != qcTree.not_found()) {
    spec.common = readCommonSpec(qcTree.get_child("config"), configurationSource);
  } else {
    ILOG(Error) << "The \"config\" section in the provided QC config file is missing." << ENDM;
  }
  if (qcTree.find("tasks") != qcTree.not_found()) {
    const auto& tasksTree = qcTree.get_child("tasks");
    spec.tasks.reserve(tasksTree.size());
    for (const auto& [taskName, taskConfig] : tasksTree) {
      spec.tasks.push_back(readTaskSpec(taskName, taskConfig, wholeTree));
    }
  }
  return spec;
}

CommonSpec InfrastructureSpecReader::readCommonSpec(const boost::property_tree::ptree& commonTree, const std::string& configurationSource)
{
  CommonSpec spec;
  for (const auto& [key, value] : commonTree.get_child("database")) {
    spec.database.emplace(key, value.get_value<std::string>());
  }
  spec.activityNumber = commonTree.get<int>("Activity.number", spec.activityNumber);
  spec.activityType = commonTree.get<int>("Activity.type", spec.activityType);
  spec.monitoringUrl = commonTree.get<std::string>("monitoring.url", spec.monitoringUrl);
  spec.consulUrl = commonTree.get<std::string>("consul.url", spec.consulUrl);
  spec.conditionDBUrl = commonTree.get<std::string>("conditionDB.url", spec.conditionDBUrl);
  spec.infologgerFilterDiscardDebug = commonTree.get<bool>("infologger.filterDiscardDebug", spec.infologgerFilterDiscardDebug);
  spec.infologgerDiscardLevel = commonTree.get<int>("infologger.filterDiscardLevel", spec.infologgerDiscardLevel);

  spec.configurationSource = configurationSource;

  return spec;
}

TaskSpec InfrastructureSpecReader::readTaskSpec(std::string taskName, const boost::property_tree::ptree& taskTree, const boost::property_tree::ptree& wholeTree)
{
  static std::unordered_map<std::string, TaskLocationSpec> const taskLocationFromString = {
    { "local", TaskLocationSpec::Local },
    { "remote", TaskLocationSpec::Remote }
  };

  TaskSpec ts;

  ts.taskName = taskName;
  ts.className = taskTree.get<std::string>("className");
  ts.moduleName = taskTree.get<std::string>("moduleName");
  ts.detectorName = taskTree.get<std::string>("detectorName");
  ts.cycleDurationSeconds = taskTree.get<int>("cycleDurationSeconds");
  ts.dataSource = readDataSourceSpec(taskTree.get_child("dataSource"), wholeTree);

  ts.active = taskTree.get<bool>("active", ts.active);
  ts.maxNumberCycles = taskTree.get<int>("maxNumberCycles", ts.maxNumberCycles);
  ts.resetAfterCycles = taskTree.get<size_t>("resetAfterCycles", ts.resetAfterCycles);
  ts.saveObjectsToFile = taskTree.get<std::string>("saveObjectsToFile", ts.saveObjectsToFile);
  if (taskTree.count("taskParameters") > 0) {
    for (const auto& [key, value] : taskTree.get_child("taskParameters")) {
      ts.customParameters.emplace(key, value.get_value<std::string>());
    }
  }

  bool multinodeSetup = taskTree.find("location") != taskTree.not_found();
  ts.location = taskLocationFromString.at(taskTree.get<std::string>("location", "remote"));
  if (taskTree.count("localMachines") > 0) {
    for (const auto& [key, value] : taskTree.get_child("localMachines")) {
      ts.localMachines.emplace_back(value.get_value<std::string>());
    }
  }
  if (multinodeSetup && taskTree.count("remoteMachine") == 0) {
    ILOG(Warning, Trace)
      << "No remote machine was specified for a multinode QC setup."
         " This is fine if running with AliECS, but it will fail in standalone mode."
      << ENDM;
  }
  ts.remoteMachine = taskTree.get<std::string>("remoteMachine", ts.remoteMachine);
  if (multinodeSetup && taskTree.count("remotePort") == 0) {
    ILOG(Warning, Trace)
      << "No remote port was specified for a multinode QC setup."
         " This is fine if running with AliECS, but it might fail in standalone mode."
      << ENDM;
  }
  ts.remotePort = taskTree.get<uint16_t>("remotePort", ts.remotePort);
  ts.localControl = taskTree.get<std::string>("localControl", ts.localControl);
  ts.mergingMode = taskTree.get<std::string>("mergingMode", ts.mergingMode);
  ts.mergerCycleMultiplier = taskTree.get<int>("mergerCycleMultiplier", ts.mergerCycleMultiplier);

  return ts;
}

DataSourceSpec InfrastructureSpecReader::readDataSourceSpec(const boost::property_tree::ptree& dataSourceTree,
                                                            const boost::property_tree::ptree& wholeTree)
{
  static std::unordered_map<std::string, DataSourceType> const dataSourceTypeFromString = {
    // fixme: the convention is inconsistent and it should be fixed in coordination with configuration files
    { "dataSamplingPolicy", DataSourceType::DataSamplingPolicy },
    { "direct", DataSourceType::Direct },
    { "Task", DataSourceType::Task },
    { "Check", DataSourceType::Check },
    { "Aggregator", DataSourceType::Aggregator },
    { "PostProcessing", DataSourceType::PostProcessingTask },
    { "ExternalTask", DataSourceType::ExternalTask }
  };

  DataSourceSpec dss;
  dss.type = dataSourceTypeFromString.at(dataSourceTree.get<std::string>("type"));

  switch (dss.type) {
    case DataSourceType::DataSamplingPolicy: {
      auto name = dataSourceTree.get<std::string>("name");
      dss.typeSpecificParams.insert({ "name", name });
      dss.inputs = DataSampling::InputSpecsForPolicy(wholeTree.get_child("dataSamplingPolicies"), name);
      break;
    }
    case DataSourceType::Direct: {
      dss.typeSpecificParams.insert({ "query", dataSourceTree.get<std::string>("query") });
      auto inputsQuery = dataSourceTree.get<std::string>("query");
      dss.inputs = DataDescriptorQueryBuilder::parse(inputsQuery.c_str());
      break;
    }
    case DataSourceType::Task: // todo all below
    case DataSourceType::PostProcessingTask:
    case DataSourceType::Check:
    case DataSourceType::Aggregator:
      dss.typeSpecificParams.insert({ "name", dataSourceTree.get<std::string>("name") });
      break;
    case DataSourceType::ExternalTask:
      dss.typeSpecificParams.insert({ "name", dataSourceTree.get<std::string>("name") });
      dss.typeSpecificParams.insert({ "query", dataSourceTree.get<std::string>("query") });
      break;
    case DataSourceType::Invalid:
      // todo: throw?
      break;
  }

  return dss;
}

std::string InfrastructureSpecReader::validateDetectorName(std::string name)
{
  // name must be a detector code from DetID or one of the few allowed general names
  int nDetectors = 16;
  const char* detNames[16] = // once we can use DetID, remove this hard-coded list
    { "ITS", "TPC", "TRD", "TOF", "PHS", "CPV", "EMC", "HMP", "MFT", "MCH", "MID", "ZDC", "FT0", "FV0", "FDD", "ACO" };
  std::vector<std::string> permitted = { "MISC", "DAQ", "GENERAL", "TST", "BMK", "CTP", "TRG", "DCS", "REC" };
  for (auto i = 0; i < nDetectors; i++) {
    permitted.emplace_back(detNames[i]);
    //    permitted.push_back(o2::detectors::DetID::getName(i));
  }
  auto it = std::find(permitted.begin(), permitted.end(), name);

  if (it == permitted.end()) {
    std::string permittedString;
    for (const auto& i : permitted)
      permittedString += i + ' ';
    ILOG(Error, Support) << "Invalid detector name : " << name << "\n"
                         << "    Placeholder 'MISC' will be used instead\n"
                         << "    Note: list of permitted detector names :" << permittedString << ENDM;
    return "MISC";
  }
  return name;
}

} // namespace o2::quality_control::core
