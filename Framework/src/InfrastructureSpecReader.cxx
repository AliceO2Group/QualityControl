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
#include "QualityControl/TaskRunner.h"
#include "QualityControl/PostProcessingDevice.h"
#include "QualityControl/Check.h"
#include "QualityControl/AggregatorRunner.h"

#include <DataSampling/DataSampling.h>
#include <Framework/DataDescriptorQueryBuilder.h>

using namespace o2::utilities;
using namespace o2::framework;
using namespace o2::quality_control::postprocessing;
using namespace o2::quality_control::checker;

namespace o2::quality_control::core
{

InfrastructureSpec InfrastructureSpecReader::readInfrastructureSpec(const boost::property_tree::ptree& wholeTree)
{
  InfrastructureSpec spec;
  const auto& qcTree = wholeTree.get_child("qc");
  if (qcTree.find("config") != qcTree.not_found()) {
    spec.common = readSpecEntry<CommonSpec>("", qcTree.get_child("config"), wholeTree);
  } else {
    ILOG(Error) << "The \"config\" section in the provided QC config file is missing." << ENDM;
  }

  spec.tasks = readSectionSpec<TaskSpec>(wholeTree, "tasks");
  spec.checks = readSectionSpec<CheckSpec>(wholeTree, "checks");
  spec.aggregators = readSectionSpec<AggregatorSpec>(wholeTree, "aggregators");
  spec.postProcessingTasks = readSectionSpec<PostProcessingTaskSpec>(wholeTree, "postprocessing");
  spec.externalTasks = readSectionSpec<ExternalTaskSpec>(wholeTree, "externalTasks");

  return spec;
}

template <>
CommonSpec InfrastructureSpecReader::readSpecEntry<CommonSpec>(const std::string&, const boost::property_tree::ptree& commonTree, const boost::property_tree::ptree&)
{
  CommonSpec spec;
  for (const auto& [key, value] : commonTree.get_child("database")) {
    spec.database.emplace(key, value.get_value<std::string>());
  }
  spec.activityNumber = commonTree.get<int>("Activity.number", spec.activityNumber);
  spec.activityType = commonTree.get<int>("Activity.type", spec.activityType);
  spec.activityPassName = commonTree.get<std::string>("Activity.passName", spec.activityPassName);
  spec.activityPeriodName = commonTree.get<std::string>("Activity.periodName", spec.activityPeriodName);
  spec.activityProvenance = commonTree.get<std::string>("Activity.provenance", spec.activityProvenance);
  spec.activityStart = commonTree.get<uint64_t>("Activity.start", spec.activityStart);
  spec.activityEnd = commonTree.get<uint64_t>("Activity.end", spec.activityEnd);
  spec.monitoringUrl = commonTree.get<std::string>("monitoring.url", spec.monitoringUrl);
  spec.consulUrl = commonTree.get<std::string>("consul.url", spec.consulUrl);
  spec.conditionDBUrl = commonTree.get<std::string>("conditionDB.url", spec.conditionDBUrl);
  spec.infologgerDiscardParameters = {
    commonTree.get<bool>("infologger.filterDiscardDebug", spec.infologgerDiscardParameters.debug),
    commonTree.get<int>("infologger.filterDiscardLevel", spec.infologgerDiscardParameters.fromLevel),
    commonTree.get<std::string>("infologger.filterDiscardFile", spec.infologgerDiscardParameters.discardFile),
    commonTree.get<u_long>("infologger.filterRotateMaxBytes", spec.infologgerDiscardParameters.rotateMaxBytes),
    commonTree.get<u_int>("infologger.filterRotateMaxFiles", spec.infologgerDiscardParameters.rotateMaxFiles)
  };
  spec.postprocessingPeriod = commonTree.get<double>("postprocessing.periodSeconds", spec.postprocessingPeriod);
  spec.bookkeepingUrl = commonTree.get<std::string>("bookkeeping.url", spec.bookkeepingUrl);

  return spec;
}

template <>
TaskSpec InfrastructureSpecReader::readSpecEntry<TaskSpec>(const std::string& taskID, const boost::property_tree::ptree& taskTree, const boost::property_tree::ptree& wholeTree)
{
  static std::unordered_map<std::string, TaskLocationSpec> const taskLocationFromString = {
    { "local", TaskLocationSpec::Local },
    { "remote", TaskLocationSpec::Remote }
  };

  TaskSpec ts;

  ts.taskName = taskTree.get<std::string>("taskName", taskID);
  ts.className = taskTree.get<std::string>("className");
  ts.moduleName = taskTree.get<std::string>("moduleName");
  ts.detectorName = taskTree.get<std::string>("detectorName");
  ts.cycleDurationSeconds = taskTree.get<int>("cycleDurationSeconds", -1);
  if (taskTree.count("cycleDurations") > 0) {
    for (const auto& cycleConfig : taskTree.get_child("cycleDurations")) {
      auto cycleDuration = cycleConfig.second.get<size_t>("cycleDurationSeconds");
      auto validity = cycleConfig.second.get<size_t>("validitySeconds");
      ts.multipleCycleDurations.push_back(std::pair{ cycleDuration, validity });
    }
  }
  ts.dataSource = readSpecEntry<DataSourceSpec>(taskID, taskTree.get_child("dataSource"), wholeTree);
  ts.active = taskTree.get<bool>("active", ts.active);
  ts.maxNumberCycles = taskTree.get<int>("maxNumberCycles", ts.maxNumberCycles);
  ts.resetAfterCycles = taskTree.get<size_t>("resetAfterCycles", ts.resetAfterCycles);
  ts.saveObjectsToFile = taskTree.get<std::string>("saveObjectsToFile", ts.saveObjectsToFile);
  if (taskTree.count("extendedTaskParameters") > 0 && taskTree.count("taskParameters") > 0) {
    ILOG(Warning, Devel) << "Both taskParameters and extendedTaskParameters are defined in the QC config file. We will use only extendedTaskParameters. " << ENDM;
  }
  if (taskTree.count("extendedTaskParameters") > 0) {
    for (const auto& [runtype, subTreeRunType] : taskTree.get_child("extendedTaskParameters")) {
      for (const auto& [beamtype, subTreeBeamType] : subTreeRunType) {
        for (const auto& [key, value] : subTreeBeamType) {
          ts.customParameters.set(key, value.get_value<std::string>(), runtype, beamtype);
        }
      }
    }
  } else if (taskTree.count("taskParameters") > 0) {
    for (const auto& [key, value] : taskTree.get_child("taskParameters")) {
      ts.customParameters.set(key, value.get_value<std::string>());
    }
  }

  bool multinodeSetup = taskTree.find("location") != taskTree.not_found();
  ts.location = taskLocationFromString.at(taskTree.get<std::string>("location", "remote"));
  if (taskTree.count("localMachines") > 0) {
    for (const auto& [key, value] : taskTree.get_child("localMachines")) {
      ts.localMachines.emplace_back(value.get_value<std::string>());
    }
  }
  // fixme: ideally we should print those only when we are running with '--local' and '--remote',
  //  but we do not have access to this information here.
  if (multinodeSetup && taskTree.count("remoteMachine") == 0) {
    ILOG(Warning, Trace)
      << "No remote machine was specified for a multinode QC setup."
         " This is fine if running with AliECS, but it will fail in standalone mode."
      << ENDM;
  }
  ts.remoteMachine = taskTree.get<std::string>("remoteMachine", ts.remoteMachine);
  if (multinodeSetup && ts.location == TaskLocationSpec::Local && taskTree.count("remotePort") == 0) {
    ILOG(Warning, Trace)
      << "No remote port was specified for a task which should use Mergers in a multinode QC setup."
         " This is fine if running with AliECS, but it might fail in standalone mode."
      << ENDM;
  }
  ts.remotePort = taskTree.get<uint16_t>("remotePort", ts.remotePort);
  ts.localControl = taskTree.get<std::string>("localControl", ts.localControl);
  ts.mergingMode = taskTree.get<std::string>("mergingMode", ts.mergingMode);
  ts.mergerCycleMultiplier = taskTree.get<int>("mergerCycleMultiplier", ts.mergerCycleMultiplier);
  if (taskTree.count("mergersPerLayer") > 0) {
    ts.mergersPerLayer.clear();
    for (const auto& [key, value] : taskTree.get_child("mergersPerLayer")) {
      ts.mergersPerLayer.emplace_back(value.get_value<uint64_t>());
    }
  }

  if (taskTree.count("grpGeomRequest") > 0) {
    ts.grpGeomRequestSpec = readSpecEntry<GRPGeomRequestSpec>(ts.taskName, taskTree.get_child("grpGeomRequest"), wholeTree);
  }

  if (taskTree.count("globalTrackingDataRequest") > 0) {
    ts.globalTrackingDataRequest = readSpecEntry<GlobalTrackingDataRequestSpec>(ts.taskName, taskTree.get_child("globalTrackingDataRequest"), wholeTree);
  }

  return ts;
}

template <>
DataSourceSpec InfrastructureSpecReader::readSpecEntry<DataSourceSpec>(const std::string& dataRequestorId,
                                                                       const boost::property_tree::ptree& dataSourceTree,
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
      dss.id = dataSourceTree.get<std::string>("name");
      dss.name = dss.id;
      dss.inputs = DataSampling::InputSpecsForPolicy(wholeTree.get_child("dataSamplingPolicies"), dss.name);
      break;
    }
    case DataSourceType::Direct: {
      auto inputsQuery = dataSourceTree.get<std::string>("query");
      dss.id = inputsQuery;
      dss.name = dss.id;
      dss.inputs = DataDescriptorQueryBuilder::parse(inputsQuery.c_str());
      break;
    }
    case DataSourceType::Task: {
      dss.id = dataSourceTree.get<std::string>("name");
      // this allows us to have tasks with the same name for different detectors
      dss.name = wholeTree.get<std::string>("qc.tasks." + dss.id + ".taskName", dss.id);
      auto detectorName = wholeTree.get<std::string>("qc.tasks." + dss.id + ".detectorName");

      dss.inputs = { { dss.name, TaskRunner::createTaskDataOrigin(detectorName), TaskRunner::createTaskDataDescription(dss.name), 0, Lifetime::Sporadic } };
      if (dataSourceTree.count("MOs") > 0) {
        for (const auto& moName : dataSourceTree.get_child("MOs")) {
          dss.subInputs.push_back(moName.second.get_value<std::string>());
        }
      }
      break;
    }
    case DataSourceType::PostProcessingTask: {
      dss.id = dataSourceTree.get<std::string>("name");
      // this allows us to have tasks with the same name for different detectors
      dss.name = wholeTree.get<std::string>("qc.postprocessing." + dss.id + ".taskName", dss.id);
      auto detectorName = wholeTree.get<std::string>("qc.postprocessing." + dss.id + ".detectorName");
      dss.inputs = { { dss.name, PostProcessingDevice::createPostProcessingDataOrigin(detectorName), PostProcessingDevice::createPostProcessingDataDescription(dss.id), 0, Lifetime::Sporadic } };
      if (dataSourceTree.count("MOs") > 0) {
        for (const auto& moName : dataSourceTree.get_child("MOs")) {
          dss.subInputs.push_back(moName.second.get_value<std::string>());
        }
      }
      break;
    }
    case DataSourceType::Check: {
      dss.id = dataSourceTree.get<std::string>("name");
      dss.name = dss.id;
      dss.inputs = { { dss.name, "QC", Check::createCheckDataDescription(dss.name), 0, Lifetime::Sporadic } };
      if (dataSourceTree.count("QOs") > 0) {
        for (const auto& moName : dataSourceTree.get_child("QOs")) {
          dss.subInputs.push_back(moName.second.get_value<std::string>());
        }
      }
      break;
    }
    case DataSourceType::Aggregator: {
      dss.name = dataSourceTree.get<std::string>("name");
      dss.inputs = { { dss.name, "QC", AggregatorRunner::createAggregatorRunnerDataDescription(dss.name), 0, Lifetime::Sporadic } };
      if (dataSourceTree.count("QOs") > 0) {
        for (const auto& moName : dataSourceTree.get_child("QOs")) {
          dss.subInputs.push_back(moName.second.get_value<std::string>());
        }
      }
      break;
    }
    case DataSourceType::ExternalTask: {
      dss.id = dataSourceTree.get<std::string>("name");
      dss.name = dss.id;
      auto query = wholeTree.get<std::string>("qc.externalTasks." + dss.name + ".query");
      dss.inputs = o2::framework::DataDescriptorQueryBuilder::parse(query.c_str());
      break;
    }
    case DataSourceType::Invalid:
      // todo: throw?
      break;
  }

  return dss;
}

