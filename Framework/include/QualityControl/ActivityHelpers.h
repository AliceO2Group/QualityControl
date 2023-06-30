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
#include <boost/property_tree/ptree_fwd.hpp>

namespace o2::quality_control::core::activity_helpers
{

namespace implementation
{

template <typename Iter, typename Accessor = const Activity& (*)(const Activity&)>
void commonActivityFields(Iter begin, Iter end, Accessor accessor, Activity& result);

}

/// Produces the most constrained Activity which will match all the provided
template <typename Iter, typename Accessor = const Activity& (*)(const Activity&)>
Activity strictestMatchingActivity(
  Iter begin, Iter end, Accessor accessor = [](const Activity& a) -> const Activity& { return a; })
{
  if (begin == end) {
    return {};
  }
  if (std::next(begin) == end) {
    return accessor(*begin);
  }
  Activity result;
  implementation::commonActivityFields(begin, end, accessor, result);

  result.mValidity = accessor(*begin).mValidity;
  std::for_each(std::next(begin), end, [&](const auto& item) {
    const auto& validity = accessor(item).mValidity;
    result.mValidity.update(validity.getMin());
    result.mValidity.update(validity.getMax());
  });

  return result;
}

/// Produces an Activity which matches all the provided, but the validity is a union.
/// Be sure to check if the result validity is valid, it might not if the argument validities do not overlap
template <typename Iter, typename Accessor = const Activity& (*)(const Activity&)>
Activity overlappingActivity(
  Iter begin, Iter end, Accessor accessor = [](const Activity& a) -> const Activity& { return a; })
{
  if (begin == end) {
    return {};
  }
  if (std::next(begin) == end) {
    return accessor(*begin);
  }

  Activity result;
  implementation::commonActivityFields(begin, end, accessor, result);

  result.mValidity = accessor(*begin).mValidity;
  std::for_each(std::next(begin), end, [&](const auto& item) {
    const auto& validity = accessor(item).mValidity;
    result.mValidity = result.mValidity.getOverlap(validity);
  });

  return result;
}

std::map<std::string, std::string> asDatabaseMetadata(const core::Activity&, bool putDefault = true);
core::Activity asActivity(const std::map<std::string, std::string>& metadata, const std::string& provenance = "qc");
core::Activity asActivity(const boost::property_tree::ptree&, const std::string& provenance = "qc");

std::function<validity_time_t(void)> getCcdbSorTimeAccessor(uint64_t runNumber);
std::function<validity_time_t(void)> getCcdbEorTimeAccessor(uint64_t runNumber);

/// \brief checks if the provided validity uses old rules, where start is creation time, end is 10 years in the future.
bool isLegacyValidity(ValidityInterval);

namespace implementation
{
template <typename Iter, typename Accessor>
void commonActivityFields(Iter begin, Iter end, Accessor accessor, Activity& result)
{
  const Activity& first = accessor(*begin);

  // with enough effort (and maybe c++20), this could be further templated to avoid repetition
  // (calculate structure field offset, reinterpret_cast shifted pointers to the deduced types)
  if (std::all_of(std::next(begin), end, [&](const auto& item) { return first.mId == accessor(item).mId; })) {
    result.mId = first.mId;
  }
  if (std::all_of(std::next(begin), end, [&](const auto& item) { return first.mType == accessor(item).mType; })) {
    result.mType = first.mType;
  }
  if (std::all_of(std::next(begin), end,
                  [&](const auto& item) { return first.mPassName == accessor(item).mPassName; })) {
    result.mPassName = first.mPassName;
  }
  if (std::all_of(std::next(begin), end,
                  [&](const auto& item) { return first.mPeriodName == accessor(item).mPeriodName; })) {
    result.mPeriodName = first.mPeriodName;
  }
  if (std::all_of(std::next(begin), end,
                  [&](const auto& item) { return first.mProvenance == accessor(item).mProvenance; })) {
    result.mProvenance = first.mProvenance;
  }
  if (std::all_of(std::next(begin), end,
                  [&](const auto& item) { return first.mBeamType == accessor(item).mBeamType; })) {
    result.mBeamType = first.mBeamType;
  }
}
} // namespace implementation
} // namespace o2::quality_control::core

#endif // QUALITYCONTROL_ACTIVITYHELPERS_H
