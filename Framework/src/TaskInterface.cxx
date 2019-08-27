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

void TaskInterface::initializeBase()
{
  if (!mCcdbApi) {
    mCcdbApi = std::make_shared<CcdbApi>();
  }

  mCcdbApi->init(mCcdbUrl);
  if (!mCcdbApi->isHostReachable()) {
    LOG(WARN) << "CCDB at URL '" << mCcdbUrl << "' is not reachable.";
  }
}

void TaskInterface::setCcdbHost(const std::string& url)
{
  mCcdbUrl = url;
}

void TaskInterface::setCustomParameters(const std::unordered_map<std::string, std::string>& parameters)
{
  mCustomParameters = parameters;
}

TObject* TaskInterface::retrieveCondition(std::string path, std::map<std::string, std::string> metadata, long timestamp)
{
  if (mCcdbApi) {
    return mCcdbApi->retrieve(path, metadata, timestamp);
  } else {
    LOG(ERROR) << "Trying to retrieve a condition, but CCDB API is not constructed.";
    return nullptr;
  }
}

std::shared_ptr<ObjectsManager> TaskInterface::getObjectsManager() { return mObjectsManager; }

} // namespace o2::quality_control::core