template <>
CheckSpec InfrastructureSpecReader::readSpecEntry<CheckSpec>(const std::string& checkID, const boost::property_tree::ptree& checkTree, const boost::property_tree::ptree& wholeTree)
{
  CheckSpec cs;

  cs.checkName = checkTree.get<std::string>("checkName", checkID);
  cs.className = checkTree.get<std::string>("className");
  cs.moduleName = checkTree.get<std::string>("moduleName");
  cs.detectorName = checkTree.get<std::string>("detectorName", cs.detectorName);

  // errors of the past
  const auto& dataSourcesTree = checkTree.count("dataSource") > 0 ? checkTree.get_child("dataSource") : checkTree.get_child("dataSources");
  for (const auto& [_key, dataSourceTree] : dataSourcesTree) {
    (void)_key;
    cs.dataSources.push_back(readSpecEntry<DataSourceSpec>(checkID, dataSourceTree, wholeTree));
  }

  if (auto policy = checkTree.get_optional<std::string>("policy"); policy.has_value()) {
    cs.updatePolicy = UpdatePolicyTypeUtils::FromString(policy.get());
  }

  cs.active = checkTree.get<bool>("active", cs.active);
  if (checkTree.count("extendedCheckParameters") > 0) {
    for (const auto& [runtype, subTreeRunType] : checkTree.get_child("extendedCheckParameters")) {
      for (const auto& [beamtype, subTreeBeamType] : subTreeRunType) {
        for (const auto& [key, value] : subTreeBeamType) {
          cs.customParameters.set(key, value.get_value<std::string>(), runtype, beamtype);
        }
      }
    }
  }
  if (checkTree.count("checkParameters") > 0) {
    for (const auto& [key, value] : checkTree.get_child("checkParameters")) {
      cs.customParameters.set(key, value.get_value<std::string>());
    }
  }

  return cs;
}

