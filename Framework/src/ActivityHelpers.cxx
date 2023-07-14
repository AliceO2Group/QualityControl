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
/// \file   ActivityHelpers.cxx
/// \author Piotr Konopka
///

#include "QualityControl/ActivityHelpers.h"

#include <boost/property_tree/ptree.hpp>
#include <CCDB/BasicCCDBManager.h>
#include "QualityControl/ObjectMetadataKeys.h"

using namespace o2::quality_control::repository;

namespace o2::quality_control::core::activity_helpers
{

std::map<std::string, std::string> asDatabaseMetadata(const core::Activity& activity, bool putDefault)
{
  std::map<std::string, std::string> metadata;
  if (putDefault || activity.mType != 0) {
    // TODO should we really treat 0 as none?
    //  we could consider making Activity use std::optional to be clear about this
    metadata[metadata_keys::runType] = std::to_string(activity.mType);
  }
  if (putDefault || activity.mId != 0) {
    metadata[metadata_keys::runNumber] = std::to_string(activity.mId);
  }
  if (putDefault || !activity.mPassName.empty()) {
    metadata[metadata_keys::passName] = activity.mPassName;
  }
  if (putDefault || !activity.mPeriodName.empty()) {
    metadata[metadata_keys::periodName] = activity.mPeriodName;
  }
  return metadata;
}

core::Activity asActivity(const std::map<std::string, std::string>& metadata, const std::string& provenance)
{
  core::Activity activity;
  if (auto runType = metadata.find(metadata_keys::runType); runType != metadata.end()) {
    activity.mType = std::strtol(runType->second.c_str(), nullptr, 10);
  }
  if (auto runNumber = metadata.find(metadata_keys::runNumber); runNumber != metadata.end()) {
    activity.mId = std::strtol(runNumber->second.c_str(), nullptr, 10);
  }
  if (auto passName = metadata.find(metadata_keys::passName); passName != metadata.end()) {
    activity.mPassName = passName->second;
  }
  if (auto periodName = metadata.find(metadata_keys::periodName); periodName != metadata.end()) {
    activity.mPeriodName = periodName->second;
  }
  if (auto validFrom = metadata.find(metadata_keys::validFrom); validFrom != metadata.end()) {
    activity.mValidity.setMin(std::stoull(validFrom->second));
  }
  if (auto validUntil = metadata.find(metadata_keys::validUntil); validUntil != metadata.end()) {
    activity.mValidity.setMax(std::stoull(validUntil->second));
  }
  activity.mProvenance = provenance;
  return activity;
}

core::Activity asActivity(const boost::property_tree::ptree& tree, const std::string& provenance)
{
  core::Activity activity;
  if (auto runType = tree.get_optional<int>(metadata_keys::runType); runType.has_value()) {
    activity.mType = runType.value();
  }
  if (auto runNumber = tree.get_optional<int>(metadata_keys::runNumber); runNumber.has_value()) {
    activity.mId = runNumber.value();
  }
  if (auto passName = tree.get_optional<std::string>(metadata_keys::passName); passName.has_value()) {
    activity.mPassName = passName.value();
  }
  if (auto periodName = tree.get_optional<std::string>(metadata_keys::periodName); periodName.has_value()) {
    activity.mPeriodName = periodName.value();
  }
  if (auto validFrom = tree.get_optional<core::validity_time_t>(metadata_keys::validFrom); validFrom.has_value()) {
    activity.mValidity.setMin(validFrom.value());
  }
  if (auto validUntil = tree.get_optional<core::validity_time_t>(metadata_keys::validUntil); validUntil.has_value()) {
    activity.mValidity.setMax(validUntil.value());
  }
  activity.mProvenance = provenance;
  return activity;
}

std::function<validity_time_t(void)> getCcdbSorTimeAccessor(uint64_t runNumber)
{
  return [runNumber]() { return static_cast<validity_time_t>(ccdb::BasicCCDBManager::instance().getRunDuration(runNumber, false).first); };
}

std::function<validity_time_t(void)> getCcdbEorTimeAccessor(uint64_t runNumber)
{
  return [runNumber]() {
    return static_cast<validity_time_t>(ccdb::BasicCCDBManager::instance().getRunDuration(runNumber, false).second);
  };
}

bool isLegacyValidity(ValidityInterval validity)
{
  return validity.isValid() && validity.delta() > 9ull * 365 * 24 * 60 * 60 * 1000ull;
}

} // namespace o2::quality_control::core::activity_helpers