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
/// \file   PostProcessingConfig.cxx
/// \author Piotr Konopka
///

#include "QualityControl/PostProcessingConfig.h"

#include <boost/property_tree/ptree.hpp>

namespace o2::quality_control::postprocessing
{

PostProcessingConfig::PostProcessingConfig(const std::string& id, const boost::property_tree::ptree& config) //
  : id(id),
    taskName(config.get<std::string>("qc.postprocessing." + id + ".taskName", id)),
    activity(config.get<int>("qc.config.Activity.number", 0),
             config.get<std::string>("qc.config.Activity.type", "NONE"),
             config.get<std::string>("qc.config.Activity.periodName", ""),
             config.get<std::string>("qc.config.Activity.passName", ""),
             config.get<std::string>("qc.config.Activity.provenance", "qc"),
             { config.get<uint64_t>("qc.config.Activity.start", 0),
               config.get<uint64_t>("qc.config.Activity.end", -1) }),
    matchAnyRunNumber(config.get<bool>("qc.config.postprocessing.matchAnyRunNumber", false)),
    critical(true)
{
  moduleName = config.get<std::string>("qc.postprocessing." + id + ".moduleName");
  className = config.get<std::string>("qc.postprocessing." + id + ".className");
  detectorName = config.get<std::string>("qc.postprocessing." + id + ".detectorName", "MISC");
  ccdbUrl = config.get<std::string>("qc.config.conditionDB.url", "");
  consulUrl = config.get<std::string>("qc.config.consul.url", "");
  // if available, use the source repo as defined in the postprocessing task, otherwise the general QCDB
  auto sourceRepo = config.get_child_optional("qc.postprocessing." + id + ".sourceRepo");
  auto databasePath = sourceRepo ? "qc.postprocessing." + id + ".sourceRepo" : "qc.config.database";
  auto qcdbUrl = config.get<std::string>(databasePath + ".implementation") == "CCDB" ? config.get<std::string>(databasePath + ".host") : "";
  // build the config of the qcdb
  std::unordered_map<std::string, std::string> dbConfig{
    { "implementation", config.get<std::string>("qc.config.database.implementation") },
    { "host", qcdbUrl }
  };
  repository = dbConfig;

  for (const auto& initTrigger : config.get_child("qc.postprocessing." + id + ".initTrigger")) {
    initTriggers.push_back(initTrigger.second.get_value<std::string>());
  }
  for (const auto& updateTrigger : config.get_child("qc.postprocessing." + id + ".updateTrigger")) {
    updateTriggers.push_back(updateTrigger.second.get_value<std::string>());
  }
  for (const auto& stopTrigger : config.get_child("qc.postprocessing." + id + ".stopTrigger")) {
    stopTriggers.push_back(stopTrigger.second.get_value<std::string>());
  }
  auto ppTree = config.get_child("qc.postprocessing." + id);
  if (ppTree.count("extendedTaskParameters")) {
    customParameters.populateCustomParameters(ppTree.get_child("extendedTaskParameters"));
  } else if (ppTree.count("taskParameters") > 0) {
    for (const auto& [key, value] : ppTree.get_child("taskParameters")) {
      customParameters.set(key, value.get_value<std::string>());
    }
  }
}

} // namespace o2::quality_control::postprocessing
