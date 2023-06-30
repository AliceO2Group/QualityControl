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
/// \file   TimekeeperAsynchronous.cxx
/// \author Piotr Konopka
///

#include "QualityControl/TimekeeperAsynchronous.h"
#include "QualityControl/QcInfoLogger.h"

#include <CommonConstants/LHCConstants.h>

namespace o2::quality_control::core
{

TimekeeperAsynchronous::TimekeeperAsynchronous(validity_time_t windowLengthMs)
  : Timekeeper(), mWindowLengthMs(windowLengthMs)
{
}

void TimekeeperAsynchronous::updateByCurrentTimestamp(validity_time_t timestampMs)
{
  // async QC should ignore current timestamp
}

void TimekeeperAsynchronous::updateByTimeFrameID(uint32_t tfid, uint64_t nOrbitsPerTF)
{
  // fixme: We might want to use this once we know how to get orbitResetTime:
  //  std::ceil((timingInfo.firstTForbit * o2::constants::lhc::LHCOrbitNS / 1000 + orbitResetTime) / 1000);
  //  Until then, we use a less precise method:
  if (mActivityDuration.isInvalid()) {
    ILOG(Warning, Support)
      << "trying to update the validity range with TF ID without having set the activity duration, returning" << ENDM;
    return;
  }
  if (tfid == 0) {
    if (!mWarnedAboutTfIdZero) {
      ILOG(Warning, Devel) << "Seen TFID equal to 0, which is not expected. Will not update TF-based validity, will not warn further." << ENDM;
      mWarnedAboutTfIdZero = true;
    }
    return;
  }

  auto tfDurationMs = constants::lhc::LHCOrbitNS / 1000000 * nOrbitsPerTF;
  auto tfStart = static_cast<validity_time_t>(mActivityDuration.getMin() + tfDurationMs * (tfid - 1));
  auto tfEnd = static_cast<validity_time_t>(mActivityDuration.getMin() + tfDurationMs * tfid - 1);
  mCurrentSampleTimespan.update(tfStart);
  mCurrentSampleTimespan.update(tfEnd);

  mCurrentTimeframeIdRange.update(tfid);

  if (mActivityDuration.isOutside(tfStart)) {
    ILOG(Warning, Support) << "Timestamp " << tfStart << " is outside of the assumed run duration ("
                           << mActivityDuration.getMin() << ", " << mActivityDuration.getMax() << ")" << ENDM;
    return;
  }

  if (mWindowLengthMs == 0) {
    mCurrentValidityTimespan = mActivityDuration;
  } else {
    size_t subdivisionIdx = (tfStart - mActivityDuration.getMin()) / mWindowLengthMs;
    size_t fullSubdivisions = mActivityDuration.delta() / mWindowLengthMs;
    if (subdivisionIdx < fullSubdivisions - 1) {
      mCurrentValidityTimespan.update(mActivityDuration.getMin() + subdivisionIdx * mWindowLengthMs);
      mCurrentValidityTimespan.update(mActivityDuration.getMin() + (subdivisionIdx + 1) * mWindowLengthMs);
    } else if (subdivisionIdx == fullSubdivisions - 1) {
      mCurrentValidityTimespan.update(mActivityDuration.getMin() + subdivisionIdx * mWindowLengthMs);
      mCurrentValidityTimespan.update(mActivityDuration.getMax());
    } else { // subdivisionIdx == fullSubdivisions
      mCurrentValidityTimespan.update(mActivityDuration.getMin() + (subdivisionIdx - 1) * mWindowLengthMs);
      mCurrentValidityTimespan.update(mActivityDuration.getMax());
    }
  }
}

void TimekeeperAsynchronous::reset()
{
  mCurrentSampleTimespan = gInvalidValidityInterval;
  mCurrentValidityTimespan = gInvalidValidityInterval;
  mCurrentTimeframeIdRange = gInvalidTimeframeIdRange;
}

template <typename T>
bool not_on_limit(T value)
{
  return value != std::numeric_limits<T>::min() && value != std::numeric_limits<T>::max();
}

validity_time_t
  TimekeeperAsynchronous::activityBoundarySelectionStrategy(validity_time_t ecsTimestamp,
                                                            validity_time_t configTimestamp,
                                                            validity_time_t currentTimestamp,
                                                            std::function<validity_time_t(void)> ccdbTimestampAccessor)
{
  validity_time_t selected = 0;
  auto ccdbTimestamp = ccdbTimestampAccessor == nullptr ? std::numeric_limits<validity_time_t>::min() : ccdbTimestampAccessor();
  if (not_on_limit(ccdbTimestamp)) {
    selected = ccdbTimestamp;
  } else if (not_on_limit(ecsTimestamp)) {
    selected = ecsTimestamp;
  } else if (not_on_limit(configTimestamp)) {
    selected = configTimestamp;
  } else {
    // an exception could be thrown here once the values above are set correctly in production
  }
  ILOG(Info, Devel) << "Received the following activity boundary propositions: " << ccdbTimestamp << ", " << ecsTimestamp
                    << ", " << configTimestamp << ", " << currentTimestamp << ". Selected: " << selected << ENDM;
  return selected;
}

} // namespace o2::quality_control::core