template <>
AggregatorSpec InfrastructureSpecReader::readSpecEntry<AggregatorSpec>(const std::string& aggregatorID, const boost::property_tree::ptree& aggregatorTree, const boost::property_tree::ptree& wholeTree)
{
  AggregatorSpec as;

  as.aggregatorName = aggregatorTree.get<std::string>("checkName", aggregatorID);
  as.className = aggregatorTree.get<std::string>("className");
  as.moduleName = aggregatorTree.get<std::string>("moduleName");
  as.detectorName = aggregatorTree.get<std::string>("detectorName", as.detectorName);

  // errors of the past
  const auto& dataSourcesTree = aggregatorTree.count("dataSource") > 0 ? aggregatorTree.get_child("dataSource") : aggregatorTree.get_child("dataSources");
  for (const auto& [_key, dataSourceTree] : dataSourcesTree) {
    (void)_key;
    as.dataSources.push_back(readSpecEntry<DataSourceSpec>(aggregatorID, dataSourceTree, wholeTree));
  }

  if (auto policy = aggregatorTree.get_optional<std::string>("policy"); policy.has_value()) {
    as.updatePolicy = UpdatePolicyTypeUtils::FromString(policy.get());
  }

  as.active = aggregatorTree.get<bool>("active", as.active);
  if (aggregatorTree.count("extendedAggregatorParameters") > 0) {
    for (const auto& [runtype, subTreeRunType] : aggregatorTree.get_child("extendedAggregatorParameters")) {
      for (const auto& [beamtype, subTreeBeamType] : subTreeRunType) {
        for (const auto& [key, value] : subTreeBeamType) {
          as.customParameters.set(key, value.get_value<std::string>(), runtype, beamtype);
        }
      }
    }
  }
  if (aggregatorTree.count("aggregatorParameters") > 0) {
    for (const auto& [key, value] : aggregatorTree.get_child("aggregatorParameters")) {
      as.customParameters.set(key, value.get_value<std::string>());
    }
  }
  return as;
}

