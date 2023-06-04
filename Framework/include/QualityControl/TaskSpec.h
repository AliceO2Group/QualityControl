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

#ifndef QUALITYCONTROL_TASKSPEC_H
#define QUALITYCONTROL_TASKSPEC_H

///
/// \file   TaskSpec.h
/// \author Piotr Konopka
///

#include <string>
#include <vector>

#include "QualityControl/DataSourceSpec.h"
#include "QualityControl/RecoRequestSpecs.h"
#include "QualityControl/CustomParameters.h"

namespace o2::quality_control::core
{

enum class TaskLocationSpec {
  Local,
  Remote
};

/// \brief Specification of a Task, which should map the JSON configuration structure.
struct TaskSpec {
  // default, invalid spec
  TaskSpec() = default;

  // minimal valid spec
  TaskSpec(std::string taskName, std::string className, std::string moduleName, std::string detectorName,
           int cycleDurationSeconds, DataSourceSpec dataSource)
    : taskName(std::move(taskName)),
      className(std::move(className)),
      moduleName(std::move(moduleName)),
      detectorName(std::move(detectorName)),
      cycleDurationSeconds(cycleDurationSeconds),
      dataSource(std::move(dataSource))
  {
  }

  // basic
  std::string taskName = "Invalid";
  std::string className = "Invalid";
  std::string moduleName = "Invalid";
  std::string detectorName = "Invalid";
  int cycleDurationSeconds = -1;                                      // simple syntax
  std::vector<std::pair<size_t, size_t>> multipleCycleDurations = {}; // complex syntax: multiple durations can be set for different intervals
  DataSourceSpec dataSource;
  // advanced
  bool active = true;
  int maxNumberCycles = -1;
  size_t resetAfterCycles = 0;
  std::string saveObjectsToFile;
  core::CustomParameters customParameters;
  // multinode setups
  TaskLocationSpec location = TaskLocationSpec::Remote;
  std::vector<std::string> localMachines = {};
  std::string remoteMachine = "any";
  uint16_t remotePort = 36543;
  std::string localControl = "aliecs";
  std::string mergingMode = "delta"; // todo as enum?
  int mergerCycleMultiplier = 1;
  std::vector<size_t> mergersPerLayer{ 1 };
  GRPGeomRequestSpec grpGeomRequestSpec;
  GlobalTrackingDataRequestSpec globalTrackingDataRequest;
};

} // namespace o2::quality_control::core

#endif // QUALITYCONTROL_TASKSPEC_H
