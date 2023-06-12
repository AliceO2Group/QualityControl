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
/// \file   Timekeeper.cxx
/// \author Piotr Konopka
///

#include "QualityControl/Timekeeper.h"
#include "QualityControl/QcInfoLogger.h"
#include "QualityControl/runnerUtils.h"

namespace o2::quality_control::core
{

Timekeeper::Timekeeper(uint64_t nOrbitsPerTF)
  : mNOrbitsPerTF(nOrbitsPerTF)
{
  if (nOrbitsPerTF == 0) {
    ILOG(Warning, Support) << "nOrbitsPerTF was set to 0, object validity may be incorrectly marked" << ENDM;
  }
}

void Timekeeper::setActivityDuration(ValidityInterval validity)
{
  mActivityDuration = validity;
}

ValidityInterval Timekeeper::getValidity() const
{
  return mCurrentValidityTimespan;
}

ValidityInterval Timekeeper::getSampleTimespan() const
{
  return mCurrentSampleTimespan;
}

TimeframeIdRange Timekeeper::getTimerangeIdRange() const
{
  return mCurrentTimeframeIdRange;
}

void Timekeeper::setStartOfActivity(validity_time_t ecsTimestamp, validity_time_t configTimestamp,
                                    validity_time_t currentTimestamp)
{
  mActivityDuration.setMin(activityBoundarySelectionStrategy(ecsTimestamp, configTimestamp, currentTimestamp));
}

void Timekeeper::setEndOfActivity(validity_time_t ecsTimestamp, validity_time_t configTimestamp,
                                  validity_time_t currentTimestamp)
{
  mActivityDuration.setMax(activityBoundarySelectionStrategy(ecsTimestamp, configTimestamp, currentTimestamp));
}

ValidityInterval Timekeeper::getActivityDuration() const
{
  return mActivityDuration;
}

} // namespace o2::quality_control::core
