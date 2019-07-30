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
/// \file   TaskInterface.cxx
/// \author Piotr Konopka
/// \author Barthelemy von Haller
///

#include "QualityControl/TaskInterface.h"

namespace o2::quality_control::core
{

TaskInterface::TaskInterface(ObjectsManager* objectsManager) : mObjectsManager(objectsManager) {}

const std::string& TaskInterface::getName() const { return mName; }

void TaskInterface::setName(const std::string& name) { mName = name; }

void TaskInterface::setObjectsManager(std::shared_ptr<ObjectsManager> objectsManager)
{
  mObjectsManager = objectsManager;
}

void TaskInterface::setCustomParameters(const std::unordered_map<std::string, std::string>& parameters)
{
  mCustomParameters = parameters;
}

std::shared_ptr<ObjectsManager> TaskInterface::getObjectsManager() { return mObjectsManager; }

} // namespace o2::quality_control::core
