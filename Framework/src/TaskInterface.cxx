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
/// \file   TaskInterface.cxx
/// \author Piotr Konopka
/// \author Barthelemy von Haller
///

#include "QualityControl/TaskInterface.h"
#include "QualityControl/stringUtils.h"
#include <CCDB/CcdbApi.h>

namespace o2::quality_control::core
{

TaskInterface::TaskInterface(ObjectsManager* objectsManager) : mObjectsManager(objectsManager)
{
}

const std::string& TaskInterface::getName() const { return mName; }

void TaskInterface::setName(const std::string& name) { mName = name; }

void TaskInterface::setObjectsManager(std::shared_ptr<ObjectsManager> objectsManager)
{
  mObjectsManager = objectsManager;
}

std::shared_ptr<ObjectsManager> TaskInterface::getObjectsManager() { return mObjectsManager; }

void TaskInterface::setMonitoring(const std::shared_ptr<o2::monitoring::Monitoring>& mMonitoring)
{
  TaskInterface::mMonitoring = mMonitoring;
}

void TaskInterface::configure(std::string /*name*/)
{
  // noop, override it if you want.
}

} // namespace o2::quality_control::core
