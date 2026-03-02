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
/// \file   TaskFactory.cxx
/// \author Barthelemy von Haller
///

#include "QualityControl/TaskFactory.h"

#include "QualityControl/RootClassFactory.h"

namespace o2::quality_control::core
{

TaskInterface* TaskFactory::create(const TaskRunnerConfig& taskConfig, std::shared_ptr<ObjectsManager> objectsManager)
{
  auto* result = root_class_factory::create<TaskInterface>(taskConfig.moduleName, taskConfig.className);
  result->setName(taskConfig.name);
  result->setObjectsManager(objectsManager);
  result->setCustomParameters(taskConfig.customParameters);
  result->setCcdbUrl(taskConfig.ccdbUrl);

  return result;
}

} // namespace o2::quality_control::core
