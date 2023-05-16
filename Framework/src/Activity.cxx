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
/// \file   Activity.cxx
/// \author Piotr Konopka
///

#include "QualityControl/Activity.h"
#include "QualityControl/ObjectMetadataKeys.h"
#include <iostream>

using namespace o2::quality_control::repository;

namespace o2::quality_control::core
{

std::ostream& operator<<(std::ostream& out, const Activity& activity)
{
  out << metadata_keys::runNumber << ": " << activity.mId
      << ", " << metadata_keys::runType << ": " << activity.mType
      << ", " << metadata_keys::periodName << ": '" << activity.mPeriodName
      << "', " << metadata_keys::passName << ": '" << activity.mPassName
      << "', provenance: '" << activity.mProvenance
      << "', " << metadata_keys::validFrom << ": " << activity.mValidity.getMin()
      << ", " << metadata_keys::validUntil << ": " << activity.mValidity.getMax()
      << ", " << metadata_keys::beamType << ": '" << activity.mBeamType << "'";
  return out;
}

bool Activity::matches(const Activity& other) const
{
  // Note that 'this' can be 'any', but 'other' cannot.
  // E.g. if we require that run number is concrete, it cannot match 'other' which has 'any' run number.
  // Also, since we do not indicate the correct validity of objects, we require that the other Activity validity start
  // is included in this validity. If checked for any overlaps, we would match with all past Activities, which is not
  // what we want e.g. in Post-processing triggers. Once we indicate the correct validity, we can change this behaviour.
  return (mId == 0 || mId == other.mId) &&
         (mType == 0 || mType == other.mType) &&
         (mPeriodName.empty() || mPeriodName == other.mPeriodName) &&
         (mPassName.empty() || mPassName == other.mPassName) &&
         (mProvenance == other.mProvenance) && // provenance has to match!
         !mValidity.isOutside(other.mValidity.getMin()) &&
         (mBeamType.empty() || mBeamType == other.mBeamType);
}

bool Activity::same(const Activity& other) const
{
  return mId == other.mId &&
         mType == other.mType &&
         mPeriodName == other.mPeriodName &&
         mPassName == other.mPassName &&
         mProvenance == other.mProvenance &&
         mBeamType == other.mBeamType;
}

bool Activity::operator==(const Activity& other) const
{
  return mId == other.mId &&
         mType == other.mType &&
         mPeriodName == other.mPeriodName &&
         mPassName == other.mPassName &&
         mProvenance == other.mProvenance &&
         mValidity == other.mValidity &&
         mBeamType == other.mBeamType;
}

} // namespace o2::quality_control::core