template <>
PostProcessingTaskSpec
  InfrastructureSpecReader::readSpecEntry<PostProcessingTaskSpec>(const std::string& ppTaskId, const boost::property_tree::ptree& ppTaskTree, const boost::property_tree::ptree& wholeTree)
{
  PostProcessingTaskSpec ppts;

  ppts.id = ppTaskId;
  ppts.taskName = ppTaskTree.get<std::string>("taskName", ppts.id);
  ppts.active = ppTaskTree.get<bool>("active", ppts.active);
  ppts.detectorName = ppTaskTree.get<std::string>("detectorName", ppts.detectorName);
  ppts.tree = wholeTree;

  return ppts;
}

template <>
ExternalTaskSpec
  InfrastructureSpecReader::readSpecEntry<ExternalTaskSpec>(const std::string& externalTaskName, const boost::property_tree::ptree& externalTaskTree, const boost::property_tree::ptree&)
{
  ExternalTaskSpec ets;

  ets.taskName = externalTaskName;
  ets.query = externalTaskTree.get<std::string>("query");
  ets.active = externalTaskTree.get<bool>("active", ets.active);

  return ets;
}

template <>
GRPGeomRequestSpec
  InfrastructureSpecReader::readSpecEntry<GRPGeomRequestSpec>(const std::string&, const boost::property_tree::ptree& grpGeomRequestTree, const boost::property_tree::ptree&)
{
  GRPGeomRequestSpec grpSpec;
  grpSpec.geomRequest = grpGeomRequestTree.get<std::string>("geomRequest", grpSpec.geomRequest);
  grpSpec.askGRPECS = grpGeomRequestTree.get<bool>("askGRPECS", grpSpec.askGRPECS);
  grpSpec.askGRPLHCIF = grpGeomRequestTree.get<bool>("askGRPLHCIF", grpSpec.askGRPLHCIF);
  grpSpec.askGRPMagField = grpGeomRequestTree.get<bool>("askGRPMagField", grpSpec.askGRPMagField);
  grpSpec.askMatLUT = grpGeomRequestTree.get<bool>("askMatLUT", grpSpec.askMatLUT);
  grpSpec.askTime = grpGeomRequestTree.get<bool>("askTime", grpSpec.askTime);
  grpSpec.askOnceAllButField = grpGeomRequestTree.get<bool>("askOnceAllButField", grpSpec.askOnceAllButField);
  grpSpec.needPropagatorD = grpGeomRequestTree.get<bool>("needPropagatorD", grpSpec.needPropagatorD);

  return grpSpec;
}

