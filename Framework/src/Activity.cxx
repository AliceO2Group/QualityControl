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

namespace o2::quality_control::core
{

bool Activity::matches(const Activity& other) const
{
  // Note that 'this' can be 'any', but 'other' cannot.
  // E.g. if we require that run number is concrete, it cannot match 'other' which has 'any' run number.
  return (mId == 0 || mId == other.mId) &&
         (mType == 0 || mType == other.mType) &&
         (mPeriodName.empty() || mPeriodName == other.mPeriodName) &&
         (mPassName.empty() || mPassName == other.mPassName) &&
         (mProvenance == other.mProvenance); // provenance has to match!
}

} // namespace o2::quality_control::core
