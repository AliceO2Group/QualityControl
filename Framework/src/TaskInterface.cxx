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
#include <CCDB/CcdbApi.h>

using namespace o2::ccdb;

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

void TaskInterface::loadCcdb()
{
  if (!mCcdbApi) {
    mCcdbApi = std::make_shared<CcdbApi>();
  }

  mCcdbApi->init(mCcdbUrl);
  if (!mCcdbApi->isHostReachable()) {
    ILOG(Warning, Support) << "CCDB at URL '" << mCcdbUrl << "' is not reachable." << ENDM;
  }
}

void TaskInterface::setCustomParameters(const std::unordered_map<std::string, std::string>& parameters)
{
  mCustomParameters = parameters;
}

TObject* TaskInterface::retrieveCondition(std::string path, std::map<std::string, std::string> metadata, long timestamp)
{
  if (!mCcdbApi) {
    loadCcdb();
  }

  return mCcdbApi->retrieveFromTFileAny<TObject>(path, metadata, timestamp);
}

std::shared_ptr<ObjectsManager> TaskInterface::getObjectsManager() { return mObjectsManager; }

void TaskInterface::setMonitoring(const std::shared_ptr<o2::monitoring::Monitoring>& mMonitoring)
{
  TaskInterface::mMonitoring = mMonitoring;
}

void TaskInterface::setCcdbUrl(const std::string& url)
{
  mCcdbUrl = url;
}

} // namespace o2::quality_control::core
