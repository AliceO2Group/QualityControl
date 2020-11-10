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
/// \file   TaskConfig.h
/// \author Barthelemy von Haller
///

#ifndef QC_CORE_TASKCONFIG_H
#define QC_CORE_TASKCONFIG_H

#include <string>
#include <unordered_map>

namespace o2::quality_control::core
{

/// \brief  Container for the configuration of a Task
struct TaskConfig {
  std::string taskName;
  std::string moduleName;
  std::string className;
  int cycleDurationSeconds;
  int maxNumberCycles;
  std::string consulUrl;
  std::string conditionUrl = "";
  std::unordered_map<std::string, std::string> customParameters = {};
  std::string detectorName = "MISC"; // intended to be the 3 letters code
  int parallelTaskID = 0;            // ID to differentiate parallel local Tasks from one another. 0 means this is the only one.
  std::string saveToFile = "";
};

} // namespace o2::quality_control::core

#endif // QC_CORE_TASKCONFIG_H
