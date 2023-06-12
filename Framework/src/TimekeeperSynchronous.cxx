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
/// \file   TimekeeperSynchronous.cxx
/// \author Piotr Konopka
///

#include "QualityControl/TimekeeperSynchronous.h"
#include "QualityControl/QcInfoLogger.h"

#include <CommonConstants/LHCConstants.h>

namespace o2::quality_control::core
{

TimekeeperSynchronous::TimekeeperSynchronous(uint64_t nOrbitsPerTF) : Timekeeper(nOrbitsPerTF)
{
}

void TimekeeperSynchronous::updateByCurrentTimestamp(validity_time_t timestampMs)
{
  mCurrentValidityTimespan.update(timestampMs);
  mActivityDuration.update(timestampMs);
}

void TimekeeperSynchronous::updateByTimeFrameID(uint32_t tfid)
{
  if (mActivityDuration.getMin() == gInvalidValidityInterval.getMin() || mActivityDuration.isInvalid()) {
    if (!mWarnedAboutDataWithoutSOR) {
      ILOG(Warning, Devel)
        << "Data arrived before SOR time was set, cannot proceed with creating sample timespan. Will not warn further."
        << ENDM;
      mWarnedAboutDataWithoutSOR = true;
    }
  } else {
    // fixme: We might want to use this once we know how to get orbitResetTime:
    //  std::ceil((timingInfo.firstTForbit * o2::constants::lhc::LHCOrbitNS / 1000 + orbitResetTime) / 1000);
    //  Until then, we use a less precise method:
    auto tfStart = mActivityDuration.getMin() + constants::lhc::LHCOrbitNS / 1000000 * mNOrbitsPerTF * tfid;
    auto tfEnd = tfStart + constants::lhc::LHCOrbitNS / 1000000 * mNOrbitsPerTF - 1;
    mCurrentSampleTimespan.update(tfStart);
    mCurrentSampleTimespan.update(tfEnd);
  }

  mCurrentTimeframeIdRange.update(tfid);
}

void TimekeeperSynchronous::reset()
{
  mCurrentSampleTimespan = gInvalidValidityInterval;
  if (mCurrentValidityTimespan.isValid()) {
    mCurrentValidityTimespan.set(mCurrentValidityTimespan.getMax(), mCurrentValidityTimespan.getMax());
  }
  mCurrentTimeframeIdRange = gInvalidTimeframeIdRange;
}

validity_time_t
  TimekeeperSynchronous::activityBoundarySelectionStrategy(validity_time_t ecsTimestamp, validity_time_t configTimestamp,
                                                           validity_time_t currentTimestamp)
{
  auto notOnLimit = [](validity_time_t value) {
    return value != std::numeric_limits<validity_time_t>::min() && value != std::numeric_limits<validity_time_t>::max();
  };
  validity_time_t selected = 0;
  if (notOnLimit(ecsTimestamp)) {
    selected = ecsTimestamp;
  } else if (notOnLimit(currentTimestamp)) {
    selected = currentTimestamp;
  } else {
    selected = configTimestamp;
  }
  ILOG(Info, Devel) << "Received the following activity boundary propositions: " << ecsTimestamp
                    << ", " << configTimestamp << ", " << currentTimestamp << ". Selected: " << selected << ENDM;
  return selected;
}

} // namespace o2::quality_control::core