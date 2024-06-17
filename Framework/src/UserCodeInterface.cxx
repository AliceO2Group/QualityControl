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
/// \file   UserCodeInterface.cxx
/// \author Barthelemy von Haller
///

#include "QualityControl/UserCodeInterface.h"
#include "QualityControl/RepoPathUtils.h"
#include "QualityControl/DatabaseFactory.h"

using namespace o2::ccdb;

namespace o2::quality_control::core
{

void UserCodeInterface::setCustomParameters(const CustomParameters& parameters)
{
  mCustomParameters = parameters;
  configure();
}

const std::string& UserCodeInterface::getName() const { return mName; }
void UserCodeInterface::setName(const std::string& name) { mName = name; }

const std::shared_ptr<o2::quality_control::repository::DatabaseInterface>& UserCodeInterface::getDatabase() const
{
  return mDatabase;
}
void UserCodeInterface::setDatabase(const std::shared_ptr<o2::quality_control::repository::DatabaseInterface>& mDatabase)
{
  UserCodeInterface::mDatabase = mDatabase;
}

std::shared_ptr<MonitorObject> UserCodeInterface::retrieveReference(string provenance, string detector, string taskName, string objectName, long timestamp, const core::Activity& activity, map<string, string> metadataFilters) // we don't use Activity because users might have an object that applies to a wide range of activities and it would be very limiting
{
  // Get the reference object from <provenance>/<det>/REF
  auto path = RepoPathUtils::getRefPath(detector, taskName, "", provenance, false);
  auto refObject = mDatabase->retrieveMO(path, objectName, repository::DatabaseInterface::Timestamp::Current, activity, metadataFilters);
  return refObject;
}

} // namespace o2::quality_control::core