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

TimekeeperSynchronous::TimekeeperSynchronous() : Timekeeper()
{
}

void TimekeeperSynchronous::updateByCurrentTimestamp(validity_time_t timestampMs)
{
  mCurrentValidityTimespan.update(timestampMs);
  mActivityDuration.update(timestampMs);
}

void TimekeeperSynchronous::updateByTimeFrameID(uint32_t tfid, uint64_t nOrbitsPerTF)
{
  if (tfid == 0) {
    if (!mWarnedAboutTfIdZero) {
      ILOG(Warning, Devel) << "Seen TFID equal to 0, which is not expected. Will not update TF-based validity, will not warn further." << ENDM;
      mWarnedAboutTfIdZero = true;
    }
    return;
  }

  mCurrentTimeframeIdRange.update(tfid);

  if (mActivityDuration.getMin() == gInvalidValidityInterval.getMin() || mActivityDuration.isInvalid()) {
    if (!mWarnedAboutDataWithoutSOR) {
      ILOG(Warning, Devel)
        << "Data arrived before SOR time was set, cannot proceed with creating sample timespan. Will not warn further." << ENDM;
      mWarnedAboutDataWithoutSOR = true;
    }
    return;
  }

  // fixme: We might want to use this once we know how to get orbitResetTime:
  //  std::ceil((timingInfo.firstTForbit * o2::constants::lhc::LHCOrbitNS / 1000 + orbitResetTime) / 1000);
  //  Until then, we use a less precise method:
  auto tfDuration = constants::lhc::LHCOrbitNS / 1000000 * nOrbitsPerTF;
  auto tfStart = mActivityDuration.getMin() + tfDuration * (tfid - 1);
  auto tfEnd = tfStart + tfDuration - 1;
  mCurrentSampleTimespan.update(tfStart);
  mCurrentSampleTimespan.update(tfEnd);
}

void TimekeeperSynchronous::reset()
{
  mCurrentSampleTimespan = gInvalidValidityInterval;
  if (mCurrentValidityTimespan.isValid()) {
    mCurrentValidityTimespan.set(mCurrentValidityTimespan.getMax(), mCurrentValidityTimespan.getMax());
  }
  mCurrentTimeframeIdRange = gInvalidTimeframeIdRange;
}

template <typename T>
bool not_on_limit(T value)
{
  return value != std::numeric_limits<T>::min() && value != std::numeric_limits<T>::max();
}

validity_time_t
  TimekeeperSynchronous::activityBoundarySelectionStrategy(validity_time_t ecsTimestamp,
                                                           validity_time_t configTimestamp,
                                                           validity_time_t currentTimestamp,
                                                           std::function<validity_time_t(void)>)
{
  validity_time_t selected = 0;
  if (not_on_limit(ecsTimestamp)) {
    selected = ecsTimestamp;
  } else if (not_on_limit(currentTimestamp)) {
    selected = currentTimestamp;
  } else {
    selected = configTimestamp;
  }
  ILOG(Info, Devel) << "Received the following activity boundary propositions: " << ecsTimestamp
                    << ", " << configTimestamp << ", " << currentTimestamp << ". Selected: " << selected << ENDM;
  return selected;
}

} // namespace o2::quality_control::core