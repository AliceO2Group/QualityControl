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
#include <iostream>

namespace o2::quality_control::core
{

std::ostream& operator<<(std::ostream& out, const Activity& activity)
{
  out << "id: " << activity.mId
      << ", type: " << activity.mType
      << ", period: '" << activity.mPeriodName
      << "', pass: '" << activity.mPassName
      << "', provenance: '" << activity.mProvenance
      << ", start: " << activity.mValidity.getMin()
      << ", end: " << activity.mValidity.getMax();
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
         !mValidity.isOutside(other.mValidity.getMin());
}

bool Activity::same(const Activity& other) const
{
  return mId == other.mId && mType == other.mType && mPeriodName == other.mPeriodName && mPassName == other.mPassName && mProvenance == other.mProvenance;
}

bool Activity::operator==(const Activity& other) const
{
  return mId == other.mId && mType == other.mType && mPeriodName == other.mPeriodName && mPassName == other.mPassName && mProvenance == other.mProvenance && mValidity == other.mValidity;
}
} // namespace o2::quality_control::core
