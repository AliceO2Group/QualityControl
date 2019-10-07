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
/// \file   TaskFactory.cxx
/// \author Barthelemy von Haller
///

#include "QualityControl/TaskFactory.h"

#include "QualityControl/QcInfoLogger.h"
#include "RootClassFactory.h"

namespace o2::quality_control::core
{

TaskInterface* TaskFactory::create(TaskConfig& taskConfig, std::shared_ptr<ObjectsManager> objectsManager)
{
  TaskInterface* result = root_class_factory::create<TaskInterface>(taskConfig.moduleName, taskConfig.className);
  result->setName(taskConfig.taskName);
  result->setObjectsManager(objectsManager);
  result->setCustomParameters(taskConfig.customParameters);

  return result;
}

} // namespace o2::quality_control::core
