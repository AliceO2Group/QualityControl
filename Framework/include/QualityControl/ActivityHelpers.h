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
/// \file   ActivityHelpers.h
/// \author Piotr Konopka
///

#ifndef QUALITYCONTROL_ACTIVITYHELPERS_H
#define QUALITYCONTROL_ACTIVITYHELPERS_H

#include "QualityControl/Activity.h"

#include <algorithm>
#include <map>
#include <string>
#include <ranges>
#include <boost/property_tree/ptree_fwd.hpp>

namespace o2::quality_control::core::activity_helpers
{

template <typename Range>
concept RangeOfActivities = requires(Range range)
{
  requires std::same_as<std::ranges::range_value_t<Range>, Activity>;
};

namespace implementation
{
Activity commonActivityFields(const RangeOfActivities auto& activities);
}

/// Produces the most constrained Activity which will match all the provided
Activity strictestMatchingActivity(const RangeOfActivities auto& activities)
{
  if (std::ranges::empty(activities)) {
    return {};
  }
  if (std::ranges::size(activities) == 1) {
    return *std::ranges::begin(activities);
  }
  Activity result = implementation::commonActivityFields(activities);

  result.mValidity = (*std::ranges::begin(activities)).mValidity;
  for (const Activity& activity : activities | std::views::drop(1)) {
    const auto& validity = activity.mValidity;
    result.mValidity.update(validity.getMin());
    result.mValidity.update(validity.getMax());
  }
  return result;
}

/// Produces an Activity which matches all the provided, but the validity is an intersection.
/// Be sure to check if the result validity is valid, it might not if the argument validities do not overlap
Activity overlappingActivity(const RangeOfActivities auto& activities)
{
  if (std::ranges::empty(activities)) {
    return {};
  }
  if (std::ranges::size(activities) == 1) {
    return *std::ranges::begin(activities);
  }

  Activity result = implementation::commonActivityFields(activities);
  result.mValidity = (*std::ranges::begin(activities)).mValidity;
  for (const Activity& activity : activities | std::views::drop(1)) {
    result.mValidity = result.mValidity.getOverlap(activity.mValidity);
  }

  return result;
}

std::map<std::string, std::string> asDatabaseMetadata(const core::Activity&, bool putDefault = true);
core::Activity asActivity(const std::map<std::string, std::string>& metadata, const std::string& provenance = "qc");
core::Activity asActivity(const boost::property_tree::ptree&, const std::string& provenance = "qc");

std::function<validity_time_t(void)> getCcdbSorTimeAccessor(uint64_t runNumber);
std::function<validity_time_t(void)> getCcdbEorTimeAccessor(uint64_t runNumber);

/// \brief checks if the provided validity uses old rules, where start is creation time, end is 10 years in the future.
bool isLegacyValidity(ValidityInterval);

bool onNumericLimit(validity_time_t timestamp);

namespace implementation
{

template <RangeOfActivities RangeType, typename FieldType>
void setMemberIfCommon(Activity& result, const RangeType& activities, FieldType Activity::*memberPtr)
{
  const Activity& first = *std::ranges::begin(activities);
  if (std::ranges::all_of(activities | std::views::drop(1), [&](const auto& other) { return first.*memberPtr == other.*memberPtr; })) {
    result.*memberPtr = first.*memberPtr;
  }
}

Activity commonActivityFields(const RangeOfActivities auto& activities)
{
  Activity result;
  setMemberIfCommon(result, activities, &Activity::mId);
  setMemberIfCommon(result, activities, &Activity::mType);
  setMemberIfCommon(result, activities, &Activity::mPassName);
  setMemberIfCommon(result, activities, &Activity::mPeriodName);
  setMemberIfCommon(result, activities, &Activity::mProvenance);
  setMemberIfCommon(result, activities, &Activity::mValidity);
  setMemberIfCommon(result, activities, &Activity::mBeamType);
  setMemberIfCommon(result, activities, &Activity::mPartitionName);
  setMemberIfCommon(result, activities, &Activity::mFillNumber);
  return result;
}

} // namespace implementation
} // namespace o2::quality_control::core

#endif // QUALITYCONTROL_ACTIVITYHELPERS_H