template <>
GlobalTrackingDataRequestSpec
  InfrastructureSpecReader::readSpecEntry<GlobalTrackingDataRequestSpec>(const std::string&, const boost::property_tree::ptree& dataRequestTree, const boost::property_tree::ptree&)
{
  GlobalTrackingDataRequestSpec gtdrSpec;
  gtdrSpec.canProcessTracks = dataRequestTree.get<std::string>("canProcessTracks", gtdrSpec.canProcessTracks);
  gtdrSpec.requestTracks = dataRequestTree.get<std::string>("requestTracks", gtdrSpec.requestTracks);
  gtdrSpec.canProcessClusters = dataRequestTree.get<std::string>("canProcessClusters", gtdrSpec.canProcessClusters);
  gtdrSpec.requestClusters = dataRequestTree.get<std::string>("requestClusters", gtdrSpec.requestClusters);
  gtdrSpec.mc = dataRequestTree.get<bool>("mc", gtdrSpec.mc);

  return gtdrSpec;
}

std::string InfrastructureSpecReader::validateDetectorName(std::string name)
{
  // name must be a detector code from DetID or one of the few allowed general names
  int nDetectors = 17;
  const char* detNames[17] = // once we can use DetID, remove this hard-coded list
    { "ITS", "TPC", "TRD", "TOF", "PHS", "CPV", "EMC", "HMP", "MFT", "MCH", "MID", "ZDC", "FT0", "FV0", "FDD", "ACO", "FOC" };
  std::vector<std::string> permitted = { "MISC", "DAQ", "GENERAL", "TST", "BMK", "CTP", "TRG", "DCS", "GLO", "FIT" };
  for (auto i = 0; i < nDetectors; i++) {
    permitted.emplace_back(detNames[i]);
    //    permitted.push_back(o2::detectors::DetID::getName(i));
  }
  auto it = std::find(permitted.begin(), permitted.end(), name);

  if (it == permitted.end()) {
    std::string permittedString;
    for (const auto& i : permitted)
      permittedString += i + ' ';
    ILOG(Error, Support) << "Invalid detector name : " << name << ". Placeholder 'MISC' will be used instead. Note: list of permitted detector names :" << permittedString << ENDM;
    return "MISC";
  }
  return name;
}

template <typename T>
T InfrastructureSpecReader::readSpecEntry(const std::string&, const boost::property_tree::ptree&, const boost::property_tree::ptree&)
{
  throw std::runtime_error("Unknown entry type: " + std::string(typeid(T).name()));
}

} // namespace o2::quality_control::core
