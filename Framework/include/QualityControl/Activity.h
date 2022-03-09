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
/// \file   Activity.h
/// \author Barthelemy von Haller
///

#ifndef QC_CORE_ACTIVITY_H
#define QC_CORE_ACTIVITY_H

#include <map>
#include <string>

#include "Rtypes.h"

namespace o2::quality_control::core
{

/// \brief Dummy class that should be removed when there is the official one.
/// This corresponds to a Run 1/2 "run".
/// \author   Barthelemy von Haller
class Activity
{
 public:
  Activity() = default;
  Activity(int id,
           int type,
           const std::string& periodName = "",
           const std::string& passName = "",
           const std::string& provenance = "qc") : mId(id),
                                                   mType(type),
                                                   mPeriodName(periodName),
                                                   mPassName(passName),
                                                   mProvenance(provenance) {}

  /// Copy constructor
  Activity(const Activity& other) = default;
  /// Move constructor
  Activity(Activity&& other) noexcept = default;
  /// Copy assignment operator
  Activity& operator=(const Activity& other) = default;
  /// Move assignment operator
  Activity& operator=(Activity&& other) noexcept = default;
  /// Comparator
  bool operator==(const Activity& other) const;

  /// Checks if the other activity matches this, taking into account that default values match any.
  bool matches(const Activity& other) const;

  virtual ~Activity() = default;

  int mId{ 0 };
  int mType{ 0 };
  std::string mPeriodName{};
  std::string mPassName{};
  std::string mProvenance{ "qc" };

  ClassDef(Activity, 1);
};

} // namespace o2::quality_control::core

#endif // QC_CORE_ACTIVITY_H
