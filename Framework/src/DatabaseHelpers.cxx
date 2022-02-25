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
/// \file   DatabaseHelpers.cxx
/// \author Piotr Konopka
///

#include "QualityControl/DatabaseHelpers.h"

namespace o2::quality_control::repository::database_helpers
{

std::map<std::string, std::string> asDatabaseMetadata(const core::Activity& activity)
{
  std::map<std::string, std::string> metadata;
  if (activity.mType != 0) {
    // TODO should we really treat 0 as none?
    //  we could consider making Activity use std::optional to be clear about this
    metadata["RunType"] = std::to_string(activity.mType);
  }
  if (activity.mId != 0) {
    metadata["RunNumber"] = std::to_string(activity.mId);
  }
  if (!activity.mPassName.empty()) {
    metadata["PassName"] = activity.mPassName;
  }
  if (!activity.mPeriodName.empty()) {
    metadata["PeriodName"] = activity.mPeriodName;
  }
  return metadata;
}

core::Activity asActivity(const std::map<std::string, std::string>& metadata, const std::string& provenance)
{
  core::Activity activity;
  if (auto runType = metadata.find("RunType"); runType != metadata.end()) {
    activity.mType = std::strtol(runType->second.c_str(), nullptr, 10);
  }
  if (auto runNumber = metadata.find("RunNumber"); runNumber != metadata.end()) {
    activity.mId = std::strtol(runNumber->second.c_str(), nullptr, 10);
  }
  if (auto passName = metadata.find("PassName"); passName != metadata.end()) {
    activity.mPassName = passName->second;
  }
  if (auto periodName = metadata.find("PeriodName"); periodName != metadata.end()) {
    activity.mPeriodName = periodName->second;
  }
  activity.mProvenance = provenance;
  return activity;
}

} // namespace o2::quality_control::repository::database_helpers