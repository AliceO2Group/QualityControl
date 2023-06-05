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

namespace o2::quality_control::core
{

TaskInterface::TaskInterface(ObjectsManager* objectsManager) : mObjectsManager(objectsManager)
{
}

void TaskInterface::setObjectsManager(std::shared_ptr<ObjectsManager> objectsManager)
{
  mObjectsManager = objectsManager;
}

std::shared_ptr<ObjectsManager> TaskInterface::getObjectsManager() { return mObjectsManager; }

void TaskInterface::setMonitoring(const std::shared_ptr<o2::monitoring::Monitoring>& mMonitoring)
{
  TaskInterface::mMonitoring = mMonitoring;
}

void TaskInterface::setGlobalTrackingDataRequest(std::shared_ptr<o2::globaltracking::DataRequest> request)
{
  mGlobalTrackingDataRequest = std::move(request);
}

const o2::globaltracking::DataRequest* TaskInterface::getGlobalTrackingDataRequest() const
{
  return mGlobalTrackingDataRequest.get();
}

void TaskInterface::configure()
{
  // noop, override it if you want.
}

} // namespace o2::quality_control::core
